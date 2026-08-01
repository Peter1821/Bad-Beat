#pragma once

#include <cstdint>
#include <vector>

#include "cards/CardDef.h"
#include "core/Rng.h"
#include "map/RunMap.h"

namespace bb {

/// Everything that survives from one fight to the next.
///
/// A run is the unit of play: HP and deck persist across nodes, and all of it is
/// thrown away on death. Nothing here knows about screens or combat -- a fight
/// reports its outcome and this decides what the run looks like afterwards.
class RunState {
public:
    explicit RunState(std::uint64_t seed);

    std::uint64_t Seed() const noexcept { return seed_; }

    /// The run's random source. Every shuffle, layout and reward draws from it,
    /// so a seed reproduces the whole run rather than just its first fight.
    Rng& GetRng() noexcept { return rng_; }

    int Hp() const noexcept { return hp_; }
    int MaxHp() const noexcept { return max_hp_; }
    bool IsAlive() const noexcept { return hp_ > 0; }

    /// Carries the damage taken in a fight back into the run.
    void SetHp(int value);

    /// Restores HP without exceeding the maximum. Rest nodes, milestone 8.
    void Heal(int amount);

    int Gold() const noexcept { return gold_; }
    void AddGold(int amount);

    const std::vector<CardId>& DeckList() const noexcept { return deck_; }
    void AddCard(CardId card);

    /// Drops one card from the deck by index. Refuses to empty the deck
    /// entirely, since a run with no cards is unplayable rather than merely
    /// difficult.
    bool RemoveCard(int index);

    /// Spends gold. Returns false and changes nothing if there is not enough.
    bool TrySpendGold(int amount);

    const RunMap& Map() const noexcept { return map_; }

    /// Where the player is standing, or -1 before they have entered the map.
    int CurrentNodeIndex() const noexcept { return current_node_; }

    /// The nodes reachable from here.
    std::vector<int> Options() const { return map_.OptionsFrom(current_node_); }

    /// Moves onto a node. Only accepts somewhere actually reachable, so a
    /// mis-wired screen cannot teleport the player up the map.
    bool MoveTo(int node_index);

    /// Node indices already visited, oldest first.
    const std::vector<int>& Visited() const noexcept { return visited_; }

    /// How many nodes have been cleared.
    int NodesCleared() const noexcept { return nodes_cleared_; }
    void ClearNode();

    /// True once the boss is behind us -- the run was won.
    bool IsComplete() const noexcept { return boss_cleared_; }

    /// 1-based row the player is on, for display.
    int CurrentRow() const noexcept;

private:
    std::uint64_t seed_;
    Rng rng_;
    RunMap map_;
    int max_hp_;
    int hp_;
    int gold_ = 0;
    int nodes_cleared_ = 0;
    int current_node_ = -1;
    bool boss_cleared_ = false;
    std::vector<int> visited_;
    std::vector<CardId> deck_;
};

}  // namespace bb
