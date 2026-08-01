#include "ui/MapScreen.h"

#include <algorithm>
#include <utility>

#include "core/Grid.h"
#include "core/Rules.h"
#include "ui/Theme.h"

namespace bb::ui {
namespace {

// A node cell is "(C)" -- three characters -- plus a three-column gap for the
// connector to live in.
//
// The gap is deliberately narrow. An earlier version spaced columns eight apart
// and drew a single character halfway between two nodes, which left the
// connector floating in open space attached to nothing -- you could not tell
// which pair it joined. A connector has to visibly touch both ends.
constexpr int kCellWidth = 6;
constexpr int kRowHeight = 2;  // one node row plus one connector row

int ScreenX(int column) { return column * kCellWidth + 2; }

/// Rows are drawn bottom-up: the start sits at the bottom of the screen and the
/// boss at the top, so climbing the map reads as climbing.
int ScreenY(int row, int total_rows) { return (total_rows - 1 - row) * kRowHeight; }

ftxui::Color ColorFor(NodeType type) {
    switch (type) {
        case NodeType::Boss:   return theme::kDanger;
        case NodeType::Elite:  return theme::kThreat;
        case NodeType::Rest:   return theme::kPlayer;
        case NodeType::Shop:   return theme::kAccent;
        case NodeType::Event:  return theme::kBlock;
        case NodeType::Combat: break;
    }
    return ftxui::Color::White;
}

}  // namespace

std::string NodeGlyph(NodeType type) {
    switch (type) {
        case NodeType::Combat: return "C";
        case NodeType::Elite:  return "E";
        case NodeType::Rest:   return "R";
        case NodeType::Shop:   return "$";
        case NodeType::Event:  return "?";
        case NodeType::Boss:   return "B";
    }
    return "?";
}

std::string NodeName(NodeType type) {
    switch (type) {
        case NodeType::Combat: return "fight";
        case NodeType::Elite:  return "elite fight";
        case NodeType::Rest:   return "rest";
        case NodeType::Shop:   return "shop";
        case NodeType::Event:  return "event";
        case NodeType::Boss:   return "boss";
    }
    return "";
}

std::string NodeDescription(NodeType type) {
    switch (type) {
        case NodeType::Combat: return "An ordinary fight.";
        case NodeType::Elite:  return "Tougher, and it will cost you.";
        case NodeType::Rest:   return "Patch up before the next one.";
        case NodeType::Shop:   return "Spend what you have.";
        case NodeType::Event:  return "Something happens.";
        case NodeType::Boss:   return "The last hand.";
    }
    return "";
}

MapScreen::MapScreen(const RunState& run) : run_(run) {}

int MapScreen::Selection() const {
    const std::vector<int> options = run_.Options();
    if (options.empty()) return -1;

    const int clamped = std::clamp(option_, 0, static_cast<int>(options.size()) - 1);
    return options[static_cast<std::size_t>(clamped)];
}

void MapScreen::SelectNext() {
    const std::vector<int> options = run_.Options();
    if (options.empty()) return;
    option_ = (option_ + 1) % static_cast<int>(options.size());
}

void MapScreen::SelectPrevious() {
    const std::vector<int> options = run_.Options();
    if (options.empty()) return;

    const int count = static_cast<int>(options.size());
    option_ = (option_ - 1 + count) % count;
}

bool MapScreen::HandleEvent(ftxui::Event event) {
    using namespace ftxui;

    if (event == Event::ArrowRight || event == Event::Character('l') ||
        event == Event::ArrowDown || event == Event::Character('j')) {
        SelectNext();
        return true;
    }
    if (event == Event::ArrowLeft || event == Event::Character('h') ||
        event == Event::ArrowUp || event == Event::Character('k')) {
        SelectPrevious();
        return true;
    }
    return false;
}

ftxui::Element MapScreen::MapView() const {
    using namespace ftxui;

    const RunMap& map = run_.Map();
    const int rows = map.RowCount();

    const int height = rows * kRowHeight - 1;
    const int width = rules::kMapWidth * kCellWidth;

    // Everything is plotted into a character canvas first, with a parallel grid
    // recording which node each cell belongs to. Composing the picture and then
    // colouring it is far simpler than trying to assemble each terminal row out
    // of overlapping pieces -- the first attempt at that had every node in a row
    // overwriting its neighbours.
    Grid<char> canvas(width, height, ' ');
    Grid<int> owner(width, height, -1);

    for (const MapNode& node : map.Nodes()) {
        const int centre = ScreenX(node.column) + 1;
        const int y = ScreenY(node.row, rows);

        for (const int target : node.next) {
            const int to_centre = ScreenX(map.Node(target).column) + 1;
            const int gap_y = y - 1;
            if (gap_y < 0) continue;

            const auto plot = [&](int x, char glyph) {
                if (canvas.Contains({x, gap_y})) canvas.At({x, gap_y}) = glyph;
            };

            if (to_centre == centre) {
                plot(centre, '|');
                continue;
            }

            // Rows are drawn bottom-up, so a branch heading right also heads up
            // the screen and slopes like '/'. Getting this backwards drew every
            // fork mirrored, which is worse than drawing nothing.
            const char glyph = to_centre > centre ? '/' : '\\';

            // Fill the whole gap between the two cells so the line starts at one
            // node and finishes at the other.
            const int from_edge = std::min(centre, to_centre) + 2;
            const int to_edge = std::max(centre, to_centre) - 2;
            for (int x = from_edge; x <= to_edge; ++x) plot(x, glyph);
        }
    }

    const int selected = Selection();
    const std::vector<int> options = run_.Options();

    // Nodes go down last so they always sit on top of their connectors.
    for (int i = 0; i < map.NodeCount(); ++i) {
        const MapNode& node = map.Node(i);
        const int y = ScreenY(node.row, rows);
        const int x = ScreenX(node.column);

        const bool is_here = i == run_.CurrentNodeIndex();
        const bool is_selected = i == selected;

        const char open = is_selected ? '>' : (is_here ? '[' : '(');
        const char close = is_selected ? '<' : (is_here ? ']' : ')');
        const std::string glyph = NodeGlyph(node.type);

        const char cells[3] = {open, glyph.empty() ? '?' : glyph[0], close};
        for (int offset = 0; offset < 3; ++offset) {
            const Vec2 at{x + offset, y};
            if (!canvas.Contains(at)) continue;
            canvas.At(at) = cells[offset];
            owner.At(at) = i;
        }
    }

    Elements lines;
    for (int y = 0; y < height; ++y) {
        Elements cells;
        for (int x = 0; x < width; ++x) {
            const Vec2 at{x, y};
            const char glyph = canvas.At(at);
            const int node_index = owner.At(at);

            if (node_index < 0) {
                cells.push_back(text(std::string(1, glyph)) | color(theme::kMuted));
                continue;
            }

            const MapNode& node = map.Node(node_index);
            const bool is_here = node_index == run_.CurrentNodeIndex();
            const bool is_selected = node_index == selected;
            const bool is_reachable =
                std::find(options.begin(), options.end(), node_index) != options.end();
            const bool is_visited =
                std::find(run_.Visited().begin(), run_.Visited().end(), node_index) !=
                run_.Visited().end();

            // Brackets carry the cursor; the letter carries the node type.
            const bool is_bracket = (x == ScreenX(node.column)) || (x == ScreenX(node.column) + 2);

            Color paint = is_bracket ? theme::kMuted : ColorFor(node.type);
            if (is_bracket && is_selected) paint = theme::kAccent;
            if (is_bracket && is_here) paint = theme::kPlayer;

            Element cell = text(std::string(1, glyph)) | color(paint) | bold;

            // Somewhere already behind you, or unreachable from here, steps back
            // so the live choices are what the eye lands on.
            if (!is_selected && !is_here && (!is_reachable || is_visited)) cell = cell | dim;

            cells.push_back(std::move(cell));
        }
        lines.push_back(hbox(std::move(cells)));
    }

    return vbox(std::move(lines));
}

ftxui::Element MapScreen::DetailPanel() const {
    using namespace ftxui;

    const int selected = Selection();
    if (selected < 0) return text("nowhere left to go") | color(theme::kMuted);

    const MapNode& node = run_.Map().Node(selected);

    return vbox({
        hbox({
            text(NodeGlyph(node.type) + "  ") | color(ColorFor(node.type)) | bold,
            text(NodeName(node.type)) | bold,
        }),
        text(""),
        paragraph(NodeDescription(node.type)) | color(theme::kMuted),
        text(""),
        hbox({
            text("row ") | color(theme::kMuted),
            text(std::to_string(node.row + 1) + " of " + std::to_string(run_.Map().RowCount())),
        }),
    });
}

ftxui::Element MapScreen::Render() const {
    using namespace ftxui;

    const int options = static_cast<int>(run_.Options().size());

    return vbox({
               hbox({
                   text(" BAD BEAT ") | bold | inverted,
                   text("  health ") | color(theme::kMuted),
                   text(std::to_string(run_.Hp()) + "/" + std::to_string(run_.MaxHp())) |
                       bold |
                       color(run_.Hp() * 2 <= run_.MaxHp() ? theme::kEnemy : theme::kPlayer),
                   text("   deck ") | color(theme::kMuted),
                   text(std::to_string(run_.DeckList().size())) | bold,
                   filler(),
                   text((run_.CurrentNodeIndex() < 0
                             ? std::string("at the start")
                             : "row " + std::to_string(run_.CurrentRow()) + " of " +
                                   std::to_string(run_.Map().RowCount())) +
                        " ") |
                       color(theme::kMuted),
               }),
               separator(),
               text("ROUTE") | bold,
               text(""),
               hbox({
                   MapView(),
                   text("   "),
                   separator(),
                   text("   "),
                   vbox({
                       text("DESTINATION") | bold,
                       text(""),
                       DetailPanel(),
                   }) | size(WIDTH, EQUAL, 30),
               }),
               filler(),
               separator(),
               // Every node type the generator can emit needs a line here. Any
               // glyph the map can show and the legend cannot explain is a
               // question mark in the player's head at the exact moment they
               // are supposed to be making a decision.
               hbox({
                   text(NodeGlyph(NodeType::Combat)) | bold | color(ColorFor(NodeType::Combat)),
                   text(" fight   ") | color(theme::kMuted),
                   text(NodeGlyph(NodeType::Elite)) | bold | color(ColorFor(NodeType::Elite)),
                   text(" elite   ") | color(theme::kMuted),
                   text(NodeGlyph(NodeType::Rest)) | bold | color(ColorFor(NodeType::Rest)),
                   text(" rest   ") | color(theme::kMuted),
                   text(NodeGlyph(NodeType::Shop)) | bold | color(ColorFor(NodeType::Shop)),
                   text(" shop   ") | color(theme::kMuted),
                   text(NodeGlyph(NodeType::Event)) | bold | color(ColorFor(NodeType::Event)),
                   text(" event   ") | color(theme::kMuted),
                   text(NodeGlyph(NodeType::Boss)) | bold | color(ColorFor(NodeType::Boss)),
                   text(" boss") | color(theme::kMuted),
               }),
               separator(),
               hbox({
                   text(" ←→ ") | bold | color(theme::kAccent),
                   text(options > 1 ? "choose route" : "only one way on"),
                   text("   "),
                   text(" enter ") | bold | color(theme::kAccent),
                   text("travel"),
                   filler(),
                   text(" q ") | bold | color(theme::kAccent),
                   text("quit"),
               }),
           }) |
           border;
}

}  // namespace bb::ui
