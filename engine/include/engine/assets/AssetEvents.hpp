#pragma once

#include <string>

#include <engine/ResourceID.hpp>
#include <engine/assets/AssetCommon.hpp>

// Events published by AssetSystem on Engine::GetEvents() once a load attempt has settled.

namespace engine {

struct AssetLoadedEvent {
    ResourceID id = 0;
    std::string name;
    AssetKind kind = AssetKind::Mesh;
};

struct AssetFailedEvent {
    ResourceID id = 0;
    std::string name;
    AssetKind kind = AssetKind::Mesh;
    std::string error;
};

}  // namespace engine
