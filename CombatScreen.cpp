#include "ui/CombatScreen.h"

#include <string>
#include <utility>

#include "cards/CardDatabase.h"
#include "cards/Targeting.h"
#include "core/Rules.h"
#include "ui/Theme.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/GridRenderer.h"
#include "ui/widgets/LogPanel.h"

namespace bb::ui {

CombatScreen::CombatScreen(CombatState& state)
    : state_(state), resolver_(state), turns_(state) {
    turns_.BeginFight();
    SnapCursorToPlayer();
}

bool CombatScreen::PendingIsStillLegal() const {
    const Actor* player = state_.PrimaryPlayer();
    if (player == nullptr || turns_.IsFightOver()) return false;

    switch (pending_) {
        case PendingKind::None:
            return false;

        case PendingKind::EndTurn:
            return true;

        case PendingKind::FreeStep: {
            if (player->HasUsedFreeStep()) return false;

            const auto from = state_.Field().PositionOf(player->Id());
            if (!from || !IsOrthogonallyAdjacent(*from, pending_target_)) return false;

            return state_.Field().IsWalkable(pending_target_);
        }

        case PendingKind::PlayCard: {
            const CardDef* card = SelectedCard();
            if (card == nullptr || !state_.CanAfford(card->cost)) return false;

            const auto caster = CasterTile();
            if (!caster) return false;

            return IsLegalAim(card->pattern, *caster, pending_target_, state_.Field());
        }
    }
    return false;
}

void CombatScreen::RequestConfirm() {
    const Actor* player = state_.PrimaryPlayer();
    if (player == nullptr || turns_.IsFightOver()) return;

    pending_target_ = cursor_;

    if (selected_card_) {
        const CardDef* card = SelectedCard();
        if (card == nullptr) return;

        const auto caster = CasterTile();
        if (!caster) return;

        // Refuse to arm a prompt for something that cannot happen -- a
        // confirmation for an illegal play just teaches the player to ignore
        // the prompt.
        if (!state_.CanAfford(card->cost)) return;
        if (!IsLegalAim(card->pattern, *caster, cursor_, state_.Field())) return;

        pending_ = PendingKind::PlayCard;
        return;
    }

    if (player->HasUsedFreeStep()) return;

    const auto from = state_.Field().PositionOf(player->Id());
    if (!from || !IsOrthogonallyAdjacent(*from, cursor_)) return;
    if (!state_.Field().IsWalkable(cursor_)) return;

    pending_ = PendingKind::FreeStep;
}

void CombatScreen::RequestEndTurn() {
    if (turns_.IsFightOver()) return;
    pending_ = PendingKind::EndTurn;
    pending_target_ = cursor_;
}

void CombatScreen::CancelPending() { pending_ = PendingKind::None; }

bool CombatScreen::CommitPending() {
    const PendingKind kind = pending_;
    pending_ = PendingKind::None;

    const Actor* player = state_.PrimaryPlayer();
    if (player == nullptr || turns_.IsFightOver()) return false;

    switch (kind) {
        case PendingKind::None:
            return false;

        case PendingKind::EndTurn:
            ClearSelection();
            turns_.EndPlayerTurn();
            SnapCursorToPlayer();
            return true;

        case PendingKind::FreeStep:
            return resolver_.TakeFreeStep(player->Id(), pending_target_);

        case PendingKind::PlayCard: {
            if (!selected_card_) return false;

            const bool played =
                resolver_.PlayCard(player->Id(), *selected_card_, pending_target_);
            if (played) {
                // Always drop the selection: the hand has shifted, so the stored
                // index no longer means what it did.
                ClearSelection();

                // A card that moved the caster leaves the cursor stranded; put
                // it back on the player so the next action starts from where
                // they are.
                SnapCursorToPlayer();
            }
            return played;
        }
    }
    return false;
}

bool CombatScreen::Confirm() {
    RequestConfirm();
    return CommitPending();
}

void CombatScreen::EndTurn() {
    RequestEndTurn();
    CommitPending();
}

bool CombatScreen::IsFightOver() const { return turns_.IsFightOver(); }

bool CombatScreen::PlayerWon() const { return turns_.PlayerWon(); }

int CombatScreen::PlayerHp() const {
    // PrimaryPlayer only reports the living, so a wipe correctly reads as zero
    // rather than as whatever the corpse happens to hold.
    const Actor* player = state_.PrimaryPlayer();
    return player != nullptr ? player->Hp() : 0;
}

void CombatScreen::MoveCursor(Vec2 step) {
    cursor_ = ClampToBounds(cursor_ + step, state_.Field().Width(), state_.Field().Height());
}

void CombatScreen::SnapCursorToPlayer() {
    const Actor* player = state_.PrimaryPlayer();
    if (player == nullptr) return;

    if (const auto position = state_.Field().PositionOf(player->Id())) {
        cursor_ = *position;
    }
}

void CombatScreen::SelectCard(int hand_index) {
    if (turns_.IsFightOver()) return;
    if (!state_.GetDeck().IsValidHandIndex(hand_index)) return;

    // Pressing the same number twice puts the card back down.
    if (selected_card_ && *selected_card_ == hand_index) {
        ClearSelection();
        return;
    }

    selected_card_ = hand_index;

    // Start the aim on the player, so a card with a Self or Direction origin is
    // immediately showing something sensible rather than an empty preview.
    SnapCursorToPlayer();
}

void CombatScreen::ClearSelection() { selected_card_.reset(); }

const CardDef* CombatScreen::SelectedCard() const {
    if (!selected_card_) return nullptr;

    const Deck& deck = state_.GetDeck();
    if (!deck.IsValidHandIndex(*selected_card_)) return nullptr;

    return &GetCard(deck.Hand()[static_cast<std::size_t>(*selected_card_)]);
}

std::optional<Vec2> CombatScreen::CasterTile() const {
    const Actor* player = state_.PrimaryPlayer();
    if (player == nullptr) return std::nullopt;
    return state_.Field().PositionOf(player->Id());
}

std::vector<Vec2> CombatScreen::AffectedTiles() const {
    const CardDef* card = SelectedCard();
    if (card == nullptr) return {};

    const auto caster = CasterTile();
    if (!caster) return {};

    return ComputeAffectedTiles(card->pattern, *caster, cursor_, state_.Field());
}

bool CombatScreen::HandleEvent(ftxui::Event event) {
    using namespace ftxui;

    // While something is staged, the only questions on the table are yes and no.
    // Anything else backs out first, so a stray keypress can never be read as
    // agreement.
    if (HasPending()) {
        if (event == Event::Return || event == Event::Character('y') ||
            event == Event::Character(' ')) {
            CommitPending();
            return true;
        }

        CancelPending();
        if (event == Event::Escape || event == Event::Character('n')) return true;
        // Fall through: a movement or selection key both cancels and acts, so
        // changing your mind mid-prompt costs one keypress rather than two.
    }

    // Arrows and vi keys both, since this is a keyboard-only game and people
    // reach for one or the other without thinking.
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        MoveCursor(kNorth);
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        MoveCursor(kSouth);
        return true;
    }
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        MoveCursor(kWest);
        return true;
    }
    if (event == Event::ArrowRight || event == Event::Character('l')) {
        MoveCursor(kEast);
        return true;
    }
    if (event == Event::Character('c')) {
        SnapCursorToPlayer();
        return true;
    }

    // Number keys pick a card out of the hand.
    for (int i = 1; i <= 9; ++i) {
        if (event == Event::Character(static_cast<char>('0' + i))) {
            SelectCard(i - 1);
            return true;
        }
    }

    if (event == Event::Return) {
        RequestConfirm();
        return true;
    }
    if (event == Event::Character(' ')) {
        RequestEndTurn();
        return true;
    }

    if (event == Event::Escape && selected_card_) {
        // Only swallow Escape when there is a selection to cancel; otherwise the
        // application still gets it as a quit.
        ClearSelection();
        return true;
    }

    return false;
}

ftxui::Element CombatScreen::InspectorPanel() const {
    using namespace ftxui;

    const Battlefield& field = state_.Field();
    const Terrain terrain = field.TerrainAt(cursor_);

    Elements lines{
        hbox({
            text("tile  ") | color(theme::kMuted),
            text(std::to_string(cursor_.x) + ", " + std::to_string(cursor_.y)),
        }),
        hbox({
            text("type  ") | color(theme::kMuted),
            text(TerrainName(terrain)) | color(AppearanceOf(terrain).color),
        }),
        hbox({
            text("walk  ") | color(theme::kMuted),
            // Say *why* a tile is closed. "no" alone reads as a bug when the
            // cursor is sitting on your own unit.
            field.IsWalkable(cursor_)   ? text("yes")
            : BlocksMovement(terrain)   ? text("no — wall") | color(theme::kEnemy)
            : field.IsOccupied(cursor_) ? text("no — occupied") | color(theme::kEnemy)
                                        : text("no — off board") | color(theme::kEnemy),
        }),
    };

    if (const Actor* actor = state_.ActorAt(cursor_)) {
        const Color team = TeamColor(actor->GetTeam());
        lines.push_back(separator());
        lines.push_back(hbox({
            text(AppearanceOf(*actor).glyph + " ") | color(team) | bold,
            text(actor->Name()) | bold,
        }));
        lines.push_back(hbox({
            text("hp    ") | color(theme::kMuted),
            text(std::to_string(actor->Hp()) + "/" + std::to_string(actor->MaxHp())),
        }));
        if (actor->Block() > 0) {
            lines.push_back(hbox({
                text("block ") | color(theme::kMuted),
                text(std::to_string(actor->Block())) | color(theme::kBlock),
            }));
        }
    } else {
        lines.push_back(separator());
        lines.push_back(text("nobody here") | color(theme::kMuted));
    }

    return vbox(std::move(lines));
}

ftxui::Element CombatScreen::AimPanel(const CardDef& card) const {
    using namespace ftxui;

    const auto caster = CasterTile();
    const std::vector<Vec2> affected = AffectedTiles();

    const AimRejection rejection =
        caster ? ExplainAim(card.pattern, *caster, cursor_, state_.Field())
               : AimRejection::OffBoard;
    const bool legal = rejection == AimRejection::None;

    // A self-targeted card's aim never moves, so showing cursor coordinates
    // there would imply the cursor matters when it does not.
    const bool self_aimed = card.pattern.origin == OriginRule::Self;

    Elements lines{
        hbox({text(card.name) | bold, filler(),
              text(std::to_string(card.cost)) | bold | color(theme::kAccent)}),
        text(DescribeShape(card.pattern)) | color(theme::kThreat),
        separator(),
        hbox({
            text("aim   ") | color(theme::kMuted),
            self_aimed ? text("self")
                       : text(std::to_string(cursor_.x) + ", " + std::to_string(cursor_.y)),
        }),
    };

    if (!legal) {
        lines.push_back(hbox({
            text("      "),
            text(DescribeRejection(rejection)) | color(theme::kEnemy),
        }));
    } else {
        lines.push_back(hbox({
            text("hits  ") | color(theme::kMuted),
            text(std::to_string(affected.size()) +
                 (affected.size() == 1 ? " tile" : " tiles")) |
                color(theme::kThreat),
        }));

        int targets = 0;
        for (const Vec2 tile : affected) {
            if (state_.ActorAt(tile) != nullptr) ++targets;
        }
        lines.push_back(hbox({
            text("actors") | color(theme::kMuted),
            text(" " + std::to_string(targets)) |
                color(targets > 0 ? theme::kThreat : theme::kMuted),
        }));
    }

    if (!state_.CanAfford(card.cost)) {
        lines.push_back(separator());
        lines.push_back(text("not enough energy") | color(theme::kEnemy));
    }

    return vbox(std::move(lines));
}

ftxui::Element CombatScreen::RosterPanel() const {
    using namespace ftxui;

    Elements rows;
    for (const Actor& actor : state_.Actors()) {
        const Color team = TeamColor(actor.GetTeam());
        const bool is_dead = !actor.IsAlive();

        const float fraction = static_cast<float>(actor.Hp()) /
                               static_cast<float>(actor.MaxHp());

        Elements line{
            text(AppearanceOf(actor).glyph + " ") | color(team),
            text(actor.Name()) | size(WIDTH, EQUAL, 10),
            // bgcolor gives the gauge a visible empty track, so "how much is
            // missing" reads at a glance and two full bars in adjacent rows do
            // not merge into one block.
            gauge(fraction) | size(WIDTH, EQUAL, 10) | color(team) |
                bgcolor(theme::kTrack),
            text(" " + std::to_string(actor.Hp()) + "/" + std::to_string(actor.MaxHp())),
        };

        if (actor.Block() > 0) {
            line.push_back(text("  [" + std::to_string(actor.Block()) + "]") |
                           color(theme::kBlock));
        }

        // What this enemy has announced. Shown next to its health so the two
        // numbers a player weighs -- can I kill it, and what does it do to me
        // if I cannot -- sit side by side.
        if (actor.GetTeam() == Team::Enemy && actor.IsAlive()) {
            const Intent& intent = actor.GetIntent();
            const bool dangerous = intent.kind == IntentKind::Attack;
            line.push_back(text("  " + DescribeIntent(intent)) |
                           color(dangerous ? theme::kDanger : theme::kMuted));
        }

        Element row = hbox(std::move(line));
        if (is_dead) row = row | dim | strikethrough;
        rows.push_back(std::move(row));
    }

    return vbox(std::move(rows));
}

ftxui::Element CombatScreen::LegendPanel() const {
    using namespace ftxui;

    // Three packed lines rather than nine.
    //
    // A boss roster runs to six actors, and at that size a one-entry-per-line
    // legend was pushed off the bottom of the panel -- losing the "! !" marker
    // exactly when the board is at its most dangerous. The legend has to fit at
    // the worst case, not the best one.
    const auto marker = [](const std::string& glyph, const Color& paint,
                           const std::string& label) {
        return hbox({text(glyph) | color(paint) | bold, text(" " + label + "   ")});
    };

    return vbox({
        hbox({
            text("@") | color(theme::kPlayer) | bold, text(" you   "),
            text("r") | color(theme::kEnemy) | bold, text(" rat   "),
            text("s") | color(theme::kEnemy) | bold, text(" spitter   "),
            text("S") | color(theme::kEnemy) | bold, text(" slugger"),
        }),
        hbox({
            text("#") | color(theme::kWall) | bold, text(" wall, blocks sight   "),
            text("~") | color(theme::kHazard) | bold, text(" hazard, hurts"),
        }),
        hbox({
            marker("[ ]", theme::kAccent, "cursor"),
            marker("< >", theme::kThreat, "you hit"),
            marker("! !", theme::kDanger, "incoming"),
        }),
    });
}

ftxui::Element CombatScreen::HandPanel() const {
    using namespace ftxui;

    const Deck& deck = state_.GetDeck();

    // Once the fight is over nothing in hand can be played, so nothing in hand
    // should look playable. The actions are already blocked in code; this stops
    // the UI advertising options that silently do nothing.
    const bool fight_over = turns_.IsFightOver();

    Elements cards;
    for (int i = 0; i < deck.HandSize(); ++i) {
        const CardDef& card = GetCard(deck.Hand()[static_cast<std::size_t>(i)]);
        const bool selected = selected_card_.has_value() && *selected_card_ == i;
        const bool playable = !fight_over && state_.CanAfford(card.cost);
        cards.push_back(RenderCard(card, i + 1, selected && !fight_over, playable));
    }

    if (cards.empty()) cards.push_back(text("hand empty") | color(theme::kMuted));

    return hbox(std::move(cards));
}

ftxui::Element CombatScreen::ConfirmBar() const {
    using namespace ftxui;

    const auto key = [](const std::string& label) {
        return text(" " + label + " ") | bold | color(theme::kAccent);
    };

    std::string question;
    std::string detail;
    Color tone = theme::kAccent;

    switch (pending_) {
        case PendingKind::None:
            return text("");

        case PendingKind::PlayCard: {
            const CardDef* card = SelectedCard();
            question = "Play " + (card != nullptr ? card->name : std::string("card")) + "?";

            const std::vector<Vec2> tiles = AffectedTiles();
            int targets = 0;
            for (const Vec2 tile : tiles) {
                const Actor* victim = state_.ActorAt(tile);
                if (victim != nullptr && victim->GetTeam() == Team::Enemy) ++targets;
            }
            detail = std::to_string(tiles.size()) +
                     (tiles.size() == 1 ? " tile, " : " tiles, ") +
                     std::to_string(targets) + (targets == 1 ? " enemy" : " enemies");
            break;
        }

        case PendingKind::FreeStep:
            question = "Step to " + std::to_string(pending_target_.x) + ", " +
                       std::to_string(pending_target_.y) + "?";
            detail = state_.Field().TerrainAt(pending_target_) == Terrain::Hazard
                         ? "that tile is a hazard"
                         : "your one free step this turn";
            if (state_.Field().TerrainAt(pending_target_) == Terrain::Hazard) {
                tone = theme::kDanger;
            }
            break;

        case PendingKind::EndTurn: {
            question = "End turn?";

            // The most common regret is ending a turn with resources still in
            // hand, so say so plainly rather than just asking.
            const int energy = state_.Energy();
            const int cards = state_.GetDeck().HandSize();
            if (energy > 0 || cards > 0) {
                detail = std::to_string(energy) + " energy and " + std::to_string(cards) +
                         (cards == 1 ? " card" : " cards") + " unspent";
                tone = theme::kDanger;
            } else {
                detail = "nothing left to spend";
            }
            break;
        }
    }

    return hbox({
        text(" ? ") | bold | inverted | color(tone),
        text(" " + question + " ") | bold,
        text(detail) | color(theme::kMuted),
        filler(),
        key("enter"),
        text("yes"),
        text("   "),
        key("esc"),
        text("no"),
    });
}

ftxui::Element CombatScreen::HelpBar() const {
    using namespace ftxui;

    const auto key = [](const std::string& label) {
        return text(" " + label + " ") | bold | color(theme::kAccent);
    };

    if (turns_.IsFightOver()) {
        return hbox({
            text(turns_.PlayerWon() ? "The board is clear." : "You are out of the run.") |
                color(theme::kMuted),
            filler(),
            key("q"),
            text("quit"),
        });
    }

    Elements items{key("←↑↓→"), text("move")};

    // Derived from SelectedCard() rather than the raw index, so the help bar can
    // never disagree with the panel above it about whether a card is in hand.
    if (SelectedCard() != nullptr) {
        items.push_back(text("  "));
        items.push_back(key("enter"));
        items.push_back(text("play"));
        items.push_back(text("  "));
        items.push_back(key("esc"));
        items.push_back(text("cancel"));
    } else {
        items.push_back(text("  "));
        items.push_back(key("1-5"));
        items.push_back(text("card"));
        items.push_back(text("  "));
        items.push_back(key("enter"));
        items.push_back(text("step"));
    }

    items.push_back(text("  "));
    items.push_back(key("space"));
    items.push_back(text("end turn"));
    items.push_back(text("  "));
    items.push_back(key("q"));
    items.push_back(text("quit"));

    return hbox(std::move(items));
}

ftxui::Element CombatScreen::OutcomeBanner() const {
    using namespace ftxui;

    if (!turns_.IsFightOver()) return text("");

    return turns_.PlayerWon()
               ? text(" VICTORY ") | bold | color(theme::kPlayer) | inverted
               : text(" DEFEAT ") | bold | color(theme::kEnemy) | inverted;
}

ftxui::Element CombatScreen::Render() const {
    using namespace ftxui;

    const CardDef* card = SelectedCard();
    const auto caster = CasterTile();

    GridOverlay overlay;
    overlay.cursor = cursor_;

    // Gather every tile any living enemy has committed to hitting. Shown at all
    // times, not just while aiming -- the player needs it most when deciding
    // where to stand, which is before they pick a card.
    if (!turns_.IsFightOver()) {
        for (const Actor& actor : state_.Actors()) {
            if (actor.GetTeam() != Team::Enemy || !actor.IsAlive()) continue;
            const Intent& intent = actor.GetIntent();
            overlay.threatened.insert(overlay.threatened.end(), intent.threatened.begin(),
                                      intent.threatened.end());
        }
    }

    if (card != nullptr && caster) {
        overlay.affected = AffectedTiles();

        // A self-targeted card accepts any aim, so painting every legal aim tile
        // would light up the whole board and imply the cursor matters when it
        // does not. Show reach only for cards that actually have reach.
        if (card->pattern.origin != OriginRule::Self) {
            overlay.in_range = LegalAimTiles(card->pattern, *caster, state_.Field());
        }
    }

    const Deck& deck = state_.GetDeck();

    Element side_panel = card != nullptr ? AimPanel(*card) : InspectorPanel();
    const std::string side_title = card != nullptr ? "AIMING" : "CURSOR";

    return vbox({
               hbox({
                   text(" BAD BEAT ") | bold | inverted,
                   text("  turn ") | color(theme::kMuted),
                   text(std::to_string(state_.TurnNumber())) | bold,
                   text("   energy ") | color(theme::kMuted),
                   text(std::to_string(state_.Energy()) + "/" +
                        std::to_string(rules::kEnergyPerTurn)) |
                       bold | color(theme::kAccent),
                   text("  "),
                   OutcomeBanner(),
                   filler(),
                   text(subtitle_.empty() ? "" : subtitle_ + " ") | color(theme::kMuted),
               }),
               separator(),

               hbox({
                   vbox({
                       text("BATTLEFIELD") | bold,
                       text(""),
                       RenderBattlefield(state_, overlay),
                   }),
                   text("  "),
                   separator(),
                   text("  "),
                   vbox({
                       text(side_title) | bold,
                       text(""),
                       std::move(side_panel),
                   }) | size(WIDTH, EQUAL, 24),
                   separator(),
                   text("  "),
                   // Kept tight on vertical padding: a boss roster is seven
                   // rows, and the battlefield beside it is only fifteen. Every
                   // blank line here is one the legend loses at the worst
                   // possible moment.
                   vbox({
                       text("ROSTER") | bold,
                       text(""),
                       RosterPanel(),
                       text(""),
                       separator(),
                       text("LEGEND") | bold,
                       LegendPanel(),
                   }) | flex,
               }),

               filler(),
               separator(),
               text("LOG") | bold,
               RenderLog(state_, 6),
               separator(),
               hbox({
                   text("HAND") | bold,
                   filler(),
                   text("draw " + std::to_string(deck.DrawPileSize()) + "   discard " +
                        std::to_string(deck.DiscardPileSize())) |
                       color(theme::kMuted),
               }),
               HandPanel(),
               separator(),
               HasPending() ? ConfirmBar() : HelpBar(),
           }) |
           border;
}

}  // namespace bb::ui
