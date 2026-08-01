#pragma once

#include <optional>

#include "combat/CombatState.h"
#include "core/Grid.h"
#include "core/Vec2.h"

namespace bb {

/// Step distance from `origin` to every reachable tile, ignoring diagonals.
/// Unreachable tiles hold -1.
///
/// Enemies may route *through* one another but never through a player unit, so
/// a formation cannot be walked past but a crowd of rats does not gridlock
/// itself. Walls block, hazards do not -- an enemy will happily path through
/// fire, which is a decision it can regret.
Grid<int> DistanceField(const CombatState& state, Vec2 origin);

/// One orthogonal step from `from` that gets closest to `target`.
///
/// Returns nullopt when nothing adjacent improves the distance, which covers
/// both "already there" and "boxed in". The step must land on a genuinely empty
/// tile even though the route may pass through allies -- two enemies never
/// finish a turn stacked.
///
/// Ties break in fixed clockwise-from-north order, so the same board always
/// produces the same move and a replayed seed stays honest.
std::optional<Vec2> StepToward(const CombatState& state, Vec2 from, Vec2 target);

/// One orthogonal step from `from` that puts more ground between it and
/// `target`. Returns nullopt when nothing adjacent is further away.
///
/// Lets a ranged enemy back out of a melee it cannot use its weapon in, rather
/// than standing next to the player doing nothing -- which looks like broken AI
/// even when it is technically a correct decision.
std::optional<Vec2> StepAwayFrom(const CombatState& state, Vec2 from, Vec2 target);

}  // namespace bb
