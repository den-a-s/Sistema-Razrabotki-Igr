#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <engine/engine_export.hpp>
#include <wrl/client.h>

#include <engine/rendering/resources/VertexLayoutCache.hpp>

namespace engine {

using Microsoft::WRL::ComPtr;

enum class ShaderType {
    Vertex,
    Pixel,
    Geometry,
};

struct ShaderDefine {
    std::string name;
    std::string value = "1";
};

struct ShaderCompileOptions {
    /// Empty selects VSMain, PSMain or GSMain for the requested stage.
    std::string entryPoint;
    std::vector<ShaderDefine> defines;
};

ENGINE_API const char* GetEntryPoint(ShaderType type);
ENGINE_API const char* GetDX11Target(ShaderType type);

struct ENGINE_API ShaderBinding {
    std::string name;
    uint8_t slot;
    uint8_t type; // aka D3D_SHADER_INPUT_TYPE, but 8 bits
};

struct ENGINE_API DXShader {
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11GeometryShader> geometryShader;
};

/// Bytecode and reflection of one compiled stage. Device-independent: it can be produced
/// on a worker thread; only FinalizeOnDevice touches the D3D device.
struct ENGINE_API CompiledShader {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3D11ShaderReflection> reflection;
    D3D11_SHADER_DESC desc{};
    std::vector<ShaderBinding> bindings;
    ShaderType type = ShaderType::Vertex;
};

struct ENGINE_API ShaderContext {
    ComPtr<ID3DBlob> blob;
    ShaderType type;
    ShaderLayoutHandle layoutHandle;

    // todo make a proper destructor
    DXShader shader;

    // debug only, later we'll add correctness checks based on names and slots
    std::vector<ShaderBinding> bindings{};

    static std::optional<ShaderContext> CompileShader(ID3D11Device* device, InputLayoutCache& layoutCache,
                                                        const std::filesystem::path& fileName, ShaderType shaderType,
                                                        const ShaderCompileOptions& options = {});

    /// D3DCompile + reflection only; runs on any thread.
    static std::optional<CompiledShader> CompileCpu(const std::filesystem::path& fileName, ShaderType shaderType,
                                                    const ShaderCompileOptions& options = {});
    /// Compiles generated HLSL; virtualPath supplies its include location and diagnostic name.
    static std::optional<CompiledShader> CompileSource(std::string_view source,
                                                       const std::filesystem::path& virtualPath, ShaderType shaderType,
                                                       const ShaderCompileOptions& options = {});
    /// Create*Shader and input-layout registration; main thread.
    static std::optional<ShaderContext> FinalizeOnDevice(const CompiledShader& compiled, ID3D11Device* device,
                                                          InputLayoutCache& layoutCache);

private:
    static ShaderVertexLayout ParseVertexShaderInputs(const ComPtr<ID3D11ShaderReflection>& reflection,
                                                       const D3D11_SHADER_DESC& shaderDesc,
                                                       const ComPtr<ID3DBlob>& blob);
};

using ShaderHandle = HandleMap<ShaderContext>::Handle;
}
