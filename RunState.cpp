#include "run/RunState.h"

#include <algorithm>

#include "cards/CardDatabase.h"
#include "core/Rules.h"

namespace bb {

RunState::RunState(std::uint64_t seed)
    : seed_(seed),
      rng_(seed),
      map_(RunMap::Generate(rng_, rules::kMapRows, rules::kMapWidth, rules::kMapPaths)),
      max_hp_(rules::kPlayerStartingHp),
      hp_(rules::kPlayerStartingHp),
      deck_(StartingDeck()) {}

void RunState::SetHp(int value) { hp_ = std::max(0, std::min(value, max_hp_)); }

void RunState::Heal(int amount) {
    if (amount <= 0) return;
    SetHp(hp_ + amount);
}

void RunState::AddGold(int amount) {
    if (amount <= 0) return;
    gold_ += amount;
}

void RunState::AddCard(CardId card) { deck_.push_back(card); }

bool RunState::RemoveCard(int index) {
    if (index < 0 || index >= static_cast<int>(deck_.size())) return false;
    if (deck_.size() <= 1) return false;

    deck_.erase(deck_.begin() + index);
    return true;
}

bool RunState::TrySpendGold(int amount) {
    if (amount < 0 || amount > gold_) return false;
    gold_ -= amount;
    return true;
}

bool RunState::MoveTo(int node_index) {
    const std::vector<int> options = Options();
    if (std::find(options.begin(), options.end(), node_index) == options.end()) return false;

    current_node_ = node_index;
    visited_.push_back(node_index);
    return true;
}

void RunState::ClearNode() {
    ++nodes_cleared_;

    // Clearing the top of the map is the win condition, and it is recorded here
    // rather than derived from position -- the player is still standing on the
    // boss node afterwards, and "am I on it" is not the same as "did I beat it".
    if (current_node_ >= 0 && map_.Node(current_node_).type == NodeType::Boss) {
        boss_cleared_ = true;
    }
}

int RunState::CurrentRow() const noexcept {
    if (current_node_ < 0) return 0;
    return map_.Node(current_node_).row + 1;
}

}  // namespace bb
