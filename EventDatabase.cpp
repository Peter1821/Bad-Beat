#include "run/EventDatabase.h"

namespace bb {
namespace {

std::vector<EventDef> BuildEvents() {
    std::vector<EventDef> events;

    // Every event offers a trade with a visible cost. An option that is strictly
    // better than the others is not a choice, so each one gives up something.

    events.push_back({
        "The Side Table",
        "A quiet game in the corner. The players do not look up, but a chair is "
        "free.",
        {
            {"Sit in for a hand", EventEffect::GainGold, 60, CardId::Jab,
             "You read them right. 60 gold."},
            {"Watch, and learn", EventEffect::GainCard, 0, CardId::Check,
             "You pick up a tell. Check joins your deck."},
            {"Walk away", EventEffect::Nothing, 0, CardId::Jab, "The chair stays empty."},
        },
    });

    events.push_back({
        "The House Doctor",
        "Someone who patches people up between hands, for a price.",
        {
            {"Pay for stitches", EventEffect::Heal, 12, CardId::Jab,
             "Cleaner than you expected. 12 health back."},
            {"Ask for the strong stuff", EventEffect::GainCard, 0, CardId::Raise,
             "Your hands stop shaking. Raise joins your deck."},
            {"Refuse", EventEffect::Nothing, 0, CardId::Jab, "You have had worse."},
        },
    });

    events.push_back({
        "A Marked Deck",
        "Someone left it behind. It is obviously crooked, and obviously useful.",
        {
            {"Take it", EventEffect::GainCard, 0, CardId::Cooler,
             "Cooler joins your deck. Someone will notice eventually."},
            {"Sell it on", EventEffect::GainGold, 55, CardId::Jab, "55 gold, no questions."},
            {"Burn it", EventEffect::Heal, 6, CardId::Jab,
             "A weight off. 6 health back."},
        },
    });

    events.push_back({
        "The Rail",
        "A crowd watching from the rail. One of them is watching you specifically.",
        {
            {"Play to the crowd", EventEffect::GainGold, 40, CardId::Jab,
             "They liked it. 40 gold."},
            {"Take the shot early", EventEffect::LoseHealth, 6, CardId::Jab,
             "You get to them first. It costs you 6."},
            {"Leave by the back", EventEffect::Nothing, 0, CardId::Jab, "Nobody follows."},
        },
    });

    events.push_back({
        "Short Stack",
        "A player down to their last chips offers a trade instead of folding.",
        {
            {"Buy their seat", EventEffect::LoseGold, 50, CardId::Jab,
             "You are 50 lighter, and one place closer."},
            {"Buy their knife", EventEffect::GainCard, 0, CardId::PocketAces,
             "Pocket Aces joins your deck."},
            {"Let them keep it", EventEffect::Heal, 8, CardId::Jab,
             "They patch you up instead. 8 health."},
        },
    });

    events.push_back({
        "Bad Beat Jackpot",
        "A board posts the worst losses of the night. There is money attached.",
        {
            {"Claim a share", EventEffect::GainGold, 70, CardId::Jab, "70 gold."},
            {"Add your name", EventEffect::LoseHealth, 10, CardId::Jab,
             "It costs you 10 to qualify. The board remembers you."},
            {"Read it and move on", EventEffect::Nothing, 0, CardId::Jab,
             "You have seen worse beats. You were there for most of them."},
        },
    });

    return events;
}

}  // namespace

const std::vector<EventDef>& AllEvents() {
    static const std::vector<EventDef> events = BuildEvents();
    return events;
}

const EventDef& RollEvent(Rng& rng) {
    const std::vector<EventDef>& events = AllEvents();
    return events[static_cast<std::size_t>(rng.Range(0, static_cast<int>(events.size()) - 1))];
}

}  // namespace bb
