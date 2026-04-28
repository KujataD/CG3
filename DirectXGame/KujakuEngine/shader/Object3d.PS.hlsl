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
ConstantBuffer<Camera> gPointLight : register(b3);

PixelShaderOutput main(VertexShaderOutput input)
{
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    PixelShaderOutput output;
    if (gMaterial.enableLighting != 0)
    { // Lightingする場合
    
        if (gMaterial.enableLighting == 1)
        {
            float cos = saturate(dot(normalize(input.normal), -normalize(gDirectionalLight.direction))); // lambertModel
    
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        }
        else if (gMaterial.enableLighting == 2)
        {
            // halfLambertModel
            float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
        }
        else if (gMaterial.enableLighting == 3)
        {
            // PhongReflectionModel
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
        {
            // BlingPhongReflectionModel
            float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            // カメラへの方向を算出する。数式のv
            float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
            // 入射角の反射ベクトルを求める。数式のr
            float32_t3 reflectLight = reflect(normalize(gDirectionalLight.direction), normalize(input.normal));

            // 内積をとって、saturateして、shininessを階乗すると鏡面反射の強度が求まる
            float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);
            
            // 拡散反射
            float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            // 鏡面反射
            float32_t3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            // 拡散反射+鏡面反射
            output.color.rgb = diffuse + specular;
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
