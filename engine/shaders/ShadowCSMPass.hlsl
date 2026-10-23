#include "Common.hlsl"

struct VS_IN {
    float4 pos : POSITION0;
};

struct GS_IN {
    float4 pos : SV_POSITION;
    float3 worldPos : TEXCOORD1;
};

struct GS_OUT {
    float4 pos : SV_POSITION;
    uint arrInd : SV_RenderTargetArrayIndex;
};

GS_IN VSMain(VS_IN input) {
    float4 worldSpace = mul(input.pos, Object.worldMatrix);

    GS_IN output = (GS_IN)0;
    output.pos = worldSpace;
    output.worldPos = output.pos;
    return output;
}

[instance(4)]
[maxvertexcount(3)]
void GSMain(triangle GS_IN p[3], in uint id : SV_GSInstanceID, inout TriangleStream<GS_OUT> stream) {
    for (int i = 0; i < 3; i++) {
        GS_OUT gs = (GS_OUT)0;
        gs.pos = mul(float4(p[i].worldPos.xyz, 1.0f), ShadowCascades.viewProjections[id]);
        gs.arrInd = id;
        stream.Append(gs);
    }
}
