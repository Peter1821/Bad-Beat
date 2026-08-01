#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace bb {

/// Seeded random source.
///
/// Every random decision in a run -- map layout, enemy placement, card rewards,
/// shuffles -- draws from one of these, never from a global generator. The seed
/// is kept alongside the engine so a run can be replayed or shared, which is
/// what makes a roguelike bug reproducible instead of a ghost story.
class Rng {
public:
    explicit Rng(std::uint64_t seed);

    /// Seeds from the OS entropy source. Called once when a run begins; the
    /// resulting Rng is then threaded down to whatever needs it.
    static Rng FromEntropy();

    std::uint64_t Seed() const noexcept { return seed_; }

    /// Uniform in [min_inclusive, max_inclusive]. Both ends are included.
    int Range(int min_inclusive, int max_inclusive);

    /// True with the given percentage chance. 0 or less never fires, 100 or more
    /// always does, and neither consumes a draw.
    bool Chance(int percent);

    /// A fresh 64-bit seed, for spawning a derived generator.
    ///
    /// Used to start a new run from the previous one, so "play again" gives a
    /// genuinely different game while the whole chain stays reproducible from
    /// the first seed.
    std::uint64_t NextSeed();

    /// Uniformly selects one element. The container must not be empty.
    template <typename T>
    const T& Pick(const std::vector<T>& values) {
        assert(!values.empty());
        return values[static_cast<std::size_t>(Range(0, static_cast<int>(values.size()) - 1))];
    }

    /// In-place Fisher-Yates via the seeded engine, so a deck shuffled from a
    /// given seed always comes out in the same order.
    template <typename T>
    void Shuffle(std::vector<T>& values) {
        std::shuffle(values.begin(), values.end(), engine_);
    }

private:
    std::uint64_t seed_;
    std::mt19937_64 engine_;
};

}  // namespace bb
