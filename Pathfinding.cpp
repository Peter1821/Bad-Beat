#include "combat/Pathfinding.h"

#include <queue>

namespace bb {
namespace {

/// Whether a route may cross this tile. Distinct from Battlefield::IsWalkable,
/// which asks whether an actor may *stop* here.
bool CanRouteThrough(const CombatState& state, Vec2 tile) {
    const Battlefield& field = state.Field();
    if (!field.Contains(tile)) return false;
    if (BlocksMovement(field.TerrainAt(tile))) return false;

    // Player units body-block; enemies do not block each other.
    if (const Actor* occupant = state.ActorAt(tile)) {
        if (occupant->GetTeam() == Team::Player) return false;
    }
    return true;
}

}  // namespace

Grid<int> DistanceField(const CombatState& state, Vec2 origin) {
    const Battlefield& field = state.Field();
    Grid<int> distance(field.Width(), field.Height(), -1);

    if (!field.Contains(origin)) return distance;

    // The origin is seeded even if it is not routable -- it is where the target
    // is standing, and every path has to be measured from somewhere.
    distance.At(origin) = 0;

    std::queue<Vec2> frontier;
    frontier.push(origin);

    while (!frontier.empty()) {
        const Vec2 current = frontier.front();
        frontier.pop();

        for (const Vec2 step : kOrthogonalSteps) {
            const Vec2 next = current + step;
            if (!field.Contains(next)) continue;
            if (distance.At(next) != -1) continue;
            if (!CanRouteThrough(state, next)) continue;

            distance.At(next) = distance.At(current) + 1;
            frontier.push(next);
        }
    }

    return distance;
}

std::optional<Vec2> StepAwayFrom(const CombatState& state, Vec2 from, Vec2 target) {
    if (from == target) return std::nullopt;

    const Grid<int> distance = DistanceField(state, target);
    const int here = distance.Contains(from) ? distance.At(from) : -1;
    if (here < 0) return std::nullopt;

    std::optional<Vec2> best;
    int best_distance = here;

    for (const Vec2 step : kOrthogonalSteps) {
        const Vec2 candidate = from + step;
        if (!state.Field().IsWalkable(candidate)) continue;

        const int candidate_distance = distance.At(candidate);
        if (candidate_distance < 0) continue;

        if (candidate_distance > best_distance) {
            best_distance = candidate_distance;
            best = candidate;
        }
    }

    return best;
}

std::optional<Vec2> StepToward(const CombatState& state, Vec2 from, Vec2 target) {
    if (from == target) return std::nullopt;

    const Grid<int> distance = DistanceField(state, target);

    const int here = distance.Contains(from) ? distance.At(from) : -1;

    std::optional<Vec2> best;
    int best_distance = here >= 0 ? here : -1;

    for (const Vec2 step : kOrthogonalSteps) {
        const Vec2 candidate = from + step;

        // Route may pass through allies, but a turn may not *end* on one.
        if (!state.Field().IsWalkable(candidate)) continue;

        const int candidate_distance = distance.At(candidate);
        if (candidate_distance < 0) continue;

        // Strictly better only, so a boxed-in enemy stands still rather than
        // shuffling between two equally distant tiles forever.
        if (best_distance < 0 || candidate_distance < best_distance) {
            best_distance = candidate_distance;
            best = candidate;
        }
    }

    return best;
}

}  // namespace bb
