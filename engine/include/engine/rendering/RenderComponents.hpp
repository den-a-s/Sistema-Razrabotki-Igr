#pragma once

#include <engine/assets/AssetHandle.hpp>
#include <engine/assets/AssetTypes.hpp>
#include <engine/rendering/RenderWorld.hpp>

// Scene components the renderer understands. SceneRenderSubsystem copies them into
// the RenderWorld every frame.

namespace engine {

/// Draws a mesh asset with the entity's world matrix. Assets come from
/// Engine::GetAssets(); until they are loaded the entity is simply not drawn.
struct MeshRenderer {
    AssetHandle<MeshAsset> mesh;
    /// Empty = plain white (BuiltinAssets::WhiteTexture).
    AssetHandle<TextureAsset> texture{};
    MaterialParameters material{DirectX::SimpleMath::Vector3{0.25f, 0.25f, 0.25f}, 32.0f, 0};
};

/// Renders the scene from the entity's world position, looking along its local -Z
/// (Vector3::Forward); roll is ignored. The first entity with a Camera is the active one.
/// The aspect ratio follows the engine's surface.
struct Camera {
    float fovY = 1.04719755f;  // 60 degrees
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};

/// Sun-like light shining along the entity's local -Z (Vector3::Forward); its position
/// doesn't matter. Only the first one is used, the renderer supports a single one.
struct DirectionalLight {
    DirectX::SimpleMath::Vector3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

/// Asset names of the primitives shipped in engine/primitives (copied next to the executable).
namespace BuiltinAssets {
inline constexpr const char* CubeMesh = "engine/primitives/cube";
inline constexpr const char* WhiteTexture = "engine/primitives/white";
}  // namespace BuiltinAssets

}  // namespace engine
