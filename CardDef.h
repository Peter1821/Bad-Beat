#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cards/EffectOp.h"
#include "cards/TargetPattern.h"

namespace bb {

/// Handle for a card definition.
///
/// An enum rather than a string keeps decks cheap to copy and comparisons
/// trivial. CardDef carries a matching text id so a JSON-backed database can be
/// dropped in later without changing anything that holds a CardId.
enum class CardId : std::uint16_t {
    // Starting deck.
    Jab,
    Fold,
    Sidestep,
    Sweep,

    // Found during a run.
    LongShot,
    SplitPot,
    Shove,
    Check,
    Backhand,
    ChipUp,
    Cooler,
    Raise,
    PocketAces,
    TheNuts,

    Count,  ///< Sentinel; never a real card.
};

enum class Rarity : std::uint8_t {
    Starter,
    Common,
    Uncommon,
    Rare,
};

/// An immutable card definition. Instances in a deck are just CardIds; nothing
/// about a card changes at runtime except which pile it is sitting in.
struct CardDef {
    CardId id = CardId::Jab;
    std::string text_id;  ///< Stable key for future serialisation.
    std::string name;
    int cost = 1;
    Rarity rarity = Rarity::Common;
    TargetPattern pattern;
    std::vector<EffectOp> effects;

    /// One-line flavour. The mechanical description is generated from `effects`
    /// so it can never drift from what the card actually does.
    std::string flavour;

    /// Removed from the deck for the rest of the fight once played.
    ///
    /// Reserved for the rare and powerful, so spending one is a real decision
    /// rather than a tax. A deck full of exhaust cards is a deck that runs out.
    ///
    /// Deliberately last in the struct: every card is built with aggregate
    /// initialisation, so a field inserted in the middle silently shifts every
    /// value after it.
    bool exhaust = false;
};

}  // namespace bb
