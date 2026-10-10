#pragma once
#include <SimpleMath.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <engine/engine_export.hpp>

#include "RenderingResourceManager.hpp"

namespace engine {
namespace ugly_utils {
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;

struct ObjVertex {
    Vector3 position{};
    Vector3 normal{};
    Vector2 uv{};
};

struct ObjModel {
    std::vector<ObjVertex> vertices{};
    std::vector<uint32_t> indices{};
    ComPtr<ID3D11ShaderResourceView> textureSRV{};
};

struct RenderingState {
    ID3D11Device* device;
    ID3D11DeviceContext* context;
};

ENGINE_API void CenterModel(ObjModel& model, bool normalize = true);
ENGINE_API void LoadTexture(RenderingState& state, const std::filesystem::path& texturePath,
                            ComPtr<ID3D11ShaderResourceView>& texture);
ENGINE_API void LoadTexture(RenderingState& state, const wchar_t* path,
                            ComPtr<ID3D11ShaderResourceView>& texture);
ENGINE_API ObjModel LoadObjModel(RenderingState& state, const wchar_t* folder, bool doCentering = false);

/// Pure CPU path of LoadObjModel: reads and parses one .obj file, optionally recenters and
/// normalizes the geometry. No GPU work, no texture loading, no termination on bad input;
/// on failure returns false with the reason in error. model.textureSRV stays null.
ENGINE_API bool ParseObjFile(const std::filesystem::path& objPath, bool center, ObjModel& model,
                             std::string& error);

struct VertData {
    Vector4 pos;
    Vector3 norm;
    Vector2 uv;
};

struct RawMesh {
    std::vector<VertData> vertexes;
    std::vector<uint32_t> indicies;
};

ENGINE_API MeshHandle FromRawMesh(RenderingResourceManager& resourceManager, const RawMesh& rawMesh);
ENGINE_API RawMesh CreateSphere(float size);
ENGINE_API RawMesh CreateCube(float sizeX, float sizeY, float sizeZ);
ENGINE_API RawMesh CreateSpotLightBoundingBox(float range, float spotOuterAngle);

}
}
