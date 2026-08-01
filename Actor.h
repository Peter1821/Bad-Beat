#pragma once

#include <cstdint>
#include <string>

#include "combat/ActorId.h"
#include "combat/Archetype.h"
#include "combat/Intent.h"

namespace bb {

enum class Team : std::uint8_t {
    Player,
    Enemy,
};

constexpr bool AreHostile(Team a, Team b) noexcept { return a != b; }

/// A combatant's identity and durability.
///
/// Deliberately holds no position. Battlefield is the single owner of where
/// things are, so an actor's coordinate and the occupancy grid cannot disagree
/// -- there is only one copy to get wrong.
///
/// Note there is no player-specific subclass and no "is the player" flag: a
/// combatant is identified only by its Team. Player-controlled units are simply
/// the actors whose team is Team::Player, which is what lets a second or third
/// hero be added later without the simulation changing shape.
class Actor {
public:
    Actor(ActorId id, std::string name, Team team, int max_hp, Archetype archetype);

    ActorId Id() const noexcept { return id_; }
    const std::string& Name() const noexcept { return name_; }
    Team GetTeam() const noexcept { return team_; }
    Archetype GetArchetype() const noexcept { return archetype_; }

    /// What this actor has announced it will do next. Only enemies telegraph;
    /// a player actor's intent stays Wait and is never displayed.
    const Intent& GetIntent() const noexcept { return intent_; }
    void SetIntent(Intent intent) { intent_ = std::move(intent); }

    int Hp() const noexcept { return hp_; }
    int MaxHp() const noexcept { return max_hp_; }
    bool IsAlive() const noexcept { return hp_ > 0; }

    int Block() const noexcept { return block_; }
    void AddBlock(int amount);
    void ClearBlock() noexcept { block_ = 0; }

    /// Applies damage, spending block first.
    ///
    /// Returns the HP actually lost, which is what the combat log reports. That
    /// differs from `amount` whenever block absorbs part of the hit or the blow
    /// overkills -- callers that want "damage dealt" want this, not the input.
    int TakeDamage(int amount);

    /// Heals up to MaxHp. Has no effect on a dead actor: death is final within a
    /// fight, and revival would need to be an explicit, separate operation.
    void Heal(int amount);

    /// Sets health directly, clamped to [0, MaxHp].
    ///
    /// For run setup only -- carrying a wounded hero into the next fight.
    /// Anything happening *during* combat should go through TakeDamage or Heal
    /// so that it produces a log entry and can kill.
    void SetHp(int value);

    /// Each actor gets one free orthogonal step per turn. The turn system clears
    /// this at the start of the actor's turn; movement cards ignore it entirely.
    bool HasUsedFreeStep() const noexcept { return used_free_step_; }
    void MarkFreeStepUsed() noexcept { used_free_step_ = true; }
    void ResetFreeStep() noexcept { used_free_step_ = false; }

private:
    ActorId id_;
    std::string name_;
    Team team_;
    Archetype archetype_;
    int max_hp_;
    int hp_;
    int block_ = 0;
    bool used_free_step_ = false;
    Intent intent_;
};

}  // namespace bb
