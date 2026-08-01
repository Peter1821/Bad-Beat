#pragma once

#include <cstdint>

namespace bb {

/// Where a card may be aimed.
enum class OriginRule : std::uint8_t {
    Self,                 ///< Always the caster's own tile; aim is ignored.
    AdjacentTile,         ///< Orthogonally adjacent only.
    AnyTileInRange,       ///< Any tile within `range`, occupied or not.
    OccupiedTileInRange,  ///< Must be aimed at an actor.
    WalkableTileInRange,  ///< Must be somewhere the caster could stand.
    Direction,            ///< Picks one of four directions rather than a tile.
};

/// What gets hit, relative to the aim point (or to the caster, for the shapes
/// that project outward).
enum class Shape : std::uint8_t {
    Single,   ///< Just the aimed tile.
    Line,     ///< `size` tiles outward from the caster.
    Cone,     ///< Widening wedge, `size` tiles deep.
    Blast,    ///< Filled square, Chebyshev radius `size`.
    Diamond,  ///< Filled diamond, Manhattan radius `size`.
    Ring,     ///< Diamond outline only: Manhattan distance exactly `size`.
    Row,      ///< The aimed tile's entire row.
    Column,   ///< The aimed tile's entire column.
};

/// The complete targeting rule for a card.
///
/// Deliberately a plain struct of scalars: it is copied freely, compared in
/// tests, and will serialise straight to JSON without a custom writer.
struct TargetPattern {
    OriginRule origin = OriginRule::AnyTileInRange;

    /// How far the aim point may be from the caster, in Manhattan steps.
    /// Ignored for Self and Direction origins.
    int range = 1;

    Shape shape = Shape::Single;

    /// Length, depth or radius, depending on `shape`. Ignored by Single, Row
    /// and Column.
    int size = 1;

    /// Whether terrain can block this card.
    ///
    /// On by default: line of sight is what makes walls matter. Turned off for
    /// the occasional card that is *supposed* to reach around cover, which is
    /// then a real selling point for that card rather than an oversight.
    bool requires_line_of_sight = true;
};

}  // namespace bb
