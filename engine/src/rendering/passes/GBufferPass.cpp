#include <engine/rendering/passes/GBufferPass.hpp>

#include <engine/rendering/UglyUtils.hpp>

namespace engine {

namespace {
void CreateGBuffer(RenderingResourceManager& resourceManager, RenderingResources& resources) {
    const auto textureDesc = [&](const DXGI_FORMAT format){
        return TextureDesc{
            .width = resources.screenWith, .height = resources.screenHeight,
            .format = format,
            .usage = TextureUsage::RenderTarget | TextureUsage::ShaderResource,
            .srvFormat = format,
            .dsvFormat = format,
        };
    };
    auto& gBuffer = resources.gBuffer;
    gBuffer.albedoTexture = resourceManager.CreateTexture(textureDesc(DXGI_FORMAT_R8G8B8A8_UNORM));
    gBuffer.normalTexture = resourceManager.CreateTexture(textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT));
    gBuffer.materialTexture = resourceManager.CreateTexture(textureDesc(DXGI_FORMAT_R8G8B8A8_UNORM));
    gBuffer.resultTexture = resourceManager.CreateTexture(textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT));
    auto desc = textureDesc(DXGI_FORMAT_R32_TYPELESS);
    desc.usage = TextureUsage::DepthStencil | TextureUsage::ShaderResource;
    desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
    desc.srvFormat = DXGI_FORMAT_R32_FLOAT;
    gBuffer.depthTexture = resourceManager.CreateTexture(desc);
}

void DestroyGBuffer(RenderingResourceManager& resourceManager, RenderingResources& resources) {
    auto& gBuffer = resources.gBuffer;
    resourceManager.DestroyTexture(gBuffer.albedoTexture);
    resourceManager.DestroyTexture(gBuffer.normalTexture);
    resourceManager.DestroyTexture(gBuffer.materialTexture);
    resourceManager.DestroyTexture(gBuffer.resultTexture);
    resourceManager.DestroyTexture(gBuffer.depthTexture);
    gBuffer = {};
}
}

void GBufferPass::Initialize(RenderingResourceManager& resourceManager, RenderingContext&,
                             RenderingResources& resources) {
    vertexShader_ = resourceManager.CompileShader(L"GBufferPass.hlsl", ShaderType::Vertex);
    pixelShader_ = resourceManager.CompileShader(L"GBufferPass.hlsl", ShaderType::Pixel);

    CreateGBuffer(resourceManager, resources);

    lightSphere_ = ugly_utils::FromRawMesh(resourceManager, ugly_utils::CreateSphere(1.0f));
}

void GBufferPass::Destroy(RenderingResourceManager& resourceManager, RenderingContext& context,
    RenderingResources& resources) {
    resourceManager.DestroyShader(vertexShader_);
    resourceManager.DestroyShader(pixelShader_);
    DestroyGBuffer(resourceManager, resources);
    resourceManager.DestroyMesh(lightSphere_);
}

void GBufferPass::Render(RenderWorld& world, RenderingContext& context, RenderingResources& resources) {
    auto& gBuffer = resources.gBuffer;
    context.SetViewport(resources.screenWith, resources.screenHeight);
    context.SetRasterizerState(RasterizerState::CullBack);
    context.ClearRenderTargetView(gBuffer.albedoTexture);
    context.ClearRenderTargetView(gBuffer.normalTexture);
    context.ClearRenderTargetView(gBuffer.materialTexture);
    context.ClearDepthView(gBuffer.depthTexture);

    context.SetRenderTargets({gBuffer.albedoTexture, gBuffer.normalTexture, gBuffer.materialTexture}, gBuffer.depthTexture);
    context.SetShader(vertexShader_);
    context.SetShader(pixelShader_);

    context.SetDepthState(DepthState::ReadWrite);
    world.BindCameraView(context, resources);

    //todo: generalize with other passes
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

        context.SetShaderResources(BindSlots::Texture::Albedo, ri.texture);

        context.DrawMesh(ri.mesh);
    }
}

bool GBufferPass::Reinitialize(RenderingResourceManager& resourceManager, RenderingContext&, RenderingResources& resources) {
    DestroyGBuffer(resourceManager, resources);
    CreateGBuffer(resourceManager, resources);
    return true;
}

}


