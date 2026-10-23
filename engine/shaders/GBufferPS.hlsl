#include "Common.hlsl"

Texture2D albedoTexture : register(t0);

struct GBufferInput {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

struct MaterialSurface {
    float4 albedo;
    float3 normal; // world-space, normalized when written to the G-buffer
    float3 specularColor;
    float shininess;
};

struct GBufferOutput {
    float4 diffuse : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
};


#ifdef MATERIAL_HAS_HOOK
void PatchGBufferSurface(GBufferInput input, inout MaterialSurface surface);
#else
void PatchGBufferSurface(GBufferInput input, inout MaterialSurface surface){}
#endif

GBufferOutput PSMain(GBufferInput input) {
    MaterialSurface surface;
    surface.albedo = albedoTexture.Sample(LinearWrapSampler, input.tex);
    surface.normal = input.normal;
    surface.specularColor = Material.specularColor;
    surface.shininess = Material.shininess;
    PatchGBufferSurface(input, surface);
    GBufferOutput output;
    output.diffuse = surface.albedo;
    output.normal = float4(normalize(surface.normal) * 0.5 + 0.5, 1.0);
    output.material = float4(surface.specularColor, surface.shininess / 256.0);
    return output;
}