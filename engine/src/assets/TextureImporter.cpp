#include "AssetPipeline.hpp"

#include <format>
#include <utility>
#include <vector>

#include <ole2.h>

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include <engine/assets/AssetPathResolver.hpp>

namespace engine {
namespace {

/// WIC is COM and COM is initialized per thread, so a scheduler worker that nobody initialized
/// would fail every decode. CoInitializeEx returns S_FALSE when the thread already was, meaning
/// this call took a refcount, so the matching CoUninitialize stays balanced either way.
class ScopedCom {
public:
    ScopedCom() : ok_(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {}
    ~ScopedCom() {
        if (ok_) {
            CoUninitialize();
        }
    }
    ScopedCom(const ScopedCom&) = delete;
    ScopedCom& operator=(const ScopedCom&) = delete;

    [[nodiscard]] explicit operator bool() const noexcept {
        return ok_;
    }

private:
    bool ok_ = false;
};

}  // namespace

bool TextureImporter::ReadCpu(const AssetDesc& desc, ID3D11Device* device, CpuPayload& payload,
                              std::string& error) const {
    if (device == nullptr) {
        error = "no device yet: a texture cannot be decoded before the rendering subsystem is up";
        return false;
    }

    const auto path = AssetPathResolver::ResolveTexture(desc);

    std::vector<uint8_t> fileBytes;
    if (!ReadWholeFileInto(path, fileBytes, error)) {
        return false;
    }

    payload = CpuPayload{};
    payload.kind = CpuPayload::Kind::Texture;

    ComPtr<ID3D11ShaderResourceView> view;
    HRESULT res = E_FAIL;
    if (_wcsicmp(path.extension().c_str(), L".dds") == 0) {
        res = DirectX::CreateDDSTextureFromMemory(
            device, fileBytes.data(), fileBytes.size(), nullptr, view.GetAddressOf());
    } else {
        const ScopedCom com;
        if (!com) {
            error = std::format("cannot initialize COM on this thread to decode '{}'", path.string());
            return false;
        }
        // The device-only overload: with a context DirectXTK would defer the mip chain to a
        // GenerateMips call, which is main-thread work again. Decoding and building the chain
        // here is the whole point of the CPU phase for a texture.
        res = DirectX::CreateWICTextureFromMemoryEx(
            device,
            fileBytes.data(), fileBytes.size(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE,
            0,
            0,
            DirectX::WIC_LOADER_FORCE_SRGB,
            nullptr,
            view.GetAddressOf());
    }

    if (FAILED(res) || !view) {
        error = std::format("decoding '{}' failed with {:08x}", path.string(), static_cast<unsigned>(res));
        return false;
    }

    payload.texture.view = std::move(view);
    return true;
}

bool TextureImporter::UploadGpu(const UploadTransfer& transfer, const CpuPayload& payload,
                                GpuPayload& result, std::string& error) const {
    if (payload.kind != CpuPayload::Kind::Texture) {
        error = "TextureImporter: payload is not a decoded texture";
        return false;
    }

    // A copy rather than a move: the payload is const here and stays readable, so a record that
    // is recycled and re-uploaded still has what it needs.
    ComPtr<ID3D11ShaderResourceView> view = payload.texture.view;
    result.texture = transfer.resources->RegisterTexture(Texture(std::move(view)));
    return true;
}

}  // namespace engine
