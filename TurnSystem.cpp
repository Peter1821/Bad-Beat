#include "combat/TurnSystem.h"

#include <utility>
#include <vector>

#include "combat/ActionResolver.h"
#include "combat/EnemyAI.h"
#include "core/Rules.h"

namespace bb {

TurnSystem::TurnSystem(CombatState& state) : state_(state) {}

bool TurnSystem::IsFightOver() const {
    return state_.IsTeamDefeated(Team::Player) || state_.IsTeamDefeated(Team::Enemy);
}

bool TurnSystem::PlayerWon() const {
    return state_.IsTeamDefeated(Team::Enemy) && !state_.IsTeamDefeated(Team::Player);
}

void TurnSystem::BeginFight() {
    // Telegraph before the first player turn, so turn one already shows what is
    // coming. A fight that opens with no visible threat teaches the player that
    // intents are optional reading.
    TelegraphIntents();
    BeginPlayerTurn();
}

void TurnSystem::BeginPlayerTurn() {
    // Block clears at the *start* of your turn rather than the end, so block
    // spent on defence protects you through the enemy phase. That timing is the
    // only one that makes a defensive card worth a slot.
    for (Actor& actor : state_.ActorsMutable()) {
        if (actor.GetTeam() != Team::Player) continue;
        actor.ClearBlock();
        actor.ResetFreeStep();
    }

    state_.RefillEnergy();
    state_.GetDeck().DrawTo(rules::kCardsDrawnPerTurn);

    CombatEvent event;
    event.kind = CombatEventKind::TurnBegan;
    event.amount = state_.TurnNumber();
    state_.AppendEvent(event);
}

void TurnSystem::RunEnemyPhase() {
    ActionResolver resolver(state_);

    // Snapshot the roster first. Executing an intent can kill an actor, and
    // iterating the live roster while it changes underneath is how one enemy
    // ends up acting twice or not at all.
    std::vector<ActorId> acting;
    for (const Actor& actor : state_.Actors()) {
        if (actor.GetTeam() == Team::Enemy && actor.IsAlive()) acting.push_back(actor.Id());
    }

    for (const ActorId id : acting) {
        const Actor* actor = state_.FindActor(id);
        if (actor == nullptr || !actor->IsAlive()) continue;  // died earlier this phase
        resolver.ExecuteIntent(id);
    }

    for (Actor& actor : state_.ActorsMutable()) {
        if (actor.GetTeam() != Team::Enemy) continue;
        actor.ClearBlock();
        actor.ResetFreeStep();
    }

    TelegraphIntents();
}

void TurnSystem::TelegraphIntents() {
    // Chosen after everything has moved, so what the player sees at the start of
    // their turn reflects the board they are actually looking at.
    std::vector<ActorId> enemies;
    for (const Actor& actor : state_.Actors()) {
        if (actor.GetTeam() == Team::Enemy && actor.IsAlive()) enemies.push_back(actor.Id());
    }

    for (const ActorId id : enemies) {
        Intent intent = ai::ChooseIntent(state_, id);
        if (Actor* actor = state_.FindActor(id)) actor->SetIntent(std::move(intent));
    }
}

void TurnSystem::EndPlayerTurn() {
    if (IsFightOver()) return;

    // Unplayed cards are discarded rather than kept, so each turn is a
    // self-contained puzzle instead of a hoarding exercise -- the same reason
    // unspent energy is lost.
    state_.GetDeck().DiscardHand();

    CombatEvent ended;
    ended.kind = CombatEventKind::TurnEnded;
    state_.AppendEvent(ended);

    RunEnemyPhase();

    if (IsFightOver()) return;

    state_.AdvanceTurn();
    BeginPlayerTurn();
}

}  // namespace bb
