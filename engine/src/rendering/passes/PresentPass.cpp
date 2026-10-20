#include <engine/rendering/passes/PresentPass.hpp>

#include <cstdlib>
#include <utility>

#include <spdlog/spdlog.h>
#include <wrl/client.h>

#include <engine/rendering/RenderingContext.hpp>
#include <engine/rendering/resources/Texture.hpp>

namespace engine {

namespace {
TextureHandle CreateBackbufferTexture(RenderingResourceManager& resourceManager, RenderingContext& context) {
    //todo: this generally doesnt have to be here
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT result = context.GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(result)) {
        spdlog::error("Failed to get the back-buffer render target");
        return {};
    }
    ComPtr<ID3D11RenderTargetView> backBufferView;
    result = context.GetDevice()->CreateRenderTargetView(backBuffer.Get(), nullptr, backBufferView.GetAddressOf());
    if (FAILED(result)) {
        spdlog::error("Failed to create the back-buffer render target");
        return {};
    }
    Texture backBufferTexture(std::move(backBufferView));
    return resourceManager.RegisterTexture(std::move(backBufferTexture));
}

}

void PresentPass::Initialize(RenderingResourceManager& resourceManager, RenderingContext& context,
                             RenderingResources&) {
    vertexShader_ = resourceManager.CompileShader(L"PresentPass.hlsl", ShaderType::Vertex);
    pixelShader_ = resourceManager.CompileShader(L"PresentPass.hlsl", ShaderType::Pixel);

    backBufferView_ = CreateBackbufferTexture(resourceManager, context);
    if (!backBufferView_) {
        std::exit(-1); //todo initialization error handling
    }
}

void PresentPass::Destroy(RenderingResourceManager& resourceManager, RenderingContext& context,
    RenderingResources& resources) {
    resourceManager.DestroyShader(vertexShader_);
    resourceManager.DestroyShader(pixelShader_);
    if (backBufferView_) {
        resourceManager.DestroyTexture(backBufferView_);
        backBufferView_ = {};
    }
}

void PresentPass::Render(RenderWorld&, RenderingContext& context, RenderingResources& resources) {
    context.SetRenderTargets({backBufferView_}, TextureHandle{});
    context.SetViewport(resources.screenWith, resources.screenHeight);
    context.SetBlendState(BlendState::Opaque);
    context.SetDepthState(DepthState::Disabled);
    context.SetRasterizerState(RasterizerState::CullNone);
    context.SetShader(vertexShader_);
    context.SetShader(pixelShader_);
    context.SetShaderResources(BindSlots::Texture::SceneResult, resources.gBuffer.resultTexture);
    context.RawDraw(6, 0);
    context.Present();
}

bool PresentPass::PreReinitialize(RenderingResourceManager& resourceManager, RenderingContext& context,
                                  RenderingResources& resources) {
    if (backBufferView_) {
        resourceManager.DestroyTexture(backBufferView_);
        backBufferView_ = {};
    }
    return true;
}

bool PresentPass::Reinitialize(RenderingResourceManager& resourceManager, RenderingContext& context,
                               RenderingResources& resources) {
    backBufferView_ = CreateBackbufferTexture(resourceManager, context);
    return static_cast<bool>(backBufferView_);
}

}
