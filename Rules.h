#pragma once

namespace bb::rules {

/// Tuning constants for the agreed vertical slice.
///
/// These live in one header so balance changes are a single edit and never a
/// hunt through the renderer for a stray magic number.

inline constexpr int kBattlefieldWidth = 8;
inline constexpr int kBattlefieldHeight = 6;

/// Energy refills to this at the start of each player turn; unspent energy is
/// lost rather than banked, so a turn is a self-contained puzzle.
inline constexpr int kEnergyPerTurn = 3;

/// One free orthogonal step per turn guarantees a player is never completely
/// stuck behind a bad draw. Anything more than a single tile costs a card.
inline constexpr int kFreeStepsPerTurn = 1;

inline constexpr int kCardsDrawnPerTurn = 5;

inline constexpr int kPlayerStartingHp = 30;

/// Rows on the run map, including the boss row. Short on purpose: a whole run
/// should fit in one sitting, so a loss costs a coffee break rather than an
/// evening.
inline constexpr int kMapRows = 5;

/// Slots per row before unused ones are pruned. Three keeps the branching
/// legible in a terminal -- a wider map reads as noise rather than as a choice.
inline constexpr int kMapWidth = 3;

/// Routes walked through the map during generation. More paths means a denser
/// graph and softer commitment; fewer means sharper forks.
inline constexpr int kMapPaths = 4;

/// Fraction of maximum health a rest node restores, as a percentage.
inline constexpr int kRestHealPercent = 30;

/// Damage for stepping into a hazard. Charged **on entry only** -- a hazard is a
/// toll you pay to cross, not a trap that keeps billing you. Standing in one
/// costs nothing extra, so a player is never punished for forgetting where they
/// left their unit.
inline constexpr int kHazardDamage = 3;

/// Damage per tile of *unspent* push momentum when a shove is cut short.
///
/// A push that travels its full distance hits nothing and deals none. One
/// stopped a tile in has the rest of its force absorbed by whatever stopped it,
/// which is what makes shoving something against a wall worth setting up.
inline constexpr int kCollisionDamagePerTile = 2;

}  // namespace bb::rules
