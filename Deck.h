#pragma once

#include <vector>

#include "cards/CardDef.h"
#include "core/Rng.h"

namespace bb {

/// The four piles a card can be in during a fight.
///
/// Holds a reference to the run's Rng rather than owning one, so every shuffle
/// comes out of the same seeded stream and a run stays reproducible end to end.
class Deck {
public:
    /// Takes the run's deck list and shuffles it into the draw pile.
    Deck(std::vector<CardId> cards, Rng& rng);

    /// Draws until the hand holds `target` cards, or the deck runs dry.
    void DrawTo(int target);

    /// Draws `count` cards, reshuffling the discard pile when the draw pile
    /// empties. Stops early only if every pile is exhausted.
    void Draw(int count);

    /// Moves the whole hand to the discard pile. End of turn.
    void DiscardHand();

    /// Discards one card from the hand by index. What a played card normally does.
    void DiscardFromHand(int index);

    /// Removes one card from the hand for the rest of the fight. Exhaust cards
    /// are meant to be rare and powerful, so spending one is a real decision.
    void ExhaustFromHand(int index);

    const std::vector<CardId>& Hand() const noexcept { return hand_; }

    int DrawPileSize() const noexcept { return static_cast<int>(draw_.size()); }
    int DiscardPileSize() const noexcept { return static_cast<int>(discard_.size()); }
    int ExhaustPileSize() const noexcept { return static_cast<int>(exhaust_.size()); }
    int HandSize() const noexcept { return static_cast<int>(hand_.size()); }

    /// Total cards still in play this fight -- everything except exhausted.
    int CardsRemaining() const noexcept;

    bool IsValidHandIndex(int index) const noexcept;

private:
    /// Tips the discard pile back into the draw pile and shuffles it.
    void ReshuffleDiscardIntoDraw();

    Rng& rng_;
    std::vector<CardId> draw_;
    std::vector<CardId> hand_;
    std::vector<CardId> discard_;
    std::vector<CardId> exhaust_;
};

}  // namespace bb
