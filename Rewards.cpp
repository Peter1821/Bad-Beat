#include "run/Rewards.h"

#include <algorithm>

#include "cards/CardDatabase.h"

namespace bb {
namespace {

/// Chance out of 100 that a roll reaches for something better than Common.
int UncommonChance(EncounterTier tier) {
    switch (tier) {
        case EncounterTier::Boss:  return 70;
        case EncounterTier::Elite: return 55;
        default:                   return 35;
    }
}

int RareChance(EncounterTier tier) {
    switch (tier) {
        case EncounterTier::Boss:  return 35;
        case EncounterTier::Elite: return 20;
        default:                   return 6;
    }
}

std::vector<CardId> PoolOfRarity(const std::vector<CardId>& pool, Rarity rarity) {
    std::vector<CardId> result;
    for (const CardId id : pool) {
        if (GetCard(id).rarity == rarity) result.push_back(id);
    }
    return result;
}

Rarity RollRarity(Rng& rng, EncounterTier tier) {
    if (rng.Chance(RareChance(tier))) return Rarity::Rare;
    if (rng.Chance(UncommonChance(tier))) return Rarity::Uncommon;
    return Rarity::Common;
}

}  // namespace

int GoldFor(EncounterTier tier, Rng& rng) {
    switch (tier) {
        case EncounterTier::Boss:  return rng.Range(90, 130);
        case EncounterTier::Elite: return rng.Range(45, 70);
        default:                   return rng.Range(20, 35);
    }
}

std::vector<CardId> RollCardReward(Rng& rng,
                                   const std::vector<CardId>& pool,
                                   EncounterTier tier,
                                   int count) {
    std::vector<CardId> offered;
    if (pool.empty()) return offered;

    // Bounded rather than looping until satisfied: with a small unlocked pool
    // there may simply not be three distinct cards to offer, and the roll should
    // degrade to fewer choices rather than spin.
    for (int attempt = 0; attempt < count * 12 && static_cast<int>(offered.size()) < count;
         ++attempt) {
        std::vector<CardId> candidates = PoolOfRarity(pool, RollRarity(rng, tier));
        if (candidates.empty()) candidates = PoolOfRarity(pool, Rarity::Common);
        if (candidates.empty()) candidates = pool;

        const CardId pick = rng.Pick(candidates);
        if (std::find(offered.begin(), offered.end(), pick) == offered.end()) {
            offered.push_back(pick);
        }
    }

    return offered;
}

std::vector<CardId> RollShopStock(Rng& rng, const std::vector<CardId>& pool, int count) {
    std::vector<CardId> stock = pool;
    rng.Shuffle(stock);

    if (static_cast<int>(stock.size()) > count) {
        stock.resize(static_cast<std::size_t>(count));
    }
    return stock;
}

int CardRemovalPrice() { return 70; }

}  // namespace bb
