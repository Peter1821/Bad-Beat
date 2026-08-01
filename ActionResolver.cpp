#include "combat/ActionResolver.h"

#include <algorithm>

#include "cards/CardDatabase.h"
#include "cards/Targeting.h"
#include "combat/EnemyAI.h"
#include "combat/Pathfinding.h"
#include "core/Rules.h"

namespace bb {

ActionResolver::ActionResolver(CombatState& state) : state_(state) {}

void ActionResolver::Emit(CombatEvent event) { state_.AppendEvent(event); }

void ActionResolver::ResolveDamage(ActorId target, int amount, ActorId source) {
    Actor* actor = state_.FindActor(target);
    if (actor == nullptr || !actor->IsAlive() || amount <= 0) return;

    const int dealt = actor->TakeDamage(amount);

    // Report what landed, not what was swung. A hit fully soaked by block is
    // still worth logging, so this fires even when `dealt` is zero.
    CombatEvent event;
    event.kind = CombatEventKind::Damaged;
    event.actor = target;
    event.source = source;
    event.amount = dealt;
    Emit(event);

    if (!actor->IsAlive()) {
        state_.RemoveFromField(target);

        CombatEvent death;
        death.kind = CombatEventKind::Died;
        death.actor = target;
        death.source = source;
        Emit(death);
    }
}

void ActionResolver::ResolveMove(ActorId id, Vec2 destination) {
    const auto origin = state_.Field().PositionOf(id);
    if (!origin) return;
    if (!state_.Field().MoveTo(id, destination)) return;

    CombatEvent moved;
    moved.kind = CombatEventKind::Moved;
    moved.actor = id;
    moved.from = *origin;
    moved.to = destination;
    Emit(moved);

    // Hazards bill on entry only, so this is the single place they can fire.
    if (state_.Field().TerrainAt(destination) == Terrain::Hazard) {
        CombatEvent burn;
        burn.kind = CombatEventKind::HazardBurn;
        burn.actor = id;
        burn.amount = rules::kHazardDamage;
        Emit(burn);

        ResolveDamage(id, rules::kHazardDamage, kNoActor);
    }
}

void ActionResolver::ResolveShove(ActorId target, Vec2 direction, int distance, ActorId source) {
    if (distance <= 0) return;

    const auto start = state_.Field().PositionOf(target);
    if (!start) return;

    int travelled = 0;
    Vec2 position = *start;

    for (int step = 0; step < distance; ++step) {
        const Vec2 next = position + direction;
        if (!state_.Field().IsWalkable(next)) break;
        position = next;
        ++travelled;
    }

    if (travelled > 0) {
        // Route through ResolveMove so a shove into a hazard triggers it, same
        // as walking in would.
        ResolveMove(target, position);

        CombatEvent pushed;
        pushed.kind = CombatEventKind::Pushed;
        pushed.actor = target;
        pushed.source = source;
        pushed.amount = travelled;
        Emit(pushed);
    }

    // Momentum that had nowhere to go becomes damage.
    const int absorbed = distance - travelled;
    if (absorbed > 0) {
        CombatEvent collision;
        collision.kind = CombatEventKind::Collided;
        collision.actor = target;
        collision.source = source;
        collision.amount = absorbed * rules::kCollisionDamagePerTile;
        Emit(collision);

        ResolveDamage(target, absorbed * rules::kCollisionDamagePerTile, source);
    }
}

void ActionResolver::ApplyEffect(const EffectOp& effect,
                                 ActorId caster,
                                 Vec2 aim,
                                 const std::vector<Vec2>& tiles) {
    switch (effect.kind) {
        case EffectKind::Damage:
            for (const Vec2 tile : tiles) {
                const Actor* victim = state_.ActorAt(tile);
                if (victim == nullptr) continue;

                // Same rule as enemy attacks: an area effect hits everything
                // standing in it except whoever threw it. A blast centred next
                // to yourself should not cost you HP.
                if (victim->Id() == caster) continue;

                ResolveDamage(victim->Id(), effect.amount, caster);
            }
            break;

        case EffectKind::Block:
            for (const Vec2 tile : tiles) {
                if (Actor* actor = state_.ActorAt(tile)) {
                    actor->AddBlock(effect.amount);

                    CombatEvent event;
                    event.kind = CombatEventKind::Blocked;
                    event.actor = actor->Id();
                    event.amount = effect.amount;
                    Emit(event);
                }
            }
            break;

        case EffectKind::Heal:
            for (const Vec2 tile : tiles) {
                if (Actor* actor = state_.ActorAt(tile)) {
                    actor->Heal(effect.amount);

                    CombatEvent event;
                    event.kind = CombatEventKind::Healed;
                    event.actor = actor->Id();
                    event.amount = effect.amount;
                    Emit(event);
                }
            }
            break;

        case EffectKind::MoveSelf:
            ResolveMove(caster, aim);
            break;

        case EffectKind::Push:
        case EffectKind::Pull: {
            const auto caster_tile = state_.Field().PositionOf(caster);
            if (!caster_tile) break;

            // Collect targets first: shoving one actor changes what is standing
            // on the remaining tiles, and iterating live occupancy while
            // mutating it would skip or double-hit people.
            std::vector<ActorId> targets;
            for (const Vec2 tile : tiles) {
                if (const Actor* actor = state_.ActorAt(tile)) targets.push_back(actor->Id());
            }

            for (const ActorId id : targets) {
                const auto target_tile = state_.Field().PositionOf(id);
                if (!target_tile) continue;

                Vec2 direction = DominantDirection(*caster_tile, *target_tile);
                if (effect.kind == EffectKind::Pull) direction = -direction;

                ResolveShove(id, direction, effect.amount, caster);
            }
            break;
        }

        case EffectKind::Draw: {
            state_.GetDeck().Draw(effect.amount);

            CombatEvent event;
            event.kind = CombatEventKind::CardsDrawn;
            event.actor = caster;
            event.amount = effect.amount;
            Emit(event);
            break;
        }

        case EffectKind::GainEnergy: {
            state_.GainEnergy(effect.amount);

            CombatEvent event;
            event.kind = CombatEventKind::EnergyGained;
            event.actor = caster;
            event.amount = effect.amount;
            Emit(event);
            break;
        }

        case EffectKind::SpawnHazard:
            for (const Vec2 tile : tiles) {
                if (state_.Field().TerrainAt(tile) == Terrain::Floor) {
                    state_.Field().SetTerrain(tile, Terrain::Hazard);
                }
            }
            break;
    }
}

bool ActionResolver::PlayCard(ActorId caster, int hand_index, Vec2 aim) {
    Deck& deck = state_.GetDeck();
    if (!deck.IsValidHandIndex(hand_index)) return false;

    const Actor* actor = state_.FindActor(caster);
    if (actor == nullptr || !actor->IsAlive()) return false;

    const auto caster_tile = state_.Field().PositionOf(caster);
    if (!caster_tile) return false;

    const CardDef& card = GetCard(deck.Hand()[static_cast<std::size_t>(hand_index)]);

    if (!state_.CanAfford(card.cost)) return false;
    if (!IsLegalAim(card.pattern, *caster_tile, aim, state_.Field())) return false;

    // Same function the preview called. If these ever diverge, a card hits
    // something other than what it highlighted.
    const std::vector<Vec2> tiles =
        ComputeAffectedTiles(card.pattern, *caster_tile, aim, state_.Field());

    state_.SpendEnergy(card.cost);

    CombatEvent played;
    played.kind = CombatEventKind::CardPlayed;
    played.actor = caster;
    played.card = card.id;
    played.to = aim;
    Emit(played);

    // Leave the hand before resolving, so an effect that draws cards cannot
    // shuffle the index out from under us mid-resolution.
    if (card.exhaust) {
        deck.ExhaustFromHand(hand_index);
    } else {
        deck.DiscardFromHand(hand_index);
    }

    for (const EffectOp& effect : card.effects) {
        ApplyEffect(effect, caster, aim, tiles);
    }

    return true;
}

void ActionResolver::ExecuteIntent(ActorId enemy) {
    const Actor* self = state_.FindActor(enemy);
    if (self == nullptr || !self->IsAlive()) return;

    const Intent intent = self->GetIntent();

    switch (intent.kind) {
        case IntentKind::Wait:
            break;

        case IntentKind::Move:
        case IntentKind::Retreat: {
            const auto from = state_.Field().PositionOf(enemy);
            if (!from) break;

            const ActorId target = ai::NearestPlayer(state_, *from);
            if (!IsValid(target)) break;

            const auto target_tile = state_.Field().PositionOf(target);
            if (!target_tile) break;

            // Movement threatens nothing, so it is free to re-path against
            // wherever the player actually ended up.
            const auto step = intent.kind == IntentKind::Retreat
                                  ? StepAwayFrom(state_, *from, *target_tile)
                                  : StepToward(state_, *from, *target_tile);
            if (step) ResolveMove(enemy, *step);
            break;
        }

        case IntentKind::Attack:
            // The locked tiles, not a fresh lookup. Standing somewhere else is
            // the whole defence.
            for (const Vec2 tile : intent.threatened) {
                const Actor* victim = state_.ActorAt(tile);
                if (victim == nullptr) continue;

                // An area attack catches whoever is standing in it, allies
                // included -- baiting a Slugger into its own side is a real
                // play. The one exception is the attacker: nothing hurts itself
                // with its own swing.
                if (victim->Id() == enemy) continue;

                ResolveDamage(victim->Id(), intent.amount, enemy);
            }
            break;

        case IntentKind::Defend: {
            Actor* mutable_self = state_.FindActor(enemy);
            if (mutable_self == nullptr) break;
            mutable_self->AddBlock(intent.amount);

            CombatEvent event;
            event.kind = CombatEventKind::Blocked;
            event.actor = enemy;
            event.amount = intent.amount;
            Emit(event);
            break;
        }
    }
}

bool ActionResolver::TakeFreeStep(ActorId id, Vec2 destination) {
    Actor* actor = state_.FindActor(id);
    if (actor == nullptr || !actor->IsAlive()) return false;
    if (actor->HasUsedFreeStep()) return false;

    const auto origin = state_.Field().PositionOf(id);
    if (!origin) return false;
    if (!IsOrthogonallyAdjacent(*origin, destination)) return false;
    if (!state_.Field().IsWalkable(destination)) return false;

    actor->MarkFreeStepUsed();
    ResolveMove(id, destination);
    return true;
}

}  // namespace bb
