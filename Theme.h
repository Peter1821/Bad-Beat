#pragma once

#include <string>

#include <ftxui/screen/color.hpp>

#include "combat/Actor.h"
#include "combat/Terrain.h"

namespace bb::ui {

/// How one thing is drawn. The simulation says what a thing *is*; this layer
/// decides what it looks like, and it decides it here and nowhere else.
struct Appearance {
    std::string glyph;
    ftxui::Color color;
};

namespace theme {

inline const ftxui::Color kPlayer = ftxui::Color::Cyan;
inline const ftxui::Color kEnemy = ftxui::Color::Red;
inline const ftxui::Color kWall = ftxui::Color::GrayLight;
inline const ftxui::Color kHazard = ftxui::Color::Orange1;
inline const ftxui::Color kFloor = ftxui::Color::GrayDark;
inline const ftxui::Color kBlock = ftxui::Color::Blue;
inline const ftxui::Color kAccent = ftxui::Color::Yellow;
inline const ftxui::Color kMuted = ftxui::Color::GrayDark;

/// Empty portion of a gauge. Dark enough to read as background, light enough to
/// show the bar's full extent.
inline const ftxui::Color kTrack = ftxui::Color::Grey11;

/// Tiles a pending card would hit. Amber: this is *your* reach.
inline const ftxui::Color kThreat = ftxui::Color::Orange1;
inline const ftxui::Color kThreatFill = ftxui::Color::RGB(72, 48, 0);

/// Tiles an enemy has announced it will hit next turn.
///
/// Red, and deliberately not the same family as the amber aim highlight -- the
/// player has to tell "what I will hit" from "what will hit me" at a glance,
/// and those two answers appear on the board at the same time.
inline const ftxui::Color kDanger = ftxui::Color::Red;
inline const ftxui::Color kDangerFill = ftxui::Color::RGB(85, 0, 0);

/// Tiles a pending card could be aimed at. Faint on purpose -- it is context,
/// not the answer.
inline const ftxui::Color kRangeFill = ftxui::Color::Grey11;

/// A card that cannot be paid for right now.
inline const ftxui::Color kUnaffordable = ftxui::Color::GrayDark;

}  // namespace theme

Appearance AppearanceOf(Terrain terrain);
Appearance AppearanceOf(const Actor& actor);

ftxui::Color TeamColor(Team team);

std::string TerrainName(Terrain terrain);

/// Short description of what an enemy has telegraphed, e.g. "hits for 5".
std::string DescribeIntent(const Intent& intent);

}  // namespace bb::ui
