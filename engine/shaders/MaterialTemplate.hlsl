// Include from a material hook file. Compile once per declared stage with
// MATERIAL_GBUFFER_VS, MATERIAL_GBUFFER_PS or MATERIAL_SHADOW_VS.
#include "Common.hlsl"

#if defined(MATERIAL_GBUFFER_VS)
#include "GBufferVS.hlsl"
#elif defined(MATERIAL_SHADOW_VS)
#include "ShadowVS.hlsl"
#elif defined(MATERIAL_GBUFFER_PS)
#include "GBufferPS.hlsl"
#else
#error Material template requires a stage define
#endif
