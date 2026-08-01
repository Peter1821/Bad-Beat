#include "core/Rng.h"

namespace bb {

Rng::Rng(std::uint64_t seed) : seed_(seed), engine_(seed) {}

Rng Rng::FromEntropy() {
    std::random_device device;
    // random_device yields 32 bits per call on MSVC, so two calls are needed to
    // fill a 64-bit seed. Without this the seed space is only 2^32.
    const auto high = static_cast<std::uint64_t>(device());
    const auto low = static_cast<std::uint64_t>(device());
    return Rng((high << 32) ^ low);
}

int Rng::Range(int min_inclusive, int max_inclusive) {
    assert(min_inclusive <= max_inclusive);
    std::uniform_int_distribution<int> distribution(min_inclusive, max_inclusive);
    return distribution(engine_);
}

std::uint64_t Rng::NextSeed() { return engine_(); }

bool Rng::Chance(int percent) {
    if (percent <= 0) return false;
    if (percent >= 100) return true;
    return Range(1, 100) <= percent;
}

}  // namespace bb
