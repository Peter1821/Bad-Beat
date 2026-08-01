#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace bb {

/// Integer coordinate on the battlefield grid.
///
/// Origin is top-left and +y points down, matching the order rows are drawn in
/// the terminal. Keeping screen order and simulation order identical removes a
/// whole class of "why is everything upside down" bugs in the renderer.
struct Vec2 {
    int x = 0;
    int y = 0;
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator-(Vec2 v) noexcept { return {-v.x, -v.y}; }
constexpr Vec2 operator*(Vec2 v, int scalar) noexcept { return {v.x * scalar, v.y * scalar}; }
constexpr bool operator==(Vec2 a, Vec2 b) noexcept { return a.x == b.x && a.y == b.y; }
constexpr bool operator!=(Vec2 a, Vec2 b) noexcept { return !(a == b); }

inline Vec2& operator+=(Vec2& a, Vec2 b) noexcept { return a = a + b; }
inline Vec2& operator-=(Vec2& a, Vec2 b) noexcept { return a = a - b; }

/// Row-major ordering, so sorted tile lists come out in reading order and any
/// container keyed on Vec2 iterates deterministically.
constexpr bool operator<(Vec2 a, Vec2 b) noexcept {
    return (a.y != b.y) ? (a.y < b.y) : (a.x < b.x);
}

constexpr int Abs(int value) noexcept { return value < 0 ? -value : value; }

/// Steps counted along the four orthogonal directions.
///
/// This is *the* distance metric for movement and card range in Bad Beat --
/// movement is orthogonal only, so a "range 2" card reaches a diamond, not a
/// square. Prefer this over ChebyshevDistance unless a shape is explicitly square.
constexpr int ManhattanDistance(Vec2 a, Vec2 b) noexcept {
    return Abs(a.x - b.x) + Abs(a.y - b.y);
}

/// Diagonals count as a single step, giving square rings. Used for blast/square
/// card shapes, never for movement.
constexpr int ChebyshevDistance(Vec2 a, Vec2 b) noexcept {
    const int dx = Abs(a.x - b.x);
    const int dy = Abs(a.y - b.y);
    return dx > dy ? dx : dy;
}

constexpr bool IsOrthogonallyAdjacent(Vec2 a, Vec2 b) noexcept {
    return ManhattanDistance(a, b) == 1;
}

/// Clamps a coordinate into [0, width) x [0, height).
///
/// Used to keep a moving cursor on the board. Clamping rather than rejecting
/// means holding an arrow key slides along the edge instead of sticking, which
/// is what a player expects.
constexpr Vec2 ClampToBounds(Vec2 p, int width, int height) noexcept {
    const int x = p.x < 0 ? 0 : (p.x > width - 1 ? width - 1 : p.x);
    const int y = p.y < 0 ? 0 : (p.y > height - 1 ? height - 1 : p.y);
    return {x, y};
}

inline constexpr Vec2 kNorth{0, -1};
inline constexpr Vec2 kEast{1, 0};
inline constexpr Vec2 kSouth{0, 1};
inline constexpr Vec2 kWest{-1, 0};

/// Clockwise from north. Iteration order is fixed so that anything derived from
/// it -- neighbour lists, AI tie-breaks -- is reproducible from a seed.
inline constexpr std::array<Vec2, 4> kOrthogonalSteps{kNorth, kEast, kSouth, kWest};

}  // namespace bb

namespace std {

template <>
struct hash<bb::Vec2> {
    size_t operator()(bb::Vec2 v) const noexcept {
        // Grids here are at most a few dozen tiles, so a cheap bit-mix is ample.
        const auto x = static_cast<uint32_t>(v.x);
        const auto y = static_cast<uint32_t>(v.y);
        return static_cast<size_t>((static_cast<uint64_t>(x) << 32) ^ y);
    }
};

}  // namespace std
