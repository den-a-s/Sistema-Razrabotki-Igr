#include <engine/rendering/passes/ShadowPass.hpp>

#include <cmath>

namespace engine {
namespace {

constexpr uint32_t mapSize = 1024;

std::vector<Vector4> GetFrustumCorners(const Matrix& viewMatrix, const Matrix& projectionMatrix) {
    std::vector<Vector4> frustumCorners;
    const auto inv = (viewMatrix * projectionMatrix).Invert();
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 2; z++) {
                Vector4 corner{
                    2.f * static_cast<float>(x) - 1,
                    2.f * static_cast<float>(y) - 1,
                    static_cast<float>(z),
                    1.0f
                };
                auto transformed = Vector4::Transform(corner, inv);
                frustumCorners.push_back(transformed / transformed.w);
            }
        }
    }
    return frustumCorners;
}

CascadeInfo GetCascadeData(Vector3 lightDirection, Matrix cameraView, float fov, float aspect, float nearPlane,
                           float farPlane) {
    std::array<float, 5> distances{nearPlane, 0, 0, 0, farPlane};
    // Blend log and linear scales.
    for (int i = 4; i >= 1; i--) {
        constexpr float t = 0.7f;
        float exp = nearPlane * powf(farPlane / nearPlane, static_cast<float>(i) / 4.f);
        float linear = nearPlane + (farPlane - nearPlane) * static_cast<float>(i) / 4.f;
        distances[i] = exp * t + linear * (1.0f - t);
    }
    CascadeInfo result{};
    // Each following cascade renders the last part of its predecessor's depth range as well, 
    // so lighting can blend two valid shadow samples at a split.
    constexpr float transitionFraction = 0.7f;
    for (int i = 0; i < 3; ++i) {
        result.bufferData.transitionStarts[i] =
            distances[i + 1] - (distances[i + 1] - distances[i]) * transitionFraction;
    }
    for (int i = 0; i < 4; i++) {
        float curNear = i == 0 ? distances[i] : result.bufferData.transitionStarts[i - 1];
        float curFar = distances[i + 1];

        result.bufferData.distances[i] = curFar;
        auto proj = Matrix::CreatePerspectiveFieldOfView(fov, aspect, curNear, curFar);
        auto frustumCorners = GetFrustumCorners(cameraView, proj);

        auto center = Vector4::Zero;
        for (auto& corner : frustumCorners) {
            center += corner;
        }
        center /= 8.f;

        // A sphere centered halfway along this frustum slice encloses its far corners. 
        // Its radius depends only on FOV, aspect and split distances, so rotating the camera cannot change the shadow projection's scale.
        const float halfDepth = (curFar - curNear) * 0.5f;
        const float farHalfHeight = curFar * std::tan(fov * 0.5f);
        const float farCornerRadius = farHalfHeight * std::sqrt(1.0f + aspect * aspect);
        float radius = std::sqrt(halfDepth * halfDepth + farCornerRadius * farCornerRadius);
        // Leave room for the PCF taps and texel snapping at the projection edge.
        radius = std::ceil(radius * 16.0f) / 16.0f * 1.01f;

        auto L = lightDirection;
        L.Normalize();

        Vector3 up = Vector3::Up;
        if (std::abs(L.Dot(up)) > 0.99f) {
            up = Vector3::Right;
        }

        // Keep the light axes anchored in world space. A view centered on the cascade would always put centerLight at (0, 0)
        const auto lightView = Matrix::CreateLookAt(-L, Vector3::Zero, up);

        auto centerLight = Vector4::Transform(center, lightView);

        constexpr float shadowMapSize = static_cast<float>(512); //translucency map will be 512
        //As this is only for snapping to avoid jitter, 512 would be fine for 1024 size maps too.
        float diameter = radius * 2.0f;
        float texelSize = diameter / shadowMapSize;
        centerLight.x = std::round(centerLight.x / texelSize) * texelSize;
        centerLight.y = std::round(centerLight.y / texelSize) * texelSize;

        const float casterMargin = radius * 2.0f;
        const float minZ = centerLight.z - radius - casterMargin;
        const float maxZ = centerLight.z + radius + casterMargin;
        // Reverse-Z: swap the light-space near and far distances.
        auto lightProj = Matrix::CreateOrthographicOffCenter(
            centerLight.x - radius, centerLight.x + radius,
            centerLight.y - radius, centerLight.y + radius, -minZ, -maxZ);
        result.viewMatrices[i] = lightView;
        result.projMatrices[i] = lightProj;
        result.bufferData.viewProjections[i] = lightView * lightProj;
    }
    return result;
}

CascadeInfo GetCascadeData(Vector3 lightDirection, const FPSCamera& camera) {
    return GetCascadeData(lightDirection, camera.GetViewMatrix(), camera.fov, camera.aspectRatio, camera.nearPlane,
                          camera.farPlane);
}

}

void ShadowCSMPass::Initialize(RenderingResourceManager& resourceManager, RenderingContext&,
                               RenderingResources& resources) {
    vertexShader_ = resourceManager.CompileShader(L"ShadowCSMPass.hlsl", ShaderType::Vertex);
    geometryShader_ = resourceManager.CompileShader(L"ShadowCSMPass.hlsl", ShaderType::Geometry);
    const TextureDesc shadowTextureDesc{
        .width = mapSize, .height = mapSize, .arraySize = 4,
        .format = DXGI_FORMAT_R24G8_TYPELESS,
        .usage = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
        .srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        .dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
    };
    resources.shadowTexture = resourceManager.CreateTexture(shadowTextureDesc);
}

void ShadowCSMPass::Destroy(RenderingResourceManager& resourceManager, RenderingContext& context,
                            RenderingResources& resources) {
    resourceManager.DestroyShader(vertexShader_);
    resourceManager.DestroyShader(geometryShader_);
    resourceManager.DestroyTexture(resources.shadowTexture);
}

void ShadowCSMPass::Render(RenderWorld& world, RenderingContext& context, RenderingResources& resources) {
    context.SetViewport(mapSize, mapSize);
    context.SetRasterizerState(RasterizerState::CullBackWithBias);
    context.ClearDepthView(resources.shadowTexture);
    context.SetRenderTargets({TextureHandle{}}, resources.shadowTexture);
    context.SetShader(vertexShader_);
    context.SetShader(geometryShader_);
    context.SetDepthState(DepthState::ReadWrite);

    world.BindDirectionalLightView(context, resources);
    resources.cascadeInfo = GetCascadeData(world.directionalLight.direction, world.camera);
    resources.shadowCascadesCBuffer.Update(context, [&](CascadeBufferData& data){
        data = resources.cascadeInfo.bufferData;
    });

    for (const auto& ri : world.renderItems) {
        resources.materialCBuffer.Update(context, [&](MaterialBufferData& data){
            data.specularColor = ri.material.specularColor;
            data.shininess = ri.material.shininess;
            data.specialType = ri.material.specialType;
        });

        resources.objectCBuffer.Update(context, [&](ObjectBufferData& data){
            data.worldMatrix = ri.worldMatrix;
            data.normalMatrix = ri.worldMatrix.Invert().Transpose();
        });

        context.DrawMesh(ri.mesh);
    }
}

}
