#include "combat/CombatEvent.h"

#include "cards/CardDatabase.h"
#include "combat/CombatState.h"

namespace bb {
namespace {

std::string NameOf(ActorId id, const CombatState& state) {
    const Actor* actor = state.FindActor(id);
    return actor != nullptr ? actor->Name() : "someone";
}

}  // namespace

std::string DescribeEvent(const CombatEvent& event, const CombatState& state) {
    const std::string who = NameOf(event.actor, state);
    const std::string amount = std::to_string(event.amount);

    switch (event.kind) {
        case CombatEventKind::TurnBegan:
            return "-- turn " + amount + " --";

        case CombatEventKind::CardPlayed:
            return who + " plays " + GetCard(event.card).name;

        case CombatEventKind::Damaged:
            return who + " takes " + amount + " damage";

        case CombatEventKind::Blocked:
            return who + " gains " + amount + " block";

        case CombatEventKind::Healed:
            return who + " heals " + amount;

        case CombatEventKind::Moved:
            return who + " moves";

        case CombatEventKind::Pushed:
            return who + " is shoved " + amount +
                   (event.amount == 1 ? " tile" : " tiles");

        case CombatEventKind::HazardBurn:
            return who + " steps into the hazard";

        case CombatEventKind::Collided:
            return who + " slams into cover";

        case CombatEventKind::Died:
            return who + " is down";

        case CombatEventKind::CardsDrawn:
            return "drew " + amount;

        case CombatEventKind::EnergyGained:
            return "gained " + amount + " energy";

        case CombatEventKind::TurnEnded:
            return "turn ends";
    }
    return {};
}

}  // namespace bb
