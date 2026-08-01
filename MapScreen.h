#pragma once

#include <string>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "run/RunState.h"

namespace bb::ui {

/// The route view: where you are, where you can go, and what is waiting there.
///
/// Node types are visible on branches you have not taken. That is deliberate --
/// a choice between three unknowns is a coin flip, not a decision.
class MapScreen {
public:
    explicit MapScreen(const RunState& run);

    /// Handles one key. Returns false for anything it does not use.
    bool HandleEvent(ftxui::Event event);

    ftxui::Element Render() const;

    /// Index of the node the player has highlighted, or -1 if there is nothing
    /// to choose.
    int Selection() const;

    void SelectNext();
    void SelectPrevious();

private:
    ftxui::Element MapView() const;
    ftxui::Element DetailPanel() const;

    const RunState& run_;

    /// Position within the list of reachable nodes, not a node index -- the
    /// options change every row, and an index would go stale.
    int option_ = 0;
};

/// Single letter shown on the map for a node type.
std::string NodeGlyph(NodeType type);

/// Short name, e.g. "elite fight".
std::string NodeName(NodeType type);

/// One line explaining what entering it does.
std::string NodeDescription(NodeType type);

}  // namespace bb::ui
