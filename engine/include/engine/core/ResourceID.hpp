#pragma once
#include <string>
#include <string_view>

#include <entt/core/hashed_string.hpp>

namespace engine {

// Fast key for resource names, used by the asset tables and the shader context.
//
// The hash is EnTT's own: FNV-1a over the name as is, at the width of entt::id_type, which
// stays 32 bits here because ENTT_ID_TYPE is not set. So the odds of a collision are exactly
// those of the hand-written FNV-32 this replaced; the id is shared with EnTT, not widened.
//
// Collisions are therefore still not resolved by re-hashing: every asset table also stores
// the normalized name it was created with, so a collision between two different names is
// detected and reported instead of silently aliasing one resource onto another.

using ResourceID = entt::hashed_string::hash_type;

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

/// ResourceID of a name; pairs with entt::hashed_string because both hash the given string as is.
constexpr ResourceID ToResourceID(const std::string_view name) {
    return entt::hashed_string::value(name.data(), name.size());
}

}
