#pragma once

#include <ftxui/dom/elements.hpp>

#include "cards/CardDef.h"

namespace bb::ui {

/// Draws one card as it appears in the hand.
///
/// `hotkey` is the 1-based number that selects it. `affordable` dims the card
/// when there is not enough energy, so the reason a card cannot be played is
/// visible without trying it.
///
/// The shape tag on the face ("cone 2", "range 4") is generated from the card's
/// own TargetPattern, so a card can never advertise a shape it does not have.
ftxui::Element RenderCard(const CardDef& card, int hotkey, bool selected, bool affordable);

/// Width of a rendered card, in terminal columns. Exposed so the hand row can
/// work out how many fit.
int CardWidth();

}  // namespace bb::ui
