#include "Common.hlsl"

Texture2D albedoTexture : register(t0);

struct VS_IN {
    float4 pos : POSITION0;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
};

struct PS_IN {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

struct PS_OUT {
    float4 diffuse : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
};

PS_IN VSMain(VS_IN input) {
    float4 worldSpace = mul(input.pos, Object.worldMatrix);
    float3 newNormal = mul(input.normal, Object.normalMatrix);
    
    float4 cameraSpace = mul(worldSpace, View.viewMatrix);
    float4 clipSpace = mul(cameraSpace, View.projectionMatrix);

    PS_IN output = (PS_IN)0;
    output.pos = clipSpace;
    output.normal = normalize(newNormal);
    output.tex = input.tex;
    output.worldPos = worldSpace;
    return output;
}


PS_OUT PSMain(PS_IN input) : SV_Target {
    PS_OUT output = (PS_OUT)0;
    output.diffuse = albedoTexture.Sample(LinearWrapSampler, input.tex);
    output.normal = float4(input.normal * 0.5 + 0.5, 1.0);
    output.material = float4(Material.specularColor, Material.shininess / 256.0);
    return output;
}
