#include "combat/ArenaGenerator.h"

#include <algorithm>
#include <queue>

#include "core/Grid.h"

namespace bb {
namespace {

int OpenTileCount(const Battlefield& field) {
    int open = 0;
    for (int y = 0; y < field.Height(); ++y) {
        for (int x = 0; x < field.Width(); ++x) {
            if (!BlocksMovement(field.TerrainAt({x, y}))) ++open;
        }
    }
    return open;
}

/// Floor tiles reachable from `from`, ignoring who is standing where -- nothing
/// is placed yet when this runs.
int ReachableCount(const Battlefield& field, Vec2 from) {
    Grid<bool> seen(field.Width(), field.Height(), false);
    std::queue<Vec2> frontier;

    seen.At(from) = true;
    frontier.push(from);
    int count = 1;

    while (!frontier.empty()) {
        const Vec2 current = frontier.front();
        frontier.pop();

        for (const Vec2 step : kOrthogonalSteps) {
            const Vec2 next = current + step;
            if (!field.Contains(next)) continue;
            if (seen.At(next)) continue;
            if (BlocksMovement(field.TerrainAt(next))) continue;

            seen.At(next) = true;
            ++count;
            frontier.push(next);
        }
    }
    return count;
}

bool BoardIsWhole(const Battlefield& field, Vec2 from) {
    return ReachableCount(field, from) == OpenTileCount(field);
}

Vec2 RandomTile(const Battlefield& field, Rng& rng) {
    return {rng.Range(0, field.Width() - 1), rng.Range(0, field.Height() - 1)};
}

/// Walls and hazards both grow with depth, but slowly. An 8x6 board turns into
/// a corridor faster than it looks like it will.
int WallBudget(int depth) { return std::min(8, 4 + depth / 2); }
int HazardBudget(int depth) { return std::min(3, 1 + depth / 3); }

}  // namespace

ArenaLayout GenerateArena(Battlefield& field, Rng& rng, int depth, int enemy_count) {
    for (int y = 0; y < field.Height(); ++y) {
        for (int x = 0; x < field.Width(); ++x) field.SetTerrain({x, y}, Terrain::Floor);
    }

    ArenaLayout layout;

    // The player always enters from the left, so the board reads the same way
    // every time and enemies come from the right -- but never from the same tile
    // twice.
    const int left_edge = std::max(1, field.Width() / 3);
    layout.player_start = {rng.Range(0, left_edge - 1), rng.Range(0, field.Height() - 1)};

    // --- walls -------------------------------------------------------------
    const int walls = WallBudget(depth);
    for (int placed = 0, attempts = 0; placed < walls && attempts < 400; ++attempts) {
        const Vec2 tile = RandomTile(field, rng);

        if (field.TerrainAt(tile) != Terrain::Floor) continue;

        // Leave the player room to breathe. Spawning already boxed in is not a
        // hard fight, it is a lost one.
        if (ManhattanDistance(tile, layout.player_start) <= 1) continue;

        field.SetTerrain(tile, Terrain::Wall);
        if (!BoardIsWhole(field, layout.player_start)) {
            field.SetTerrain(tile, Terrain::Floor);
            continue;
        }
        ++placed;
    }

    // --- hazards -----------------------------------------------------------
    // Walkable, so they can never disconnect anything and need no validation.
    const int hazards = HazardBudget(depth);
    for (int placed = 0, attempts = 0; placed < hazards && attempts < 200; ++attempts) {
        const Vec2 tile = RandomTile(field, rng);

        if (field.TerrainAt(tile) != Terrain::Floor) continue;
        if (ManhattanDistance(tile, layout.player_start) <= 1) continue;

        field.SetTerrain(tile, Terrain::Hazard);
        ++placed;
    }

    if (enemy_count <= 0) return layout;

    // --- the opener --------------------------------------------------------
    // One enemy always starts in reach, so turn one opens with a marked tile.
    // A fight that begins quiet teaches the player the shading is optional.
    std::vector<Vec2> adjacent;
    for (const Vec2 step : kOrthogonalSteps) {
        const Vec2 tile = layout.player_start + step;
        if (field.Contains(tile) && field.TerrainAt(tile) == Terrain::Floor) {
            adjacent.push_back(tile);
        }
    }
    if (!adjacent.empty()) layout.enemy_spawns.push_back(rng.Pick(adjacent));

    // --- everyone else -----------------------------------------------------
    const auto far_enough = [&](Vec2 tile) {
        if (ManhattanDistance(tile, layout.player_start) < 3) return false;
        for (const Vec2 taken : layout.enemy_spawns) {
            if (ManhattanDistance(tile, taken) < 2) return false;
        }
        return true;
    };

    for (int attempts = 0;
         static_cast<int>(layout.enemy_spawns.size()) < enemy_count && attempts < 400;
         ++attempts) {
        const Vec2 tile = RandomTile(field, rng);

        if (field.TerrainAt(tile) != Terrain::Floor) continue;
        if (tile == layout.player_start) continue;
        if (!far_enough(tile)) continue;

        layout.enemy_spawns.push_back(tile);
    }

    // A busy board can leave nowhere that satisfies the spacing rule. Rather
    // than spawn fewer enemies than the difficulty curve called for, relax the
    // spacing and take any free tile.
    for (int attempts = 0;
         static_cast<int>(layout.enemy_spawns.size()) < enemy_count && attempts < 400;
         ++attempts) {
        const Vec2 tile = RandomTile(field, rng);

        if (field.TerrainAt(tile) != Terrain::Floor) continue;
        if (tile == layout.player_start) continue;
        if (std::find(layout.enemy_spawns.begin(), layout.enemy_spawns.end(), tile) !=
            layout.enemy_spawns.end()) {
            continue;
        }
        layout.enemy_spawns.push_back(tile);
    }

    return layout;
}

}  // namespace bb
