#pragma once

#include <ftxui/dom/elements.hpp>

#include "combat/CombatState.h"

namespace bb::ui {

/// Renders the tail of the combat log, oldest of the visible lines first.
///
/// Reads the CombatEvent list the resolver already produces -- it does not
/// reconstruct anything. When an animated enemy phase arrives, it replays the
/// same list with delays and this panel needs no change.
ftxui::Element RenderLog(const CombatState& state, int max_lines);

}  // namespace bb::ui
