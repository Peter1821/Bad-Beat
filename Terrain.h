#pragma once

#include <cstdint>

namespace bb {

/// What a tile is made of.
///
/// Terrain is fixed for the duration of a fight: it is generated with the
/// encounter and never changes. Anything that moves or can be destroyed is an
/// Actor, not terrain.
enum class Terrain : std::uint8_t {
    Floor,   ///< Ordinary walkable ground.
    Wall,    ///< Blocks movement, and line of sight once ranged cards exist.
    Hazard,  ///< Walkable but harmful -- standing here costs HP.
};

constexpr bool BlocksMovement(Terrain terrain) noexcept { return terrain == Terrain::Wall; }

constexpr bool BlocksLineOfSight(Terrain terrain) noexcept { return terrain == Terrain::Wall; }

}  // namespace bb
