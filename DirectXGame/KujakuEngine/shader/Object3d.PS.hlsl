#include "object3d.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;

};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);


static const uint32_t kMaxPointLight = 16;
struct PointLight
{
    float32_t4 color;   // !< ライトの色
    float32_t3 position;// !< ライトの位置
    float32_t intensity;// !< 輝度
    float32_t radius;   // !< ライトの届く最大距離
    float32_t decay;    // !< 減衰率
    float32_t2 padding;
};
cbuffer gPointLight : register(b3)
{
    PointLight pointLights[kMaxPointLight];
    int32_t pointLightCount;
};

struct SpotLight
{
    float32_t4 color;   // !< ライトの色
    float32_t3 position;// !< ライトの位置
    float32_t intensity;// !< 輝度
    float32_t3 direction; // !< スポットライトの方向
    float32_t distance; // !< ライトの届く最大距離
    float32_t decay; // !< 減衰率
    float32_t cosAngle; // スポットライトの余弦
    float32_t cosFalloffStart;
    float32_t2 padding;
    
};

ConstantBuffer<SpotLight> gSpotLight : register(b4);

PixelShaderOutput main(VertexShaderOutput input)
{
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    PixelShaderOutput output;
    // ライティング各処理
    if (gMaterial.enableLighting != 0)
    { // Lightingする場合
    
        if (gMaterial.enableLighting == 1)
        { // lambertModel
            float cos = saturate(dot(normalize(input.normal), -normalize(gDirectionalLight.direction)));
    
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        }
        else if (gMaterial.enableLighting == 2)
        { // halfLambertModel
            
            float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        }
        else if (gMaterial.enableLighting == 3)
        { // PhongReflectionModel
           
            float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            // カメラへの方向を算出する。数式のv
            float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
            // 入射角の反射ベクトルを求める。数式のr
            float32_t3 reflectLight = reflect(normalize(gDirectionalLight.direction), normalize(input.normal));
    
            // 内積をとって、saturateして、shininessを階乗すると鏡面反射の強度が求まる
            float32_t RdotE = dot(reflectLight, toEye);
            float32_t specularPow = pow(saturate(RdotE), gMaterial.shininess); // 反射強度
            
            // 拡散反射
            float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            // 鏡面反射
            float32_t3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            // 拡散反射+鏡面反射
            output.color.rgb = diffuse + specular;
            // アルファは今まで通り
            output.color.a = gMaterial.color.a * textureColor.a;
            
        }
        else if (gMaterial.enableLighting == 4)
        { // BlingPhongReflectionModel
            
            float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            // カメラへの方向を算出する。数式のv
            float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
            // DirectionalLight計算処理
            // ----------------------------------
            
            // 入射角の反射ベクトルを求める。数式のr
            float32_t3 reflectLight = reflect(normalize(gDirectionalLight.direction), normalize(input.normal));

            // 内積をとって、saturateして、shininessを階乗すると鏡面反射の強度が求まる
            float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);
            
            // 拡散反射
            float32_t3 directionalLightDiffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            // 鏡面反射
            float32_t3 directionalLightSpecular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            // 拡散反射+鏡面反射
            output.color.rgb = directionalLightDiffuse + directionalLightSpecular;
            
            // PointLights実装
            // ----------------------------------
            for (int32_t i = 0; i < pointLightCount; i++)
            {
                // 方向
                float32_t3 pointLightDirection = normalize(pointLights[i].position - input.worldPosition);
                float32_t pointLightCos = saturate(dot(normalize(input.normal), pointLightDirection));
                float32_t3 halfVectorPoint = normalize(pointLightDirection + toEye);
                float32_t pointLightSpecularPow = pow(saturate(dot(normalize(input.normal), halfVectorPoint)), gMaterial.shininess);
                

                float32_t distance = length(pointLights[i].position - input.worldPosition); // ポイントライトへの距離
                float32_t factor = pow(saturate(-distance / pointLights[i].radius + 1.0f), pointLights[i].decay); // 指数によるコントロール
                
                // 拡散反射
                float32_t3 pointLightDiffuse = gMaterial.color.rgb * textureColor.rgb * pointLights[i].color.rgb * pointLightCos * pointLights[i].intensity * factor;
                // 鏡面反射
                float32_t3 pointLightSpecular = pointLights[i].color.rgb * pointLights[i].intensity * pointLightSpecularPow * float32_t3(1.0f, 1.0f, 1.0f) * factor;
                
                output.color.rgb += pointLightDiffuse + pointLightSpecular;

            }
            
            // SpotLights実装
            // ----------------------------------
            {  
                // 方向
                float32_t3 spotLightDirection = normalize(gSpotLight.position - input.worldPosition);
                float32_t spotLightCos = saturate(dot(normalize(input.normal), spotLightDirection));
                float32_t3 halfVectorSpot = normalize(spotLightDirection + toEye);
                float32_t spotLightSpecularPow = pow(saturate(dot(normalize(input.normal), halfVectorSpot)), gMaterial.shininess);
                
                float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);
                
                float32_t cosAngle = dot(spotLightDirectionOnSurface, normalize(gSpotLight.direction));
                float32_t falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
                
                float32_t distance = length(gSpotLight.position - input.worldPosition);
                float32_t attenuationFactor = pow(saturate(-distance / gSpotLight.distance + 1.0f), gSpotLight.decay);
                
                // 拡散反射
                float32_t3 spotLightDiffuse = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * spotLightCos * gSpotLight.intensity * attenuationFactor * falloffFactor;
                // 鏡面反射
                float32_t3 spotLightSpecular = gSpotLight.color.rgb * gSpotLight.intensity * spotLightSpecularPow * float32_t3(1.0f, 1.0f, 1.0f) * attenuationFactor * falloffFactor;
                
                output.color.rgb += spotLightDiffuse + spotLightSpecular;

            }
            // アルファは今まで通り
            output.color.a = gMaterial.color.a * textureColor.a;
        }
        else
        {
            float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction)); // lambertModel
    
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        }
    }
    else
    { //Lightingしない場合。前回までと同じ演算
        output.color = gMaterial.color * textureColor;
    }
    
    if (textureColor.a <= 0.5)
    {
        discard;
    }
    if (textureColor.a == 0.0)
    {
        discard;
    }
    if (output.color.a == 0.0)
    {
        discard;
    }
    
    return output;
}
