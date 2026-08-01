#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "combat/ActorId.h"
#include "combat/Terrain.h"
#include "core/Grid.h"
#include "core/Vec2.h"

namespace bb {

/// The fight's terrain, plus who is standing where.
///
/// Battlefield is the only owner of actor positions -- Actor stores no
/// coordinate. Position and occupancy are two views of the same fact, and both
/// are updated together behind this interface, so they cannot drift apart no
/// matter what a caller does.
///
/// Reads outside the grid are answered rather than rejected: TerrainAt returns
/// Wall and OccupantAt returns kNoActor. Card shapes and pushes routinely probe
/// past the border, and making every one of those sites bounds-check first is
/// how off-by-one bugs get in.
class Battlefield {
public:
    Battlefield(int width, int height);

    int Width() const noexcept { return terrain_.Width(); }
    int Height() const noexcept { return terrain_.Height(); }
    bool Contains(Vec2 p) const noexcept { return terrain_.Contains(p); }

    /// Off-grid reads as Wall -- see the class comment.
    Terrain TerrainAt(Vec2 p) const;
    void SetTerrain(Vec2 p, Terrain terrain);

    /// Who stands on p, or kNoActor if the tile is empty or off-grid.
    ActorId OccupantAt(Vec2 p) const;
    bool IsOccupied(Vec2 p) const { return IsValid(OccupantAt(p)); }

    /// Where an actor is, or nullopt if it is not on the field -- either never
    /// placed, or removed when it died.
    std::optional<Vec2> PositionOf(ActorId id) const;

    /// A tile an actor could legally stand on: on the grid, not a wall, and
    /// unoccupied. Hazards are walkable -- stepping into fire is permitted, just
    /// a bad idea, and that choice is the player's to make.
    bool IsWalkable(Vec2 p) const;

    /// Puts an actor on the field. The destination must be walkable and the
    /// actor must not already be placed.
    void Place(ActorId id, Vec2 destination);

    /// Moves an already-placed actor. Returns false and changes nothing if the
    /// actor is not on the field or the destination is not walkable, so a
    /// rejected move is always a no-op rather than a half-applied one.
    bool MoveTo(ActorId id, Vec2 destination);

    /// Takes an actor off the field, e.g. on death. Safe to call for an actor
    /// that was never placed.
    void Remove(ActorId id);

    /// Walkable tiles orthogonally adjacent to p, in fixed clockwise-from-north
    /// order. The primitive the free step and, later, pathfinding build on.
    std::vector<Vec2> WalkableNeighbours(Vec2 p) const;

    /// Whether a straight line from `from` to `to` is unobstructed.
    ///
    /// Only tiles strictly *between* the endpoints can block. Aiming at a wall
    /// itself is allowed, and standing next to one does not blind you. Actors do
    /// not block sight -- only terrain does, so a target behind an ally is still
    /// shootable and the rule stays something a player can eyeball.
    ///
    /// Symmetric by construction: if A can see B, B can see A.
    bool HasLineOfSight(Vec2 from, Vec2 to) const;

private:
    Grid<Terrain> terrain_;
    Grid<ActorId> occupancy_;
    std::unordered_map<ActorId, Vec2> positions_;
};

}  // namespace bb
