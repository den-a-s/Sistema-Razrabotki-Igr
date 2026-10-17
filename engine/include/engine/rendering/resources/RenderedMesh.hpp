#pragma once
#include <engine/rendering/resources/Buffer.hpp>
#include <engine/rendering/resources/VertexLayout.hpp>


namespace engine {

struct RenderedMesh {
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    MeshLayoutHandle meshLayout;
    uint32_t indexCount;
};

using MeshHandle = HandleMap<RenderedMesh>::Handle;
}

