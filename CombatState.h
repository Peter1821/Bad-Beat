#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "cards/CardDef.h"
#include "cards/Deck.h"
#include "combat/Actor.h"
#include "combat/ActorId.h"
#include "combat/Battlefield.h"
#include "combat/CombatEvent.h"
#include "core/Rng.h"
#include "core/Vec2.h"

namespace bb {

/// Everything that makes up one fight: the board, and everyone on it.
///
/// Actors are stored in a std::deque rather than a std::vector on purpose.
/// Appending to a deque invalidates iterators but *not* references, so a pointer
/// handed out by FindActor stays valid even if something spawns mid-fight. With
/// a vector, a summon effect would silently dangle every actor pointer in
/// flight.
///
/// Dead actors stay in the roster and are only removed from the battlefield, so
/// the combat log can still name them after they die.
class CombatState {
public:
    CombatState(int width,
                int height,
                std::uint64_t seed = 0,
                std::vector<CardId> deck_list = {});

    /// Neither copyable nor movable, and deliberately so.
    ///
    /// Deck holds a reference to the Rng below it. If this object were moved,
    /// that reference would still point into the moved-from husk -- a dangling
    /// reference that would survive every test and then corrupt shuffles at
    /// runtime. Pass it by reference, or hold it in a unique_ptr.
    CombatState(const CombatState&) = delete;
    CombatState& operator=(const CombatState&) = delete;
    CombatState(CombatState&&) = delete;
    CombatState& operator=(CombatState&&) = delete;

    Battlefield& Field() noexcept { return field_; }
    const Battlefield& Field() const noexcept { return field_; }

    Deck& GetDeck() noexcept { return deck_; }
    const Deck& GetDeck() const noexcept { return deck_; }

    Rng& GetRng() noexcept { return rng_; }

    int Energy() const noexcept { return energy_; }
    bool CanAfford(int cost) const noexcept { return cost <= energy_; }
    void SpendEnergy(int cost) noexcept;
    void GainEnergy(int amount) noexcept;

    int TurnNumber() const noexcept { return turn_number_; }
    void AdvanceTurn() noexcept { ++turn_number_; }

    /// Everything that has happened this fight, oldest first.
    const std::vector<CombatEvent>& Log() const noexcept { return log_; }
    void AppendEvent(const CombatEvent& event) { log_.push_back(event); }

    /// Refills energy for a new turn. Unspent energy is lost rather than banked,
    /// so each turn stays a self-contained puzzle.
    void RefillEnergy() noexcept;

    /// Creates an actor, assigns it the next id, and places it on the board.
    /// The destination must be walkable.
    ///
    /// Omitting the archetype picks the obvious default for the team, which
    /// keeps simple test setups short without letting a real encounter spawn an
    /// enemy with no behaviour.
    ActorId Spawn(std::string name,
                  Team team,
                  int max_hp,
                  Vec2 destination,
                  std::optional<Archetype> archetype = std::nullopt);

    Actor* FindActor(ActorId id);
    const Actor* FindActor(ActorId id) const;

    /// Whoever is standing on a tile, or nullptr. Off-grid tiles are empty.
    Actor* ActorAt(Vec2 tile);
    const Actor* ActorAt(Vec2 tile) const;

    const std::deque<Actor>& Actors() const noexcept { return actors_; }

    /// Mutable roster access, for systems that sweep every actor -- turn
    /// upkeep, status ticks. Prefer FindActor when you want one specific actor.
    std::deque<Actor>& ActorsMutable() noexcept { return actors_; }

    /// Living actors on a team, in spawn order.
    std::vector<ActorId> LivingActorsOnTeam(Team team) const;

    /// The actor the player is currently controlling.
    ///
    /// Today there is exactly one, so this returns it. It is deliberately a
    /// *query over the roster* rather than a stored pointer, because when squads
    /// arrive this becomes "the selected one of several" and nothing else has to
    /// change.
    Actor* PrimaryPlayer();
    const Actor* PrimaryPlayer() const;

    /// True when every actor on a team is dead -- the fight's end condition.
    bool IsTeamDefeated(Team team) const;

    /// Marks an actor as no longer on the board. Keeps it in the roster.
    void RemoveFromField(ActorId id);

private:
    Battlefield field_;

    // Declaration order matters: rng_ must outlive deck_, which holds a
    // reference to it.
    Rng rng_;
    Deck deck_;

    std::deque<Actor> actors_;
    std::vector<CombatEvent> log_;
    std::uint32_t next_id_ = 1;
    int energy_ = 0;
    int turn_number_ = 1;
};

}  // namespace bb
