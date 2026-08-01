#pragma once

#include "combat/CombatState.h"

namespace bb {

/// Drives the turn cycle.
///
/// Player phase, then enemy phase -- no interleaved initiative, because a
/// threat you cannot predict cannot be dodged, and dodging is the game.
///
/// Enemy behaviour is a stub for now: the phase runs and ends. Intents and AI
/// arrive in milestone 5, and they slot into RunEnemyPhase without the turn
/// structure changing.
class TurnSystem {
public:
    explicit TurnSystem(CombatState& state);

    /// Prepares the first player turn. Call once, after the encounter is built.
    void BeginFight();

    /// Ends the player's turn, runs the enemy phase, and starts the next player
    /// turn. Does nothing once the fight is over.
    void EndPlayerTurn();

    /// True once one side has no one left standing.
    bool IsFightOver() const;

    /// True when the player won. Only meaningful once IsFightOver().
    bool PlayerWon() const;

private:
    void BeginPlayerTurn();
    void RunEnemyPhase();

    /// Has every living enemy choose and display its next action.
    void TelegraphIntents();

    CombatState& state_;
};

}  // namespace bb
