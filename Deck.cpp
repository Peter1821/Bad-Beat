#include "cards/Deck.h"

#include <utility>

namespace bb {

Deck::Deck(std::vector<CardId> cards, Rng& rng) : rng_(rng), draw_(std::move(cards)) {
    rng_.Shuffle(draw_);
}

void Deck::ReshuffleDiscardIntoDraw() {
    if (discard_.empty()) return;

    draw_.insert(draw_.end(), discard_.begin(), discard_.end());
    discard_.clear();
    rng_.Shuffle(draw_);
}

void Deck::Draw(int count) {
    for (int i = 0; i < count; ++i) {
        if (draw_.empty()) ReshuffleDiscardIntoDraw();

        // Every pile is empty; drawing further is a no-op rather than an error.
        // A deck small enough to run out mid-turn is a deckbuilding decision,
        // not a crash.
        if (draw_.empty()) return;

        hand_.push_back(draw_.back());
        draw_.pop_back();
    }
}

void Deck::DrawTo(int target) {
    const int missing = target - HandSize();
    if (missing > 0) Draw(missing);
}

void Deck::DiscardHand() {
    discard_.insert(discard_.end(), hand_.begin(), hand_.end());
    hand_.clear();
}

void Deck::DiscardFromHand(int index) {
    if (!IsValidHandIndex(index)) return;

    discard_.push_back(hand_[static_cast<std::size_t>(index)]);
    hand_.erase(hand_.begin() + index);
}

void Deck::ExhaustFromHand(int index) {
    if (!IsValidHandIndex(index)) return;

    exhaust_.push_back(hand_[static_cast<std::size_t>(index)]);
    hand_.erase(hand_.begin() + index);
}

int Deck::CardsRemaining() const noexcept {
    return DrawPileSize() + HandSize() + DiscardPileSize();
}

bool Deck::IsValidHandIndex(int index) const noexcept {
    return index >= 0 && index < HandSize();
}

}  // namespace bb
