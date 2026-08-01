#include "ui/ShopScreen.h"

#include <algorithm>
#include <string>
#include <utility>

#include "cards/CardDatabase.h"
#include "run/Rewards.h"
#include "ui/Theme.h"

namespace bb::ui {

ShopScreen::ShopScreen(RunState& run, std::vector<CardId> stock)
    : run_(run), stock_(std::move(stock)), sold_(stock_.size(), false) {}

int ShopScreen::RowCount() const { return static_cast<int>(stock_.size()) + 2; }

bool ShopScreen::OnRemoveRow() const { return cursor_ == static_cast<int>(stock_.size()); }

bool ShopScreen::OnLeaveRow() const { return cursor_ == static_cast<int>(stock_.size()) + 1; }

void ShopScreen::SelectNext() {
    if (removing_) {
        const int count = static_cast<int>(run_.DeckList().size());
        if (count > 0) removal_cursor_ = (removal_cursor_ + 1) % count;
        return;
    }
    cursor_ = (cursor_ + 1) % RowCount();
}

void ShopScreen::SelectPrevious() {
    if (removing_) {
        const int count = static_cast<int>(run_.DeckList().size());
        if (count > 0) removal_cursor_ = (removal_cursor_ - 1 + count) % count;
        return;
    }
    cursor_ = (cursor_ - 1 + RowCount()) % RowCount();
}

void ShopScreen::Activate() {
    if (done_) return;

    if (removing_) {
        if (!run_.TrySpendGold(CardRemovalPrice())) {
            removing_ = false;
            return;
        }
        if (!run_.RemoveCard(removal_cursor_)) {
            // Refund rather than charge for nothing -- removal is refused when
            // it would empty the deck.
            run_.AddGold(CardRemovalPrice());
        }
        removing_ = false;
        removal_cursor_ = 0;
        return;
    }

    if (OnLeaveRow()) {
        done_ = true;
        return;
    }

    if (OnRemoveRow()) {
        if (run_.Gold() >= CardRemovalPrice() && run_.DeckList().size() > 1) {
            removing_ = true;
            removal_cursor_ = 0;
        }
        return;
    }

    const auto index = static_cast<std::size_t>(cursor_);
    if (index >= stock_.size() || sold_[index]) return;

    const CardDef& card = GetCard(stock_[index]);
    if (!run_.TrySpendGold(ShopPrice(card))) return;

    run_.AddCard(card.id);
    sold_[index] = true;
}

bool ShopScreen::HandleEvent(ftxui::Event event) {
    using namespace ftxui;

    if (event == Event::ArrowDown || event == Event::Character('j') ||
        event == Event::ArrowRight || event == Event::Character('l')) {
        SelectNext();
        return true;
    }
    if (event == Event::ArrowUp || event == Event::Character('k') ||
        event == Event::ArrowLeft || event == Event::Character('h')) {
        SelectPrevious();
        return true;
    }
    if (event == Event::Escape && removing_) {
        removing_ = false;
        return true;
    }
    return false;
}

ftxui::Element ShopScreen::Render() const {
    using namespace ftxui;

    Elements rows;

    for (int i = 0; i < static_cast<int>(stock_.size()); ++i) {
        const CardDef& card = GetCard(stock_[static_cast<std::size_t>(i)]);
        const int price = ShopPrice(card);
        const bool sold = sold_[static_cast<std::size_t>(i)];
        const bool affordable = run_.Gold() >= price;
        const bool here = !removing_ && cursor_ == i;

        Elements line{
            text(here ? "> " : "  ") | color(theme::kAccent) | bold,
            text(card.name) | bold | size(WIDTH, EQUAL, 14),
            text(DescribeShape(card.pattern)) | color(theme::kThreat) |
                size(WIDTH, EQUAL, 12),
            text(DescribeCard(card)) | color(theme::kMuted) | size(WIDTH, EQUAL, 30),
            text(sold ? "sold" : std::to_string(price) + "g") | bold |
                color(sold ? theme::kMuted
                           : (affordable ? theme::kAccent : theme::kEnemy)),
        };

        Element row = hbox(std::move(line));
        if (sold || !affordable) row = row | dim;
        rows.push_back(std::move(row));
    }

    const bool can_remove =
        run_.Gold() >= CardRemovalPrice() && run_.DeckList().size() > 1;

    rows.push_back(text(""));
    rows.push_back(hbox({
        text(!removing_ && OnRemoveRow() ? "> " : "  ") | color(theme::kAccent) | bold,
        text("Scrap a card") | bold | size(WIDTH, EQUAL, 14),
        text("thin the deck") | color(theme::kMuted) | size(WIDTH, EQUAL, 42),
        text(std::to_string(CardRemovalPrice()) + "g") | bold |
            color(can_remove ? theme::kAccent : theme::kEnemy),
    }));
    rows.push_back(hbox({
        text(!removing_ && OnLeaveRow() ? "> " : "  ") | color(theme::kAccent) | bold,
        text("Leave") | bold,
    }));

    Element body = vbox(std::move(rows));

    if (removing_) {
        Elements deck_rows;
        const auto& deck = run_.DeckList();
        for (int i = 0; i < static_cast<int>(deck.size()); ++i) {
            const CardDef& card = GetCard(deck[static_cast<std::size_t>(i)]);
            deck_rows.push_back(hbox({
                text(i == removal_cursor_ ? "> " : "  ") | color(theme::kAccent) | bold,
                text(card.name),
            }));
        }
        body = vbox({
            text("SCRAP WHICH?") | bold | color(theme::kEnemy),
            text(""),
            vbox(std::move(deck_rows)),
        });
    }

    return vbox({
               hbox({
                   text(" BAD BEAT ") | bold | inverted,
                   text("  gold ") | color(theme::kMuted),
                   text(std::to_string(run_.Gold())) | bold | color(theme::kAccent),
                   text("   deck ") | color(theme::kMuted),
                   text(std::to_string(run_.DeckList().size())) | bold,
                   filler(),
                   text("shop ") | color(theme::kMuted),
               }),
               separator(),
               text("THE CAGE") | bold,
               text(""),
               std::move(body),
               filler(),
               separator(),
               hbox({
                   text(" ↑↓ ") | bold | color(theme::kAccent),
                   text("choose"),
                   text("   "),
                   text(" enter ") | bold | color(theme::kAccent),
                   text(removing_ ? "scrap it" : "buy / select"),
                   text("   "),
                   text(" esc ") | bold | color(theme::kAccent),
                   text(removing_ ? "back" : "—"),
                   filler(),
                   text(" q ") | bold | color(theme::kAccent),
                   text("quit"),
               }),
           }) |
           border;
}

}  // namespace bb::ui
