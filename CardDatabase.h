#pragma once

#include <string>
#include <vector>

#include "cards/CardDef.h"

namespace bb {

/// Every card in the game.
///
/// Currently a hardcoded table. The design doc calls for JSON, and the shape
/// here is built for it -- CardDef is a plain aggregate with a text id, and
/// everything goes through these two functions -- but a loader would not change
/// anything visible about milestone 3, so it is deliberately deferred rather
/// than bundled in.
const CardDef& GetCard(CardId id);

/// The fixed 10-card starting deck. No draft at run start: opening from a known
/// position makes the *map* the first real decision, and makes a card pickup
/// legible against a stable baseline.
std::vector<CardId> StartingDeck();

/// Cards that can turn up as a reward or in a shop. Excludes the starting
/// four -- finding a copy of what you already have four of is not a reward.
const std::vector<CardId>& RewardPool();

/// What a card costs in a shop, by rarity.
int ShopPrice(const CardDef& card);

/// Human-readable rules text, generated from the card's pattern and effects so
/// it cannot drift from behaviour.
std::string DescribeCard(const CardDef& card);

/// Compact shape tag for the card face, e.g. "cone 2" or "range 4".
std::string DescribeShape(const TargetPattern& pattern);

}  // namespace bb
