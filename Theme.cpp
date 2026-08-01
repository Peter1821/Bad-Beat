#include "ui/Theme.h"

namespace bb::ui {

Appearance AppearanceOf(Terrain terrain) {
    switch (terrain) {
        case Terrain::Wall:   return {"#", theme::kWall};
        case Terrain::Hazard: return {"~", theme::kHazard};
        case Terrain::Floor:  break;
    }
    return {"·", theme::kFloor};
}

Appearance AppearanceOf(const Actor& actor) {
    // A distinct letter per archetype, so the board says which threat is which
    // without the player having to move the cursor over each one.
    switch (actor.GetArchetype()) {
        case Archetype::Drifter: return {"@", theme::kPlayer};
        case Archetype::ChipRat: return {"r", theme::kEnemy};
        case Archetype::Spitter: return {"s", theme::kEnemy};
        case Archetype::Slugger: return {"S", theme::kEnemy};
        case Archetype::TheHouse: return {"H", theme::kDanger};
        case Archetype::Count:   break;
    }
    return {"?", theme::kEnemy};
}

std::string DescribeIntent(const Intent& intent) {
    switch (intent.kind) {
        case IntentKind::Attack: return "hits for " + std::to_string(intent.amount);
        case IntentKind::Defend: return "guards " + std::to_string(intent.amount);
        case IntentKind::Move:    return "closing in";
        case IntentKind::Retreat: return "backing off";
        case IntentKind::Wait:    return "waiting";
    }
    return {};
}

ftxui::Color TeamColor(Team team) {
    return team == Team::Player ? theme::kPlayer : theme::kEnemy;
}

std::string TerrainName(Terrain terrain) {
    switch (terrain) {
        case Terrain::Wall:   return "wall";
        case Terrain::Hazard: return "hazard";
        case Terrain::Floor:  break;
    }
    return "floor";
}

}  // namespace bb::ui
