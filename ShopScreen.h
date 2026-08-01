#pragma once

#include <optional>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "cards/CardDef.h"
#include "run/RunState.h"

namespace bb::ui {

/// Somewhere to spend gold.
///
/// Sells cards, and sells the removal of one. Removal is the more valuable of
/// the two in a short run -- thinning a ten-card deck does more than adding an
/// eleventh -- so it is priced accordingly and never hidden behind the stock.
class ShopScreen {
public:
    ShopScreen(RunState& run, std::vector<CardId> stock);

    bool HandleEvent(ftxui::Event event);
    ftxui::Element Render() const;

    void SelectNext();
    void SelectPrevious();

    /// Buys or removes whatever is highlighted. Silently does nothing if the
    /// player cannot afford it.
    void Activate();

    /// True once the player has chosen to leave.
    bool IsDone() const noexcept { return done_; }

private:
    /// The rows are the stock, then "remove a card", then "leave".
    int RowCount() const;
    bool OnRemoveRow() const;
    bool OnLeaveRow() const;

    RunState& run_;
    std::vector<CardId> stock_;
    std::vector<bool> sold_;

    int cursor_ = 0;
    bool done_ = false;

    /// Set while the player is picking which card to scrap.
    bool removing_ = false;
    int removal_cursor_ = 0;
};

}  // namespace bb::ui
