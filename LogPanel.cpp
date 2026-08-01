#include "ui/widgets/LogPanel.h"

#include <algorithm>
#include <string>
#include <utility>

#include "ui/Theme.h"

namespace bb::ui {
namespace {

ftxui::Color ColorFor(CombatEventKind kind) {
    switch (kind) {
        case CombatEventKind::Damaged:
        case CombatEventKind::Died:
            return theme::kEnemy;

        case CombatEventKind::Blocked:
            return theme::kBlock;

        case CombatEventKind::HazardBurn:
        case CombatEventKind::Collided:
            return theme::kHazard;

        case CombatEventKind::CardPlayed:
        case CombatEventKind::TurnBegan:
            return theme::kAccent;

        case CombatEventKind::Healed:
            return theme::kPlayer;

        default:
            return theme::kMuted;
    }
}

}  // namespace

ftxui::Element RenderLog(const CombatState& state, int max_lines) {
    using namespace ftxui;

    const auto& log = state.Log();

    const int total = static_cast<int>(log.size());
    const int first = std::max(0, total - max_lines);

    Elements lines;
    for (int i = first; i < total; ++i) {
        const CombatEvent& event = log[static_cast<std::size_t>(i)];

        Element line = text(DescribeEvent(event, state)) | color(ColorFor(event.kind));
        if (event.kind == CombatEventKind::Died) line = line | bold;

        lines.push_back(std::move(line));
    }

    // Keep the panel a fixed height so the hand below it never shifts as events
    // accumulate -- a jumping layout is far more distracting than empty space.
    while (static_cast<int>(lines.size()) < max_lines) {
        lines.insert(lines.begin(), text(""));
    }

    return vbox(std::move(lines));
}

}  // namespace bb::ui
