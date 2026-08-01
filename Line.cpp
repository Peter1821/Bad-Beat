#include "core/Line.h"

#include <algorithm>

namespace bb {
namespace {

/// Standard integer Bresenham, always walked from `from` to `to`.
std::vector<Vec2> Walk(Vec2 from, Vec2 to) {
    std::vector<Vec2> tiles;

    int x = from.x;
    int y = from.y;

    const int dx = Abs(to.x - from.x);
    const int dy = -Abs(to.y - from.y);
    const int step_x = from.x < to.x ? 1 : -1;
    const int step_y = from.y < to.y ? 1 : -1;

    int error = dx + dy;

    while (true) {
        tiles.push_back({x, y});
        if (x == to.x && y == to.y) break;

        const int doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x += step_x;
        }
        if (doubled <= dx) {
            error += dx;
            y += step_y;
        }
    }

    return tiles;
}

}  // namespace

std::vector<Vec2> LineTiles(Vec2 a, Vec2 b) {
    // Walk from the lexicographically smaller endpoint so the tile set never
    // depends on which way round the caller asked, then flip if needed.
    const bool reversed = b < a;

    std::vector<Vec2> tiles = reversed ? Walk(b, a) : Walk(a, b);
    if (reversed) std::reverse(tiles.begin(), tiles.end());

    return tiles;
}

}  // namespace bb
