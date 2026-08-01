#pragma once

#include <optional>

#include "combat/ActorId.h"
#include "combat/CombatState.h"
#include "combat/Intent.h"

namespace bb {

/// Decides what an enemy announces it will do next.
///
/// Not a search. Each enemy walks a short ordered list of abilities and
/// telegraphs the first one whose conditions hold, falling back to closing
/// distance. Predictability is a feature, not a limitation: the player is meant
/// to look at the board, work out what is about to happen, and step out of the
/// way. An opponent that outsmarts you cannot be dodged.
namespace ai {

/// Picks the intent this enemy will telegraph.
Intent ChooseIntent(const CombatState& state, ActorId enemy);

/// The living player unit nearest to `from`, or kNoActor if none is left.
ActorId NearestPlayer(const CombatState& state, Vec2 from);

}  // namespace ai
}  // namespace bb
