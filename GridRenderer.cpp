#include "ui/widgets/GridRenderer.h"

#include <algorithm>
#include <string>
#include <utility>

#include "ui/Theme.h"

namespace bb::ui {
namespace {

bool Holds(const std::vector<Vec2>& tiles, Vec2 wanted) {
    return std::find(tiles.begin(), tiles.end(), wanted) != tiles.end();
}

/// One horizontal rule of the grid, e.g. "├───┼───┼ ... ┤".
std::string HorizontalRule(int width,
                           const std::string& left,
                           const std::string& middle,
                           const std::string& right) {
    std::string out = left;
    for (int x = 0; x < width; ++x) {
        out += "───";
        out += (x + 1 < width) ? middle : right;
    }
    return out;
}

Appearance AppearanceAt(const CombatState& state, Vec2 tile) {
    if (const Actor* actor = state.ActorAt(tile)) return AppearanceOf(*actor);
    return AppearanceOf(state.Field().TerrainAt(tile));
}

}  // namespace

ftxui::Element RenderBattlefield(const CombatState& state, const GridOverlay& overlay) {
    using namespace ftxui;

    const Battlefield& field = state.Field();

    Elements rows;
    rows.push_back(text(HorizontalRule(field.Width(), "┌", "┬", "┐")));

    for (int y = 0; y < field.Height(); ++y) {
        Elements cells;
        for (int x = 0; x < field.Width(); ++x) {
            const Vec2 tile{x, y};
            const Appearance look = AppearanceAt(state, tile);
            const bool under_cursor = overlay.cursor.has_value() && *overlay.cursor == tile;
            const bool affected = Holds(overlay.affected, tile);
            const bool in_range = Holds(overlay.in_range, tile);
            const bool threatened = Holds(overlay.threatened, tile);

            cells.push_back(text("│"));

            // Every state carries a text marker as well as a colour. Two
            // reasons: it survives a colour-blind player or a monochrome
            // terminal, and it makes the board verifiable from a text dump.
            //
            // Brackets are exclusive -- a cell has room for one pair -- so they
            // rank by what the player most needs to know: where the cursor is,
            // then what their card hits, then what is about to hit them. The
            // background colours layer underneath and are not exclusive, so a
            // tile that is both aimed at and dangerous still reads as dangerous.
            Element cell;
            if (under_cursor) {
                // The cursor sits on the player by default, so the one tile the
                // player most needs to see marked as dangerous is the one whose
                // marker the cursor hides. Recolour the brackets instead of
                // losing the warning.
                const Color bracket = threatened ? theme::kDanger : theme::kAccent;
                cell = hbox({
                    text("[") | color(bracket) | bold,
                    text(look.glyph) | color(look.color) | bold,
                    text("]") | color(bracket) | bold,
                });
            } else if (affected) {
                cell = hbox({
                    text("<") | color(theme::kThreat) | bold,
                    text(look.glyph) | color(look.color) | bold,
                    text(">") | color(theme::kThreat) | bold,
                });
            } else if (threatened) {
                cell = hbox({
                    text("!") | color(theme::kDanger) | bold,
                    text(look.glyph) | color(look.color) | bold,
                    text("!") | color(theme::kDanger) | bold,
                });
            } else {
                cell = text(" " + look.glyph + " ") | color(look.color);
            }

            // Danger outranks the aim tint: being about to take 7 damage
            // matters more than where your own card would land.
            if (threatened) {
                cell = cell | bgcolor(theme::kDangerFill);
            } else if (affected) {
                cell = cell | bgcolor(theme::kThreatFill);
            } else if (in_range) {
                cell = cell | bgcolor(theme::kRangeFill);
            }

            cells.push_back(std::move(cell));
        }
        cells.push_back(text("│"));
        rows.push_back(hbox(std::move(cells)));

        if (y + 1 < field.Height()) {
            rows.push_back(text(HorizontalRule(field.Width(), "├", "┼", "┤")));
        }
    }

    rows.push_back(text(HorizontalRule(field.Width(), "└", "┴", "┘")));
    return vbox(std::move(rows));
}

}  // namespace bb::ui
