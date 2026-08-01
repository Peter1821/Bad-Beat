#pragma once

#include <cstdint>

namespace bb {

/// One thing a card does.
///
/// Effects are *data*, not a class hierarchy -- no virtual Execute(). That buys
/// three things: cards serialise to JSON with no custom writer, a card's
/// description can be generated from its effect list instead of written out a
/// second time and kept in sync by hand, and an "upgraded" card is the same
/// definition with a bumped number.
///
/// Resolution lives in the ActionResolver (milestone 4). This header only
/// describes intent.
enum class EffectKind : std::uint8_t {
    Damage,       ///< `amount` damage to each affected actor. Block absorbs first.
    Block,        ///< `amount` block to each affected actor.
    Heal,         ///< `amount` HP restored.
    MoveSelf,     ///< Caster moves to the aimed tile.
    Push,         ///< Shove targets `amount` tiles directly away from the caster.
    Pull,         ///< Drag targets `amount` tiles towards the caster.
    Draw,         ///< Draw `amount` cards.
    GainEnergy,   ///< Refund `amount` energy.
    SpawnHazard,  ///< Turn each affected tile into a hazard.
};

struct EffectOp {
    EffectKind kind = EffectKind::Damage;

    /// Damage, block, tile count -- whatever the kind calls for.
    int amount = 0;
};

}  // namespace bb
