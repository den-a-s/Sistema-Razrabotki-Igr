#include "Common.hlsl"

struct MaterialVertex {
    float4 position;
    float3 normal;
    float2 uv;
};

//Material shaders could use normal and textures for shadows, default shader doesn't need them
struct MaterialVertexInput {
    float4 pos : POSITION0;
#ifdef MATERIAL_HAS_HOOK
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
#endif
};

struct ShadowVertexOutput {
    float4 pos : SV_POSITION;
    float3 worldPos : TEXCOORD1;
};

#ifdef MATERIAL_HAS_HOOK
void PatchShadowVertex(inout MaterialVertex vertex);
#else
void PatchShadowVertex(inout MaterialVertex vertex) {
}
#endif

ShadowVertexOutput VSMain(MaterialVertexInput input) {
    MaterialVertex vertex = (MaterialVertex)0;
    vertex.position = input.pos;
#ifdef MATERIAL_HAS_HOOK
    vertex.normal = input.normal;
    vertex.uv = input.tex;
#endif
    PatchShadowVertex(vertex);
    float4 worldSpace = mul(vertex.position, Object.worldMatrix);
    ShadowVertexOutput output = (ShadowVertexOutput)0;
    output.pos = worldSpace;
    output.worldPos = worldSpace.xyz;
    return output;
}
