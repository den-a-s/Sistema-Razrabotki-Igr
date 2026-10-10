#pragma once

#include <engine/RenderedMesh.h>
#include <engine/ShaderContext.hpp>
#include <engine/Texture.hpp>
#include <engine/assets/AssetCommon.hpp>

namespace engine {

/// Thin records: they name GPU resources owned by RenderingResourceManager, they do not
/// hold D3D objects or free anything. The handles they carry stay valid for the process.

struct MeshAsset {
    MeshHandle mesh;
};

struct TextureAsset {
    TextureHandle texture;
};

/// One .hlsl file, compiled for the stages it is asked for. A stage handle is empty when
/// the file was not loaded for that stage.
struct ShaderAsset {
    ShaderHandle vertex;
    ShaderHandle pixel;
    ShaderHandle geometry;
};

/// Which table each asset type lives in; used by the templated lookup helpers.
template <typename T>
struct AssetValueKind;

template <>
struct AssetValueKind<MeshAsset> {
    static constexpr AssetKind value = AssetKind::Mesh;
};

template <>
struct AssetValueKind<TextureAsset> {
    static constexpr AssetKind value = AssetKind::Texture;
};

template <>
struct AssetValueKind<ShaderAsset> {
    static constexpr AssetKind value = AssetKind::Shader;
};

}  // namespace engine
