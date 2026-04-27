#include "particle.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};
StructuredBuffer<TransformationMatrix> gTranformationMatrices : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTranformationMatrices[instanceId].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3)gTranformationMatrices[instanceId].World));
    return output;
}
