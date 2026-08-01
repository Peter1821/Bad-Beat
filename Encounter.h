#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "cards/CardDef.h"
#include "combat/CombatState.h"

namespace bb {

/// How hard a fight should be, taken from the map node that spawned it.
enum class EncounterTier {
    Normal,
    Elite,
    Boss,
};

/// Builds a fight.
///
/// The roster grows with `depth` -- how far up the map the player has climbed --
/// and with the tier. Arena layouts are still hand-authored; a table of them
/// arrives in milestone 8.
///
/// `starting_hp` is the player's carried-over health, so a run gets harder as
/// much through attrition as through bigger rosters.
///
/// Returns a unique_ptr because CombatState is deliberately immovable -- see the
/// comment on that class.
std::unique_ptr<CombatState> MakeEncounter(std::uint64_t seed,
                                           int depth,
                                           EncounterTier tier,
                                           int starting_hp,
                                           std::vector<CardId> deck_list);

}  // namespace bb
