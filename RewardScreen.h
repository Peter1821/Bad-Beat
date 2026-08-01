#pragma once

#include <string>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "cards/CardDef.h"

namespace bb::ui {

/// The card pick after a fight.
///
/// Skipping is always available and always visible. In a short run a thin deck
/// is often stronger than a broad one, so taking every card on offer should be a
/// decision the player makes rather than the only button on screen.
class RewardScreen {
public:
    RewardScreen(std::vector<CardId> offered, int gold);

    bool HandleEvent(ftxui::Event event);
    ftxui::Element Render() const;

    void SelectNext();
    void SelectPrevious();

    /// The highlighted card, or nullopt when "skip" is highlighted.
    std::optional<CardId> Selection() const;

    /// True once the player has taken or skipped.
    bool IsDone() const noexcept { return done_; }

    /// What they took, valid once IsDone().
    std::optional<CardId> Taken() const noexcept { return taken_; }

    /// Commits the highlighted choice.
    void Choose();

private:
    std::vector<CardId> offered_;
    int gold_ = 0;

    /// Index into offered_, or offered_.size() for the skip option.
    int cursor_ = 0;
    bool done_ = false;
    std::optional<CardId> taken_;
};

}  // namespace bb::ui
