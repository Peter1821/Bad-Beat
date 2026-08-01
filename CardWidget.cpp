#include "ui/widgets/CardWidget.h"

#include <string>
#include <utility>

#include "cards/CardDatabase.h"
#include "ui/Theme.h"

namespace bb::ui {
namespace {

constexpr int kCardWidth = 20;

}  // namespace

int CardWidth() { return kCardWidth; }

ftxui::Element RenderCard(const CardDef& card, int hotkey, bool selected, bool affordable) {
    using namespace ftxui;

    // Interior width, allowing for the border on each side.
    const int inner = kCardWidth - 2;

    const Color name_color = affordable ? Color::White : theme::kUnaffordable;
    const Color body_color = affordable ? theme::kMuted : theme::kUnaffordable;

    Element cost_badge = text(" " + std::to_string(card.cost) + " ") | bold |
                         (affordable ? color(theme::kAccent) | inverted
                                     : color(theme::kUnaffordable));

    Element face = vbox({
        hbox({
            std::move(cost_badge),
            text(" "),
            text(card.name) | bold | color(name_color) | flex,
            // The key that plays it. Shown on the card rather than only in the
            // help bar, so the mapping is where the player is already looking.
            text(std::to_string(hotkey)) | color(theme::kMuted),
        }),
        hbox({
            text(DescribeShape(card.pattern)) | color(theme::kThreat),
            filler(),
        }),
        separator(),
        paragraph(DescribeCard(card)) | color(body_color),
    });

    face = face | size(WIDTH, EQUAL, inner);

    Element framed = selected ? (face | borderHeavy | color(theme::kAccent))
                              : (face | border);

    // Fixed height so the hand row stays level whether or not a card's text
    // wraps onto a second line.
    return framed | size(HEIGHT, EQUAL, 7) | size(WIDTH, EQUAL, kCardWidth);
}

}  // namespace bb::ui
