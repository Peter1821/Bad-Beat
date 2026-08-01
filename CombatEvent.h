#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cards/CardDef.h"
#include "combat/ActorId.h"
#include "core/Vec2.h"

namespace bb {

class CombatState;

/// Something that happened during a fight.
///
/// The resolver mutates state and appends these; it never formats a string and
/// never touches the screen. The log panel renders them, and later an animated
/// enemy phase can replay the same list with delays -- no change to the
/// simulation required.
enum class CombatEventKind : std::uint8_t {
    TurnBegan,
    CardPlayed,
    Damaged,
    Blocked,       ///< Gained block, not "damage was blocked".
    Healed,
    Moved,
    Pushed,
    HazardBurn,    ///< Walked into a hazard.
    Collided,      ///< Shove cut short by terrain or another actor.
    Died,
    CardsDrawn,
    EnergyGained,
    TurnEnded,
};

struct CombatEvent {
    CombatEventKind kind = CombatEventKind::TurnBegan;

    /// Who it happened to.
    ActorId actor = kNoActor;

    /// Who caused it, when that is not the same thing.
    ActorId source = kNoActor;

    /// Damage, block, tiles -- whatever the kind calls for.
    int amount = 0;

    Vec2 from;
    Vec2 to;

    CardId card = CardId::Jab;
};

/// Renders one event as log text.
///
/// Lives in core rather than the UI because it needs actor names out of
/// CombatState, and because it is worth testing -- a log that misreports what
/// happened is worse than no log.
std::string DescribeEvent(const CombatEvent& event, const CombatState& state);

}  // namespace bb
