#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cards/CardDef.h"
#include "core/Rng.h"

namespace bb {

/// What picking an option does.
enum class EventEffect : std::uint8_t {
    Nothing,
    Heal,        ///< Restore `amount` health.
    LoseHealth,  ///< Pay `amount` health.
    GainGold,
    LoseGold,
    GainCard,    ///< A specific card joins the deck.
};

struct EventOption {
    std::string label;
    EventEffect effect = EventEffect::Nothing;
    int amount = 0;
    CardId card = CardId::Jab;

    /// Shown after choosing, so the outcome is narrated rather than silently
    /// applied.
    std::string result;
};

struct EventDef {
    std::string title;
    std::string body;
    std::vector<EventOption> options;
};

/// Every event in the game.
const std::vector<EventDef>& AllEvents();

/// Picks one at random.
const EventDef& RollEvent(Rng& rng);

}  // namespace bb
