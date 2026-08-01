#pragma once

#include <vector>

#include "combat/Battlefield.h"
#include "core/Rng.h"
#include "core/Vec2.h"

namespace bb {

/// Where everyone stands when a fight begins.
struct ArenaLayout {
    Vec2 player_start;

    /// Spawn tiles, in roster order. The first is deliberately within striking
    /// distance of the player, so every fight opens with a tile already marked.
    std::vector<Vec2> enemy_spawns;
};

/// Fills a battlefield with terrain and picks spawn tiles.
///
/// Walls go down one at a time and are taken back up if they would cut the board
/// in two, so a generated arena is always fully connected. An enemy sealed
/// behind terrain it cannot path around does not make a fight harder -- it makes
/// a fight that never ends.
///
/// `depth` is how far up the map the run has climbed; boards get busier as it
/// grows.
ArenaLayout GenerateArena(Battlefield& field, Rng& rng, int depth, int enemy_count);

}  // namespace bb
