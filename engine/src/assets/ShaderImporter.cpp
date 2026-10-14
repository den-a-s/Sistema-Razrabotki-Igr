#include "AssetPipeline.hpp"

#include <utility>

#include <engine/assets/AssetPathResolver.hpp>

namespace engine {

bool ShaderImporter::ReadCpu(const AssetDesc& desc, ID3D11Device* /*device*/, CpuPayload& payload,
                            std::string& error) const {
    const auto path = AssetPathResolver::ResolveShader(desc);

    auto compiled = ShaderContext::CompileCpu(path.native(), desc.shaderStage);
    if (!compiled) {
        // CompileCpu already logged the compiler output with the file name.
        error = "HLSL compilation failed for " + desc.name;
        return false;
    }

    payload = CpuPayload{};
    payload.kind = CpuPayload::Kind::Shader;
    payload.shader = std::move(*compiled);
    return true;
}

bool ShaderImporter::UploadGpu(const UploadTransfer& transfer, const CpuPayload& payload,
                               GpuPayload& result, std::string& error) const {
    if (payload.kind != CpuPayload::Kind::Shader) {
        error = "ShaderImporter: payload is not compiled shader bytecode";
        return false;
    }

    const auto handle = transfer.resources->RegisterCompiledShader(payload.shader);
    if (!handle) {
        error = "shader registration on device failed";
        return false;
    }

    switch (payload.shader.type) {
    case ShaderType::Vertex:
        result.vertex = handle;
        break;
    case ShaderType::Pixel:
        result.pixel = handle;
        break;
    case ShaderType::Geometry:
        result.geometry = handle;
        break;
    }
    return true;
}

}  // namespace engine
