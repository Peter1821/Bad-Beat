#include "combat/EnemyAI.h"

#include <algorithm>

#include "cards/Targeting.h"
#include "combat/EnemyDatabase.h"
#include "combat/Pathfinding.h"

namespace bb {
namespace ai {

ActorId NearestPlayer(const CombatState& state, Vec2 from) {
    ActorId best = kNoActor;
    int best_distance = 0;

    for (const Actor& actor : state.Actors()) {
        if (actor.GetTeam() != Team::Player || !actor.IsAlive()) continue;

        const auto tile = state.Field().PositionOf(actor.Id());
        if (!tile) continue;

        const int distance = ManhattanDistance(from, *tile);
        if (!IsValid(best) || distance < best_distance) {
            best = actor.Id();
            best_distance = distance;
        }
    }

    return best;
}

Intent ChooseIntent(const CombatState& state, ActorId enemy) {
    Intent intent;

    const Actor* self = state.FindActor(enemy);
    if (self == nullptr || !self->IsAlive()) return intent;

    const auto self_tile = state.Field().PositionOf(enemy);
    if (!self_tile) return intent;

    const ActorId target = NearestPlayer(state, *self_tile);
    if (!IsValid(target)) return intent;

    const auto target_tile = state.Field().PositionOf(target);
    if (!target_tile) return intent;

    const int distance = ManhattanDistance(*self_tile, *target_tile);
    const EnemyDef& def = GetEnemy(self->GetArchetype());

    for (const EnemyAbility& ability : def.abilities) {
        if (distance < ability.min_range || distance > ability.max_range) continue;

        // Same legality check a player card goes through, so an enemy can no
        // more shoot through a wall than you can.
        if (!IsLegalAim(ability.pattern, *self_tile, *target_tile, state.Field())) continue;

        intent.kind = ability.kind;
        intent.amount = ability.amount;
        intent.threatened =
            ComputeAffectedTiles(ability.pattern, *self_tile, *target_tile, state.Field());

        // An attack that would land on nothing is not worth telegraphing; fall
        // through and let the enemy reposition instead.
        if (intent.threatened.empty()) continue;

        return intent;
    }

    // Too close to use anything it owns? Back off rather than loiter. Without
    // this a ranged enemy that gets rushed just stands adjacent doing nothing,
    // which reads as broken however defensible it is.
    const bool crowded = std::any_of(
        def.abilities.begin(), def.abilities.end(),
        [distance](const EnemyAbility& ability) { return distance < ability.min_range; });

    if (crowded && StepAwayFrom(state, *self_tile, *target_tile).has_value()) {
        intent.kind = IntentKind::Retreat;
        return intent;
    }

    // Otherwise close the distance. Movement threatens no tiles, so it re-paths
    // when it runs rather than committing now.
    if (StepToward(state, *self_tile, *target_tile).has_value()) {
        intent.kind = IntentKind::Move;
        return intent;
    }

    intent.kind = IntentKind::Wait;
    return intent;
}

}  // namespace ai
}  // namespace bb
