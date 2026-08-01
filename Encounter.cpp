#include "combat/Encounter.h"

#include <algorithm>
#include <utility>

#include "combat/ArenaGenerator.h"
#include "combat/EnemyDatabase.h"
#include "core/Rules.h"

namespace bb {
namespace {

/// How many bodies a fight is worth. Grows slowly with depth so the curve comes
/// mostly from attrition and roster mix rather than sheer count -- eight actors
/// on an 8x6 board is a traffic jam, not a battle.
int EnemyCount(int depth, EncounterTier tier) {
    int count = 1 + depth / 2;
    if (tier == EncounterTier::Elite) count += 2;
    if (tier == EncounterTier::Boss) count += 2;
    return std::clamp(count, 1, 5);
}

/// A random rank-and-file enemy. Weighted rather than uniform so rats stay the
/// common case and a Slugger still feels like an event.
Archetype RandomMinion(Rng& rng) {
    const int roll = rng.Range(1, 100);
    if (roll <= 45) return Archetype::ChipRat;
    if (roll <= 80) return Archetype::Spitter;
    return Archetype::Slugger;
}

/// The enemies for one fight, in spawn order.
///
/// Index 0 is always a melee type, because index 0 is the tile next to the
/// player -- opening a fight with a Spitter breathing down your neck would put
/// it somewhere it cannot even shoot from.
std::vector<Archetype> BuildRoster(Rng& rng, int depth, EncounterTier tier) {
    const int count = EnemyCount(depth, tier);

    std::vector<Archetype> roster;
    roster.push_back(Archetype::ChipRat);

    if (tier == EncounterTier::Boss) {
        roster.push_back(Archetype::TheHouse);
    }

    while (static_cast<int>(roster.size()) < count) roster.push_back(RandomMinion(rng));
    return roster;
}

}  // namespace

std::unique_ptr<CombatState> MakeEncounter(std::uint64_t seed,
                                           int depth,
                                           EncounterTier tier,
                                           int starting_hp,
                                           std::vector<CardId> deck_list) {
    // Fold the depth and tier into the seed so each stop of a run plays
    // differently while the run as a whole stays reproducible from its one seed.
    const auto variation =
        static_cast<std::uint64_t>(depth) * 7919 + static_cast<std::uint64_t>(tier) * 104729;

    auto state = std::make_unique<CombatState>(rules::kBattlefieldWidth,
                                               rules::kBattlefieldHeight,
                                               seed + variation,
                                               std::move(deck_list));

    Rng& rng = state->GetRng();

    const std::vector<Archetype> roster = BuildRoster(rng, depth, tier);
    const ArenaLayout layout =
        GenerateArena(state->Field(), rng, depth, static_cast<int>(roster.size()));

    const ActorId hero =
        state->Spawn("Drifter", Team::Player, rules::kPlayerStartingHp, layout.player_start,
                     Archetype::Drifter);
    state->FindActor(hero)->SetHp(starting_hp);

    // The generator can come up short of spawn tiles on a crowded board, so the
    // roster is trimmed to what actually fits rather than assumed to.
    const std::size_t placeable =
        std::min(roster.size(), layout.enemy_spawns.size());

    for (std::size_t i = 0; i < placeable; ++i) {
        const EnemyDef& def = GetEnemy(roster[i]);
        state->Spawn(def.name, Team::Enemy, def.max_hp, layout.enemy_spawns[i], roster[i]);
    }

    return state;
}

}  // namespace bb
