#include "cards/CardDatabase.h"

#include <array>
#include <cassert>

namespace bb {
namespace {

using Cards = std::array<CardDef, static_cast<std::size_t>(CardId::Count)>;

Cards BuildCards() {
    Cards cards;

    auto& jab = cards[static_cast<std::size_t>(CardId::Jab)];
    jab = {CardId::Jab, "jab", "Jab", 1, Rarity::Starter,
           TargetPattern{OriginRule::AdjacentTile, 1, Shape::Single, 1, true},
           {{EffectKind::Damage, 5}},
           "Nothing clever."};

    auto& fold = cards[static_cast<std::size_t>(CardId::Fold)];
    fold = {CardId::Fold, "fold", "Fold", 1, Rarity::Starter,
            TargetPattern{OriginRule::Self, 0, Shape::Single, 1, false},
            {{EffectKind::Block, 5}},
            "Live to see the river."};

    auto& sidestep = cards[static_cast<std::size_t>(CardId::Sidestep)];
    sidestep = {CardId::Sidestep, "sidestep", "Sidestep", 1, Rarity::Starter,
                TargetPattern{OriginRule::WalkableTileInRange, 3, Shape::Single, 1, true},
                {{EffectKind::MoveSelf, 0}},
                "The board was the problem."};

    auto& sweep = cards[static_cast<std::size_t>(CardId::Sweep)];
    sweep = {CardId::Sweep, "sweep", "Sweep", 2, Rarity::Starter,
             TargetPattern{OriginRule::Direction, 0, Shape::Cone, 2, true},
             {{EffectKind::Damage, 4}},
             "Take the whole pot."};

    auto& long_shot = cards[static_cast<std::size_t>(CardId::LongShot)];
    long_shot = {CardId::LongShot, "long_shot", "Long Shot", 1, Rarity::Common,
                 TargetPattern{OriginRule::OccupiedTileInRange, 4, Shape::Single, 1, true},
                 {{EffectKind::Damage, 4}},
                 "Needs a clean line."};

    auto& split_pot = cards[static_cast<std::size_t>(CardId::SplitPot)];
    split_pot = {CardId::SplitPot, "split_pot", "Split Pot", 2, Rarity::Common,
                 TargetPattern{OriginRule::AnyTileInRange, 3, Shape::Diamond, 1, true},
                 {{EffectKind::Damage, 3}},
                 "Everyone gets a share."};

    auto& shove = cards[static_cast<std::size_t>(CardId::Shove)];
    shove = {CardId::Shove, "shove", "Shove", 1, Rarity::Common,
             TargetPattern{OriginRule::AdjacentTile, 1, Shape::Single, 1, true},
             {{EffectKind::Push, 2}},
             "Terrain does the rest."};

    auto& check = cards[static_cast<std::size_t>(CardId::Check)];
    check = {CardId::Check, "check", "Check", 0, Rarity::Common,
             TargetPattern{OriginRule::Self, 0, Shape::Single, 1, false},
             {{EffectKind::Draw, 1}},
             "See one more."};

    auto& backhand = cards[static_cast<std::size_t>(CardId::Backhand)];
    backhand = {CardId::Backhand, "backhand", "Backhand", 1, Rarity::Common,
                TargetPattern{OriginRule::OccupiedTileInRange, 3, Shape::Single, 1, true},
                {{EffectKind::Pull, 2}},
                "Come here."};

    auto& chip_up = cards[static_cast<std::size_t>(CardId::ChipUp)];
    chip_up = {CardId::ChipUp, "chip_up", "Chip Up", 1, Rarity::Common,
               TargetPattern{OriginRule::Self, 0, Shape::Single, 1, false},
               {{EffectKind::Block, 3}, {EffectKind::Draw, 1}},
               "Grind it back."};

    auto& cooler = cards[static_cast<std::size_t>(CardId::Cooler)];
    cooler = {CardId::Cooler, "cooler", "Cooler", 2, Rarity::Uncommon,
              TargetPattern{OriginRule::AnyTileInRange, 3, Shape::Single, 1, true},
              {{EffectKind::SpawnHazard, 0}},
              "Make the floor the problem."};

    auto& raise = cards[static_cast<std::size_t>(CardId::Raise)];
    raise = {CardId::Raise, "raise", "Raise", 1, Rarity::Uncommon,
             TargetPattern{OriginRule::Self, 0, Shape::Single, 1, false},
             {{EffectKind::GainEnergy, 3}, {EffectKind::Draw, 1}},
             "Push it all in.",
             /*exhaust=*/true};

    auto& pocket_aces = cards[static_cast<std::size_t>(CardId::PocketAces)];
    pocket_aces = {CardId::PocketAces, "pocket_aces", "Pocket Aces", 2, Rarity::Rare,
                   TargetPattern{OriginRule::AdjacentTile, 1, Shape::Single, 1, true},
                   {{EffectKind::Damage, 12}},
                   "Best hand you will ever hold.",
                   /*exhaust=*/true};

    auto& the_nuts = cards[static_cast<std::size_t>(CardId::TheNuts)];
    the_nuts = {CardId::TheNuts, "the_nuts", "The Nuts", 3, Rarity::Rare,
                TargetPattern{OriginRule::Direction, 0, Shape::Line, 4, true},
                {{EffectKind::Damage, 6}},
                "Unbeatable, until it isn't.",
                /*exhaust=*/true};

    return cards;
}

const Cards& AllCards() {
    static const Cards cards = BuildCards();
    return cards;
}

std::string DescribeEffect(const EffectOp& effect) {
    switch (effect.kind) {
        case EffectKind::Damage:      return "Deal " + std::to_string(effect.amount) + " damage.";
        case EffectKind::Block:       return "Gain " + std::to_string(effect.amount) + " block.";
        case EffectKind::Heal:        return "Heal " + std::to_string(effect.amount) + ".";
        case EffectKind::MoveSelf:    return "Move to the target tile.";
        case EffectKind::Push:        return "Push " + std::to_string(effect.amount) + ".";
        case EffectKind::Pull:        return "Pull " + std::to_string(effect.amount) + ".";
        case EffectKind::Draw:        return "Draw " + std::to_string(effect.amount) + ".";
        case EffectKind::GainEnergy:  return "Gain " + std::to_string(effect.amount) + " energy.";
        case EffectKind::SpawnHazard: return "Leave a hazard.";
    }
    return {};
}

}  // namespace

const CardDef& GetCard(CardId id) {
    assert(id != CardId::Count);
    return AllCards()[static_cast<std::size_t>(id)];
}

std::vector<CardId> StartingDeck() {
    return {
        CardId::Jab,      CardId::Jab,      CardId::Jab,   CardId::Jab,
        CardId::Fold,     CardId::Fold,     CardId::Fold,
        CardId::Sidestep, CardId::Sidestep,
        CardId::Sweep,
    };
}

const std::vector<CardId>& RewardPool() {
    static const std::vector<CardId> pool = {
        CardId::LongShot, CardId::SplitPot,   CardId::Shove,   CardId::Check,
        CardId::Backhand, CardId::ChipUp,     CardId::Cooler,  CardId::Raise,
        CardId::PocketAces, CardId::TheNuts,
    };
    return pool;
}

int ShopPrice(const CardDef& card) {
    switch (card.rarity) {
        case Rarity::Rare:     return 120;
        case Rarity::Uncommon: return 75;
        case Rarity::Common:   return 45;
        case Rarity::Starter:  break;
    }
    return 45;
}

std::string DescribeShape(const TargetPattern& pattern) {
    const std::string size = std::to_string(pattern.size);

    switch (pattern.shape) {
        case Shape::Line:    return "line " + size;
        case Shape::Cone:    return "cone " + size;
        case Shape::Blast:   return "blast " + size;
        case Shape::Diamond: return "burst " + size;
        case Shape::Ring:    return "ring " + size;
        case Shape::Row:     return "row";
        case Shape::Column:  return "column";
        case Shape::Single:  break;
    }

    switch (pattern.origin) {
        case OriginRule::Self:                return "self";
        case OriginRule::AdjacentTile:        return "adjacent";
        case OriginRule::Direction:           return "direction";
        case OriginRule::AnyTileInRange:      return "range " + std::to_string(pattern.range);
        case OriginRule::OccupiedTileInRange: return "range " + std::to_string(pattern.range);
        case OriginRule::WalkableTileInRange: return "move " + std::to_string(pattern.range);
    }
    return {};
}

std::string DescribeCard(const CardDef& card) {
    std::string out;
    for (const EffectOp& effect : card.effects) {
        if (!out.empty()) out += " ";
        out += DescribeEffect(effect);
    }
    return out;
}

}  // namespace bb
