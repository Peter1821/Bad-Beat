#pragma once

#include <optional>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "combat/CombatState.h"
#include "core/Vec2.h"

namespace bb::ui {

/// What to draw on top of the plain board.
///
/// Kept as a struct rather than extra parameters because it keeps growing:
/// milestone 5 adds the tiles enemies are about to hit. Callers that do not care
/// keep passing {}.
struct GridOverlay {
    std::optional<Vec2> cursor;

    /// Tiles the pending card would hit. Always produced by
    /// ComputeAffectedTiles -- never recomputed here -- so the highlight cannot
    /// disagree with what actually happens.
    std::vector<Vec2> affected;

    /// Tiles the pending card could legally be aimed at. Drawn faintly, so the
    /// player can see a card's reach before committing to an aim point.
    std::vector<Vec2> in_range;

    /// Tiles enemies have announced they will hit next turn. The reason the
    /// game is tactical rather than a damage race: these are visible for a full
    /// turn before they land.
    std::vector<Vec2> threatened;
};

/// Draws the battlefield as a bordered grid of 3-wide cells.
///
/// The cursor is drawn with brackets -- `[@]` instead of ` @ ` -- rather than by
/// colour alone. That keeps it legible when colour is unavailable, and means the
/// headless dump can verify cursor position from text.
ftxui::Element RenderBattlefield(const CombatState& state, const GridOverlay& overlay);

}  // namespace bb::ui
