#pragma once

#include <string>
#include <vector>

#include "cards/TargetPattern.h"
#include "combat/Archetype.h"
#include "combat/Intent.h"

namespace bb {

/// One thing an enemy knows how to do.
///
/// Reuses the card system's TargetPattern rather than inventing a parallel one.
/// That means an enemy attack resolves through exactly the same shape and
/// line-of-sight code a player card does -- so a wall that stops your Long Shot
/// stops a Spitter too, and there is only one set of rules to learn or debug.
struct EnemyAbility {
    IntentKind kind = IntentKind::Attack;
    int amount = 0;
    TargetPattern pattern;

    /// Only considered when the nearest player sits inside this distance band,
    /// measured in orthogonal steps.
    int min_range = 0;
    int max_range = 99;
};

/// A kind of enemy.
struct EnemyDef {
    Archetype archetype = Archetype::ChipRat;
    std::string name;
    int max_hp = 10;

    /// Tried in order; the first ability whose conditions hold is telegraphed.
    /// Ordering *is* the AI -- there is no search, because an enemy whose next
    /// move cannot be predicted cannot be dodged, and dodging is the game.
    std::vector<EnemyAbility> abilities;
};

const EnemyDef& GetEnemy(Archetype archetype);

}  // namespace bb
