#pragma once

#include <vector>

#include "cards/CardDef.h"
#include "combat/ActorId.h"
#include "combat/CombatEvent.h"
#include "combat/CombatState.h"
#include "core/Vec2.h"

namespace bb {

/// Applies card effects to the board.
///
/// Every mutation a card causes goes through here, and every one of them appends
/// a CombatEvent. The resolver never formats text and never knows a screen
/// exists; it produces a record of what happened and lets the UI decide how to
/// show it.
///
/// Legality is re-checked here rather than trusted from the caller. The UI
/// already prevents illegal plays, but a resolver that assumes its inputs are
/// valid is one refactor away from corrupting the board.
class ActionResolver {
public:
    explicit ActionResolver(CombatState& state);

    /// Plays a card out of the hand.
    ///
    /// Returns false and changes nothing if the card is unaffordable, the aim is
    /// illegal, or the index is out of range -- so a rejected play is never a
    /// half-applied one.
    bool PlayCard(ActorId caster, int hand_index, Vec2 aim);

    /// Spends the actor's one free step of the turn.
    ///
    /// Returns false if the step is already spent or the destination is not an
    /// adjacent walkable tile.
    bool TakeFreeStep(ActorId actor, Vec2 destination);

    /// Carries out whatever an enemy telegraphed last turn.
    ///
    /// Attacks strike the tiles that were locked in when the intent was chosen,
    /// **not** wherever the player is standing now. If they moved, the blow
    /// lands on empty ground -- that miss is the reward for reading the board.
    void ExecuteIntent(ActorId enemy);

private:
    /// Applies a single effect to an already-validated set of tiles.
    void ApplyEffect(const EffectOp& effect, ActorId caster, Vec2 aim,
                     const std::vector<Vec2>& tiles);

    /// Moves an actor and applies whatever the destination does to it.
    ///
    /// Every position change in the game routes through here -- steps, dashes,
    /// shoves, pulls -- so a hazard cannot be dodged by arriving via an unusual
    /// path. Centralising this is the difference between one hazard rule and
    /// four subtly different ones.
    void ResolveMove(ActorId id, Vec2 destination);

    /// Applies damage and reaps the actor if it kills them.
    void ResolveDamage(ActorId target, int amount, ActorId source);

    /// Shoves a target along `direction`, stopping at the first thing in the
    /// way and converting the leftover momentum into collision damage.
    void ResolveShove(ActorId target, Vec2 direction, int distance, ActorId source);

    void Emit(CombatEvent event);

    CombatState& state_;
};

}  // namespace bb
