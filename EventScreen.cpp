#include "ui/EventScreen.h"

#include <utility>

#include "ui/Theme.h"

namespace bb::ui {

EventScreen::EventScreen(const EventDef& definition) : definition_(definition) {}

void EventScreen::SelectNext() {
    if (chosen_ || definition_.options.empty()) return;
    cursor_ = (cursor_ + 1) % static_cast<int>(definition_.options.size());
}

void EventScreen::SelectPrevious() {
    if (chosen_ || definition_.options.empty()) return;
    const int count = static_cast<int>(definition_.options.size());
    cursor_ = (cursor_ - 1 + count) % count;
}

void EventScreen::Choose() {
    if (definition_.options.empty()) return;
    chosen_ = true;
}

const EventOption* EventScreen::Chosen() const {
    if (!chosen_ || definition_.options.empty()) return nullptr;
    return &definition_.options[static_cast<std::size_t>(cursor_)];
}

bool EventScreen::HandleEvent(ftxui::Event event) {
    using namespace ftxui;

    if (chosen_) return false;

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        SelectNext();
        return true;
    }
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        SelectPrevious();
        return true;
    }

    for (int i = 1; i <= static_cast<int>(definition_.options.size()); ++i) {
        if (event == Event::Character(static_cast<char>('0' + i))) {
            cursor_ = i - 1;
            return true;
        }
    }
    return false;
}

ftxui::Element EventScreen::Render() const {
    using namespace ftxui;

    Elements body{
        text(definition_.title) | bold | color(theme::kBlock),
        text(""),
        paragraph(definition_.body) | color(theme::kMuted),
        text(""),
        separator(),
        text(""),
    };

    if (const EventOption* taken = Chosen()) {
        body.push_back(text(taken->label) | bold);
        body.push_back(text(""));
        body.push_back(paragraph(taken->result) | color(theme::kPlayer));
    } else {
        for (int i = 0; i < static_cast<int>(definition_.options.size()); ++i) {
            const EventOption& option = definition_.options[static_cast<std::size_t>(i)];
            const bool here = i == cursor_;

            body.push_back(hbox({
                text(here ? "> " : "  ") | color(theme::kAccent) | bold,
                text(std::to_string(i + 1) + ". ") | color(theme::kMuted),
                text(option.label) | (here ? bold : nothing),
            }));
        }
    }

    return vbox({
               hbox({text(" BAD BEAT ") | bold | inverted, filler(),
                     text("event ") | color(theme::kMuted)}),
               separator(),
               filler(),
               hbox({filler(), vbox(std::move(body)) | size(WIDTH, EQUAL, 64), filler()}),
               filler(),
               separator(),
               hbox({
                   text(" ↑↓ ") | bold | color(theme::kAccent),
                   text("choose"),
                   text("   "),
                   text(" enter ") | bold | color(theme::kAccent),
                   text(chosen_ ? "move on" : "commit"),
                   filler(),
                   text(" q ") | bold | color(theme::kAccent),
                   text("quit"),
               }),
           }) |
           border;
}

}  // namespace bb::ui
