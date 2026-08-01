#include "combat/EnemyDatabase.h"

#include <array>
#include <cassert>

namespace bb {
namespace {

using Enemies = std::array<EnemyDef, static_cast<std::size_t>(Archetype::Count)>;

Enemies BuildEnemies() {
    Enemies enemies;

    // The player is in the same table so lookups never need a special case.
    enemies[static_cast<std::size_t>(Archetype::Drifter)] = {
        Archetype::Drifter, "Drifter", 30, {}};

    // Closes and bites. Teaches the most basic lesson: kill it, or step away.
    enemies[static_cast<std::size_t>(Archetype::ChipRat)] = {
        Archetype::ChipRat, "Chip Rat", 12,
        {
            EnemyAbility{IntentKind::Attack, 5,
                         TargetPattern{OriginRule::AdjacentTile, 1, Shape::Single, 1, true},
                         1, 1},
        }};

    // Shoots down a clear line and never closes. Teaches: break line of sight.
    enemies[static_cast<std::size_t>(Archetype::Spitter)] = {
        Archetype::Spitter, "Spitter", 9,
        {
            EnemyAbility{IntentKind::Attack, 4,
                         TargetPattern{OriginRule::AnyTileInRange, 4, Shape::Single, 1, true},
                         2, 4},
        }};

    // Slow, and telegraphs a 3x3 slam. Teaches: read the shading and move one
    // tile. Hits hard enough that ignoring the telegraph is not survivable.
    enemies[static_cast<std::size_t>(Archetype::Slugger)] = {
        Archetype::Slugger, "Slugger", 18,
        {
            EnemyAbility{IntentKind::Attack, 7,
                         TargetPattern{OriginRule::AnyTileInRange, 3, Shape::Blast, 1, true},
                         1, 3},
        }};

    // The boss. Its signature is a full-row sweep that ignores cover entirely --
    // the one place in the game where line of sight does not save you, which is
    // why it reads as a boss rather than a bigger Slugger. You dodge it by
    // changing rows, not by hiding.
    enemies[static_cast<std::size_t>(Archetype::TheHouse)] = {
        Archetype::TheHouse, "The House", 40,
        {
            EnemyAbility{IntentKind::Attack, 6,
                         TargetPattern{OriginRule::AnyTileInRange, 9, Shape::Row, 0, false},
                         2, 9},
            // Point blank it stops sweeping and just hits back.
            EnemyAbility{IntentKind::Attack, 8,
                         TargetPattern{OriginRule::AdjacentTile, 1, Shape::Single, 1, true},
                         1, 1},
        }};

    return enemies;
}

}  // namespace

const EnemyDef& GetEnemy(Archetype archetype) {
    static const Enemies enemies = BuildEnemies();
    assert(archetype != Archetype::Count);
    return enemies[static_cast<std::size_t>(archetype)];
}

}  // namespace bb
