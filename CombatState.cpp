#include "combat/CombatState.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "core/Rules.h"

namespace bb {

CombatState::CombatState(int width, int height, std::uint64_t seed, std::vector<CardId> deck_list)
    : field_(width, height),
      rng_(seed),
      deck_(std::move(deck_list), rng_),
      energy_(rules::kEnergyPerTurn) {}

void CombatState::SpendEnergy(int cost) noexcept { energy_ = std::max(0, energy_ - cost); }

void CombatState::GainEnergy(int amount) noexcept {
    if (amount > 0) energy_ += amount;
}

void CombatState::RefillEnergy() noexcept { energy_ = rules::kEnergyPerTurn; }

ActorId CombatState::Spawn(std::string name,
                           Team team,
                           int max_hp,
                           Vec2 destination,
                           std::optional<Archetype> archetype) {
    assert(field_.IsWalkable(destination) && "spawn destination must be free");

    const Archetype kind = archetype.value_or(
        team == Team::Player ? Archetype::Drifter : Archetype::ChipRat);

    const ActorId id{next_id_++};
    actors_.emplace_back(id, std::move(name), team, max_hp, kind);
    field_.Place(id, destination);
    return id;
}

Actor* CombatState::FindActor(ActorId id) {
    for (Actor& actor : actors_) {
        if (actor.Id() == id) return &actor;
    }
    return nullptr;
}

const Actor* CombatState::FindActor(ActorId id) const {
    return const_cast<CombatState*>(this)->FindActor(id);
}

Actor* CombatState::ActorAt(Vec2 tile) {
    const ActorId occupant = field_.OccupantAt(tile);
    return IsValid(occupant) ? FindActor(occupant) : nullptr;
}

const Actor* CombatState::ActorAt(Vec2 tile) const {
    return const_cast<CombatState*>(this)->ActorAt(tile);
}

std::vector<ActorId> CombatState::LivingActorsOnTeam(Team team) const {
    std::vector<ActorId> result;
    for (const Actor& actor : actors_) {
        if (actor.GetTeam() == team && actor.IsAlive()) result.push_back(actor.Id());
    }
    return result;
}

Actor* CombatState::PrimaryPlayer() {
    for (Actor& actor : actors_) {
        if (actor.GetTeam() == Team::Player && actor.IsAlive()) return &actor;
    }
    return nullptr;
}

const Actor* CombatState::PrimaryPlayer() const {
    return const_cast<CombatState*>(this)->PrimaryPlayer();
}

bool CombatState::IsTeamDefeated(Team team) const {
    bool has_member = false;
    for (const Actor& actor : actors_) {
        if (actor.GetTeam() != team) continue;
        has_member = true;
        if (actor.IsAlive()) return false;
    }
    // A team that was never populated is not "defeated" -- otherwise an empty
    // encounter would report both sides as losers on turn one.
    return has_member;
}

void CombatState::RemoveFromField(ActorId id) { field_.Remove(id); }

}  // namespace bb
