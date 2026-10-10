#include "AssetPipeline.hpp"

#include <utility>

#include <engine/assets/AssetPathResolver.hpp>

namespace engine {

bool ObjMeshImporter::ReadCpu(const AssetDesc& desc, CpuPayload& payload, std::string& error) const {
    ugly_utils::ObjModel model{};
    if (!ugly_utils::ParseObjFile(AssetPathResolver::ResolveMesh(desc), desc.centerOnLoad, model,
                                  error)) {
        return false;
    }

    payload = CpuPayload{};
    payload.kind = CpuPayload::Kind::Mesh;
    payload.mesh.vertices.reserve(model.vertices.size());
    for (const auto& vertex : model.vertices) {
        payload.mesh.vertices.push_back(ugly_utils::VertData{
            .pos = DirectX::SimpleMath::Vector4{vertex.position.x, vertex.position.y,
                                                vertex.position.z, 1.0f},
            .norm = vertex.normal,
            .uv = vertex.uv
        });
    }
    payload.mesh.indices = std::move(model.indices);
    return true;
}

bool ObjMeshImporter::UploadGpu(const UploadTransfer& transfer, const CpuPayload& payload,
                                GpuPayload& result, std::string& error) const {
    if (payload.kind != CpuPayload::Kind::Mesh) {
        error = "ObjMeshImporter: payload is not a parsed mesh";
        return false;
    }

    const auto vertexBuffer = transfer.resources->CreateVertexBuffer<ugly_utils::VertData>(
        payload.mesh.vertices);
    const auto indexBuffer = transfer.resources->CreateIndexBuffer<uint32_t>(payload.mesh.indices);

    VertexLayout layout;
    layout.Add("POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    layout.Add("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 16);
    layout.Add("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 16 + 12);

    result.mesh = transfer.resources->CreateMesh(
        vertexBuffer, indexBuffer, std::move(layout),
        static_cast<uint32_t>(payload.mesh.indices.size()));
    return true;
}

}  // namespace engine
