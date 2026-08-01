#pragma once

#include <string>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "run/EventDatabase.h"

namespace bb::ui {

/// A text choice between fights.
///
/// Every option costs something. The outcome is narrated after choosing rather
/// than applied silently, so an event reads as a decision with a consequence
/// instead of a slot machine.
class EventScreen {
public:
    explicit EventScreen(const EventDef& definition);

    bool HandleEvent(ftxui::Event event);
    ftxui::Element Render() const;

    void SelectNext();
    void SelectPrevious();

    /// Locks in the highlighted option and shows its outcome.
    void Choose();

    /// The option taken, or nullptr before choosing.
    const EventOption* Chosen() const;

    /// True once an option has been taken and its outcome read.
    bool IsResolved() const noexcept { return chosen_; }

private:
    const EventDef& definition_;
    int cursor_ = 0;
    bool chosen_ = false;
};

}  // namespace bb::ui
