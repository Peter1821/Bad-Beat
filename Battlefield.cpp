#include "combat/Battlefield.h"

#include <cassert>

#include "core/Line.h"

namespace bb {

Battlefield::Battlefield(int width, int height)
    : terrain_(width, height, Terrain::Floor), occupancy_(width, height, kNoActor) {}

Terrain Battlefield::TerrainAt(Vec2 p) const {
    const Terrain* found = terrain_.TryAt(p);
    return found ? *found : Terrain::Wall;
}

void Battlefield::SetTerrain(Vec2 p, Terrain terrain) {
    assert(Contains(p));
    terrain_.At(p) = terrain;
}

ActorId Battlefield::OccupantAt(Vec2 p) const {
    const ActorId* found = occupancy_.TryAt(p);
    return found ? *found : kNoActor;
}

std::optional<Vec2> Battlefield::PositionOf(ActorId id) const {
    const auto found = positions_.find(id);
    if (found == positions_.end()) return std::nullopt;
    return found->second;
}

bool Battlefield::IsWalkable(Vec2 p) const {
    if (!Contains(p)) return false;
    if (BlocksMovement(TerrainAt(p))) return false;
    return !IsOccupied(p);
}

void Battlefield::Place(ActorId id, Vec2 destination) {
    assert(IsValid(id));
    assert(IsWalkable(destination) && "caller must check IsWalkable before placing");
    assert(positions_.find(id) == positions_.end() && "actor is already on the field");

    occupancy_.At(destination) = id;
    positions_.emplace(id, destination);
}

bool Battlefield::MoveTo(ActorId id, Vec2 destination) {
    const auto found = positions_.find(id);
    if (found == positions_.end()) return false;

    // Moving onto your own tile is a no-op, and has to be handled before the
    // walkability check: an actor always occupies its own square, so that check
    // would otherwise reject it as "tile taken".
    if (found->second == destination) return true;

    if (!IsWalkable(destination)) return false;

    occupancy_.At(found->second) = kNoActor;
    occupancy_.At(destination) = id;
    found->second = destination;
    return true;
}

void Battlefield::Remove(ActorId id) {
    const auto found = positions_.find(id);
    if (found == positions_.end()) return;

    occupancy_.At(found->second) = kNoActor;
    positions_.erase(found);
}

bool Battlefield::HasLineOfSight(Vec2 from, Vec2 to) const {
    const std::vector<Vec2> tiles = LineTiles(from, to);

    // Skip both endpoints: the shooter's own tile obviously does not block, and
    // the target tile is allowed to be a wall (you can aim *at* cover).
    for (std::size_t i = 1; i + 1 < tiles.size(); ++i) {
        if (BlocksLineOfSight(TerrainAt(tiles[i]))) return false;
    }
    return true;
}

std::vector<Vec2> Battlefield::WalkableNeighbours(Vec2 p) const {
    std::vector<Vec2> result;
    result.reserve(kOrthogonalSteps.size());
    for (const Vec2 step : kOrthogonalSteps) {
        const Vec2 neighbour = p + step;
        if (IsWalkable(neighbour)) result.push_back(neighbour);
    }
    return result;
}

}  // namespace bb
