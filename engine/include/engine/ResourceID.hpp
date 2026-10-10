#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace engine {

//Simple string hash for resource names, used as the fast key for assets and shaders.
//
//Collisions are not resolved by re-hashing: every asset table also stores the normalized
//name it was created with, so a collision between two different names is detected and
//reported instead of silently aliasing one resource onto another.

constexpr std::uint32_t FNV_PRIME = 16777619u;
constexpr std::uint32_t FNV_OFFSET_BASIS = 2166136261u;

constexpr std::uint32_t fnv1a_32(const std::string_view str) noexcept {
    std::uint32_t hash = FNV_OFFSET_BASIS;

    for (const char c : str) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= FNV_PRIME;
    }

    return hash;
}

using ResourceID = std::uint32_t;

constexpr ResourceID operator""_rid(
    const char* str,
    const size_t len) {
    return fnv1a_32(std::string_view(str, len));
}

/// Names are compared case insensitively and with '/' separators, so "Models/Table" and
/// "models/table" address the same resource. An empty result means "not a usable name".
constexpr std::string NormalizeName(const std::string_view name) {
    std::string result(name);
    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        } else if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

/// ResourceID of a name; pairs with operator""_rid because both hash the given string as is.
constexpr ResourceID ToResourceID(const std::string_view name) {
    return fnv1a_32(name);
}

}
