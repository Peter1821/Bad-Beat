#pragma once

#include <cstdint>
#include <vector>

#include "core/Vec2.h"

namespace bb {

/// What an enemy has announced it will do on its next turn.
enum class IntentKind : std::uint8_t {
    Wait,     ///< Nothing it can usefully do.
    Move,     ///< Closing distance. Threatens nothing.
    Retreat,  ///< Backing off to regain firing range. Threatens nothing.
    Attack,   ///< Will hit `threatened` for `amount`.
    Defend,   ///< Will gain `amount` block.
};

/// A telegraphed action.
///
/// The threatened tiles are **locked when the intent is chosen**, not
/// recalculated when it fires. That is the entire tactical engine: an enemy
/// commits to hitting specific squares, and the player has a full turn to not be
/// standing on them. Recomputing at execution time would make attacks
/// unavoidable and reduce the game to a damage race.
///
/// Movement is the deliberate exception -- it threatens nothing, so it re-paths
/// when it runs and simply walks towards wherever the player ended up.
struct Intent {
    IntentKind kind = IntentKind::Wait;
    int amount = 0;
    std::vector<Vec2> threatened;
};

}  // namespace bb
