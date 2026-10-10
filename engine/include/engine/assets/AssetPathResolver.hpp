#pragma once

#include <filesystem>
#include <string_view>

#include <engine/assets/AssetCommon.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace engine {

/// Maps a logical asset name to a file on disk.
///
/// The root is the directory of the executable, never the current working directory:
/// double-clicking an exe or launching it from another tool would otherwise change which
/// assets are found.
class AssetPathResolver {
public:
    /// Mesh assets address a folder that holds model.obj (and optionally texture.png);
    /// a name already ending in .obj addresses that file directly.
    [[nodiscard]] static std::filesystem::path ResolveMesh(const AssetDesc& desc) {
        std::filesystem::path path = MakeAbsolute(desc.path);
        if (HasExtension(path, L".obj")) {
            return path;
        }
        return path / L"model.obj";
    }

    /// A name ending in a known image extension addresses that file; anything else is a
    /// folder holding texture.png, which is what the legacy model loader used.
    [[nodiscard]] static std::filesystem::path ResolveTexture(const AssetDesc& desc) {
        std::filesystem::path path = MakeAbsolute(desc.path);
        if (IsImageFile(path)) {
            return path;
        }
        return path / L"texture.png";
    }

    /// "shaders/LightPass" -> <root>/engine/shaders/LightPass.hlsl (extension may be given).
    [[nodiscard]] static std::filesystem::path ResolveShader(const AssetDesc& desc) {
        std::filesystem::path path = MakeAbsolute(desc.path);
        if (!HasExtension(path, L".hlsl")) {
            path += L".hlsl";
        }
        return path;
    }

    /// Default on-disk location for a kind when the caller gave only a logical name.
    [[nodiscard]] static std::filesystem::path DefaultPath(std::string_view name, AssetKind kind) {
        if (kind == AssetKind::Shader) {
            constexpr std::string_view prefix = "shaders/";
            if (name.starts_with(prefix)) {
                name.remove_prefix(prefix.size());
            }
            return std::filesystem::path("engine") / "shaders" / std::filesystem::path(name);
        }
        return std::filesystem::path(name);
    }

    [[nodiscard]] static const std::filesystem::path& GetRoot() {
        return Root();
    }

private:
    [[nodiscard]] static const std::filesystem::path& Root() {
        static const std::filesystem::path root = ExecutableDirectory();
        return root;
    }

    [[nodiscard]] static std::filesystem::path ExecutableDirectory() {
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH];
        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length > 0 && length < MAX_PATH) {
            return std::filesystem::path(buffer).parent_path();
        }
#endif
        return std::filesystem::current_path();
    }

    [[nodiscard]] static std::filesystem::path MakeAbsolute(const std::filesystem::path& path) {
        if (path.is_absolute()) {
            return path;
        }
        return Root() / path;
    }

    [[nodiscard]] static bool HasExtension(const std::filesystem::path& path, const wchar_t* extension) {
        const std::filesystem::path ext = path.extension();
        if (ext.empty()) {
            return false;
        }
        return _wcsicmp(ext.c_str(), extension) == 0;
    }

    [[nodiscard]] static bool IsImageFile(const std::filesystem::path& path) {
        return HasExtension(path, L".png") || HasExtension(path, L".jpg") ||
               HasExtension(path, L".jpeg") || HasExtension(path, L".tif") ||
               HasExtension(path, L".tiff") || HasExtension(path, L".dds") ||
               HasExtension(path, L".bmp");
    }
};

}  // namespace engine
