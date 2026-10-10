#pragma once

#include <cstdint>
#include <limits>

namespace engine {

/// Address of one record inside an asset table. The generation makes a handle that was
/// handed out before a failed load was retried resolve to "stale" instead of to the new
/// asset that took the same slot.
template <typename T>
struct AssetHandle {
    uint32_t slot = std::numeric_limits<uint32_t>::max();
    uint32_t generation = 0;

    AssetHandle() = default;
    AssetHandle(const uint32_t slot_, const uint32_t generation_)
        : slot(slot_), generation(generation_) {}

    explicit operator bool() const noexcept {
        return slot != std::numeric_limits<uint32_t>::max();
    }

    bool operator==(const AssetHandle&) const = default;

    static AssetHandle Invalid() {
        return {};
    }
};

}  // namespace engine
