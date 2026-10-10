#include "AssetPipeline.hpp"

#include <format>

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include <engine/assets/AssetPathResolver.hpp>

namespace engine {

bool TextureImporter::ReadCpu(const AssetDesc& desc, CpuPayload& payload, std::string& error) const {
    const auto path = AssetPathResolver::ResolveTexture(desc);

    payload = CpuPayload{};
    payload.kind = CpuPayload::Kind::Bytes;
    payload.bytes.isDds = path.extension() == L".dds" || path.extension() == L".DDS";
    return ReadWholeFileInto(path, payload.bytes.bytes, error);
}

bool TextureImporter::UploadGpu(const UploadTransfer& transfer, const CpuPayload& payload,
                               GpuPayload& result, std::string& error) const {
    if (payload.kind != CpuPayload::Kind::Bytes) {
        error = "TextureImporter: payload is not raw file bytes";
        return false;
    }

    ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT res = E_FAIL;
    if (payload.bytes.isDds) {
        res = DirectX::CreateDDSTextureFromMemory(
            transfer.device, payload.bytes.bytes.data(), payload.bytes.bytes.size(),
            nullptr, srv.GetAddressOf());
    } else {
        // Same parameters as the legacy CreateWICTextureFromFileEx path, including the
        // forced sRGB view; the device+context overload keeps mip generation behavior.
        res = DirectX::CreateWICTextureFromMemoryEx(
            transfer.device, transfer.context,
            payload.bytes.bytes.data(), payload.bytes.bytes.size(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE,
            0,
            0,
            DirectX::WIC_LOADER_FORCE_SRGB,
            nullptr,
            srv.GetAddressOf());
    }

    if (FAILED(res) || !srv) {
        error = std::format("texture upload failed with {:08x}", static_cast<unsigned>(res));
        return false;
    }

    result.texture = transfer.resources->RegisterTexture(Texture(std::move(srv)));
    return true;
}

}  // namespace engine
