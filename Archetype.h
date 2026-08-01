#pragma once

#include <cstdint>

namespace bb {

/// What kind of thing an actor is.
///
/// Covers player and enemy kinds in one enum rather than tagging enemies
/// separately, so anything that varies by unit type -- glyph, colour, AI
/// abilities -- has exactly one key to look up.
enum class Archetype : std::uint8_t {
    Drifter,   ///< The player.
    ChipRat,   ///< Closes and bites. Teaches: kill it or step away.
    Spitter,   ///< Shoots from range, keeps its distance. Teaches: break sight.
    Slugger,   ///< Slow, telegraphs a wide slam. Teaches: read the shading.
    TheHouse,  ///< The boss. Sweeps an entire row, through cover.
    Count,
};

}  // namespace bb
