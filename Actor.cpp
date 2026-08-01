#include "combat/Actor.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace bb {

Actor::Actor(ActorId id, std::string name, Team team, int max_hp, Archetype archetype)
    : id_(id),
      name_(std::move(name)),
      team_(team),
      archetype_(archetype),
      max_hp_(max_hp),
      hp_(max_hp) {
    assert(IsValid(id) && "actor ids start at 1; 0 is reserved for kNoActor");
    assert(max_hp > 0);
}

void Actor::AddBlock(int amount) {
    if (amount <= 0) return;
    block_ += amount;
}

int Actor::TakeDamage(int amount) {
    if (amount <= 0) return 0;

    const int absorbed = std::min(block_, amount);
    block_ -= absorbed;

    // Clamp to remaining HP so an overkill hit reports the damage that actually
    // landed. A 99-damage blow on a 3 HP target dealt 3, not 99.
    const int lost = std::min(amount - absorbed, hp_);
    hp_ -= lost;
    return lost;
}

void Actor::Heal(int amount) {
    if (amount <= 0 || !IsAlive()) return;
    hp_ = std::min(hp_ + amount, max_hp_);
}

void Actor::SetHp(int value) { hp_ = std::max(0, std::min(value, max_hp_)); }

}  // namespace bb
