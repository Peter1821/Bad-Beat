#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>

#include "core/Vec2.h"

namespace bb {

/// Fixed-size 2D array addressed by Vec2, stored row-major.
///
/// Dimensions are set at construction and never change -- a battlefield does not
/// resize mid-fight -- which keeps indexing branch-free and references stable.
///
/// Storage is a plain heap array rather than std::vector, specifically so that
/// Grid<bool> works. std::vector<bool> is the bit-packed specialisation: its
/// operator[] hands back a proxy object rather than a reference, so At() cannot
/// return T& and the whole template fails to compile the moment anyone reaches
/// for the obvious "mark visited tiles" grid.
template <typename T>
class Grid {
public:
    Grid(int width, int height, const T& initial = T{})
        : width_(width),
          height_(height),
          cells_(std::make_unique<T[]>(static_cast<std::size_t>(width) *
                                       static_cast<std::size_t>(height))) {
        assert(width > 0 && height > 0);
        std::fill_n(cells_.get(), Count(), initial);
    }

    Grid(const Grid& other)
        : width_(other.width_),
          height_(other.height_),
          cells_(std::make_unique<T[]>(other.Count())) {
        std::copy_n(other.cells_.get(), other.Count(), cells_.get());
    }

    Grid& operator=(const Grid& other) {
        if (this == &other) return *this;

        Grid copy(other);
        *this = std::move(copy);
        return *this;
    }

    Grid(Grid&&) noexcept = default;
    Grid& operator=(Grid&&) noexcept = default;

    int Width() const noexcept { return width_; }
    int Height() const noexcept { return height_; }

    bool Contains(Vec2 p) const noexcept {
        return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_;
    }

    /// Unchecked in release, asserts in debug. Use when the caller has already
    /// established that p is on the grid.
    T& At(Vec2 p) {
        assert(Contains(p));
        return cells_[Index(p)];
    }
    const T& At(Vec2 p) const {
        assert(Contains(p));
        return cells_[Index(p)];
    }

    /// Bounds-safe accessor returning nullptr off-grid.
    ///
    /// Preferred at the edges of the simulation, where out-of-range coordinates
    /// are routine rather than exceptional: a card shape probing outward from a
    /// border tile, or a push that would shove a target off the field.
    T* TryAt(Vec2 p) noexcept { return Contains(p) ? &cells_[Index(p)] : nullptr; }
    const T* TryAt(Vec2 p) const noexcept {
        return Contains(p) ? &cells_[Index(p)] : nullptr;
    }

    void Fill(const T& value) { std::fill_n(cells_.get(), Count(), value); }

    /// Visits every cell in row-major (reading) order as f(Vec2, const T&).
    template <typename F>
    void ForEach(F&& f) const {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const Vec2 p{x, y};
                f(p, cells_[Index(p)]);
            }
        }
    }

private:
    std::size_t Count() const noexcept {
        return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    }

    std::size_t Index(Vec2 p) const noexcept {
        return static_cast<std::size_t>(p.y) * static_cast<std::size_t>(width_) +
               static_cast<std::size_t>(p.x);
    }

    int width_;
    int height_;
    std::unique_ptr<T[]> cells_;
};

}  // namespace bb
