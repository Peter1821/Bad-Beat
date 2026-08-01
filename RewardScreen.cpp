#include "ui/RewardScreen.h"

#include <algorithm>
#include <utility>

#include "cards/CardDatabase.h"
#include "ui/Theme.h"
#include "ui/widgets/CardWidget.h"

namespace bb::ui {

RewardScreen::RewardScreen(std::vector<CardId> offered, int gold)
    : offered_(std::move(offered)), gold_(gold) {}

int OptionCount(std::size_t offered) { return static_cast<int>(offered) + 1; }

void RewardScreen::SelectNext() {
    cursor_ = (cursor_ + 1) % OptionCount(offered_.size());
}

void RewardScreen::SelectPrevious() {
    const int count = OptionCount(offered_.size());
    cursor_ = (cursor_ - 1 + count) % count;
}

std::optional<CardId> RewardScreen::Selection() const {
    if (cursor_ < 0 || cursor_ >= static_cast<int>(offered_.size())) return std::nullopt;
    return offered_[static_cast<std::size_t>(cursor_)];
}

void RewardScreen::Choose() {
    if (done_) return;
    taken_ = Selection();
    done_ = true;
}

bool RewardScreen::HandleEvent(ftxui::Event event) {
    using namespace ftxui;

    if (event == Event::ArrowRight || event == Event::Character('l')) {
        SelectNext();
        return true;
    }
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        SelectPrevious();
        return true;
    }

    for (int i = 1; i <= static_cast<int>(offered_.size()); ++i) {
        if (event == Event::Character(static_cast<char>('0' + i))) {
            cursor_ = i - 1;
            return true;
        }
    }
    return false;
}

ftxui::Element RewardScreen::Render() const {
    using namespace ftxui;

    Elements cards;
    for (int i = 0; i < static_cast<int>(offered_.size()); ++i) {
        const CardDef& card = GetCard(offered_[static_cast<std::size_t>(i)]);
        cards.push_back(RenderCard(card, i + 1, i == cursor_, true));
    }

    const bool skipping = cursor_ >= static_cast<int>(offered_.size());

    Element skip = vbox({
                       text("SKIP") | bold,
                       text(""),
                       paragraph("Keep the deck thin.") | color(theme::kMuted),
                   }) |
                   size(WIDTH, EQUAL, 18) | size(HEIGHT, EQUAL, 7);
    skip = skipping ? (skip | borderHeavy | color(theme::kAccent)) : (skip | border);

    cards.push_back(text("  "));
    cards.push_back(std::move(skip));

    return vbox({
               hbox({text(" BAD BEAT ") | bold | inverted, filler(),
                     text("reward ") | color(theme::kMuted)}),
               separator(),
               filler(),
               text("TAKE A CARD") | bold | color(theme::kPlayer) | center,
               text(""),
               hbox({
                   filler(),
                   text("+" + std::to_string(gold_) + " gold") | bold |
                       color(theme::kAccent),
                   filler(),
               }),
               text(""),
               hbox({filler(), hbox(std::move(cards)), filler()}),
               filler(),
               separator(),
               hbox({
                   text(" ←→ ") | bold | color(theme::kAccent),
                   text("choose"),
                   text("   "),
                   text(" enter ") | bold | color(theme::kAccent),
                   text(skipping ? "skip it" : "take it"),
                   filler(),
                   text(" q ") | bold | color(theme::kAccent),
                   text("quit"),
               }),
           }) |
           border;
}

}  // namespace bb::ui
