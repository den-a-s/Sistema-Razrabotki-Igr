#include "Common.hlsl"

struct MaterialVertex {
    float4 position;
    float3 normal;
    float2 uv;
};

struct MaterialVertexInput {
    float4 pos : POSITION0;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
};

struct GBufferInput {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

#ifdef MATERIAL_HAS_HOOK
void PatchGBufferVertex(inout MaterialVertex vertex);
#else 
void PatchGBufferVertex(inout MaterialVertex vertex) {}
#endif

GBufferInput VSMain(MaterialVertexInput input) {
    MaterialVertex vertex = {input.pos, input.normal, input.tex};
    PatchGBufferVertex(vertex);
    float4 worldSpace = mul(vertex.position, Object.worldMatrix);
    GBufferInput output = (GBufferInput)0;
    output.pos = mul(mul(worldSpace, View.viewMatrix), View.projectionMatrix);
    output.normal = normalize(mul(vertex.normal, (float3x3)Object.normalMatrix));
    output.tex = vertex.uv;
    output.worldPos = worldSpace.xyz;
    return output;
}