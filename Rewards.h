#pragma once

#include <vector>

#include "cards/CardDef.h"
#include "combat/Encounter.h"
#include "core/Rng.h"

namespace bb {

/// Gold paid out for clearing a fight of a given tier.
int GoldFor(EncounterTier tier, Rng& rng);

/// Rolls the card choices offered after a fight.
///
/// Rarity is weighted rather than uniform, and the picks are distinct -- being
/// offered the same card three times is not a choice. Elites and the boss skew
/// the roll upward, which is most of what makes taking the harder branch worth
/// the health it costs.
///
/// `pool` is the set of cards currently unlocked. Passing it in rather than
/// reading the global card list keeps this a pure function of its inputs, which
/// is what lets the tests pin the weighting down.
std::vector<CardId> RollCardReward(Rng& rng,
                                   const std::vector<CardId>& pool,
                                   EncounterTier tier,
                                   int count);

/// Rolls a shop's stock. Distinct cards, no rarity bias -- a shop is a place to
/// spend gold on what you need, not another lottery.
std::vector<CardId> RollShopStock(Rng& rng, const std::vector<CardId>& pool, int count);

/// What removing a card from the deck costs.
int CardRemovalPrice();

}  // namespace bb
