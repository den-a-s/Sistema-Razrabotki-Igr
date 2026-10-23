#include <engine/MaterialTemplate.hlsl>

void PatchGBufferVertex(inout MaterialVertex vertex) {
    vertex.position.y += 0.05 * sin(vertex.position.x);
}

void PatchShadowVertex(inout MaterialVertex vertex) {
    vertex.position.y += 0.05 * sin(vertex.position.x);
}

void PatchGBufferSurface(GBufferInput input, inout MaterialSurface surface) {
    surface.albedo.rgb *= float3(0.9, 1.0, 0.9);
}

