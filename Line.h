#pragma once

#include <vector>

#include "core/Vec2.h"

namespace bb {

/// Every tile a straight line from `a` to `b` passes through, inclusive of both
/// endpoints, ordered from `a` to `b`.
///
/// Symmetry is guaranteed: LineTiles(a, b) is always the exact reverse of
/// LineTiles(b, a). Plain Bresenham does not give you this -- it breaks ties by
/// stepping order, so a line drawn one way can clip a different tile than the
/// same line drawn the other way. That would produce "I can see you but you
/// cannot see me", which players notice immediately and report as a bug. This
/// implementation walks from a canonical endpoint and reverses when needed.
std::vector<Vec2> LineTiles(Vec2 a, Vec2 b);

}  // namespace bb
