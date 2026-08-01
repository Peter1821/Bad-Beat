#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>  // component.hpp only forward-declares Event
#include <ftxui/dom/elements.hpp>

#include "cards/CardDef.h"
#include "combat/ActionResolver.h"
#include "combat/CombatState.h"
#include "combat/TurnSystem.h"
#include "core/Vec2.h"

namespace bb::ui {

/// The fight view: the board, the roster, the hand, and a cursor.
///
/// Has two modes. With no card selected the cursor inspects tiles. With a card
/// selected the cursor is an aim point and the board shows what that card would
/// hit -- always via ComputeAffectedTiles, never recomputed here.
///
/// Owns only view state. As of milestone 3 it still never mutates CombatState;
/// actually resolving a card is milestone 4.
class CombatScreen {
public:
    explicit CombatScreen(CombatState& state);

    /// Handles one key. Returns false for anything it does not use, so the
    /// owning app can act on it instead -- quitting, advancing the run.
    ///
    /// This screen is driven by GameApp rather than wrapping itself in a
    /// Component, so there is exactly one place that decides which screen sees
    /// an event.
    bool HandleEvent(ftxui::Event event);

    /// One frame, without event handling. Public so headless tooling can render
    /// the screen with no terminal attached.
    ftxui::Element Render() const;

    /// Text shown in the top-right corner -- the run's progress, set by the app
    /// that owns this fight. Replaces what used to be a hardcoded milestone
    /// label that went stale every time the project moved on.
    void SetSubtitle(std::string subtitle) { subtitle_ = std::move(subtitle); }

    Vec2 Cursor() const noexcept { return cursor_; }
    void MoveCursor(Vec2 step);
    void SnapCursorToPlayer();

    /// Selects a card by hand index, or clears the selection if that card is
    /// already selected. Out-of-range indices are ignored.
    void SelectCard(int hand_index);
    void ClearSelection();

    std::optional<int> SelectedCardIndex() const noexcept { return selected_card_; }

    /// What the player has staged but not yet committed.
    enum class PendingKind {
        None,
        PlayCard,
        FreeStep,
        EndTurn,
    };

    /// Stages whatever the cursor is pointing at -- playing the selected card,
    /// or spending the free step onto an adjacent tile. Does nothing if the
    /// action would not be legal, so an illegal aim never arms a prompt.
    void RequestConfirm();

    /// Stages the end of the turn.
    void RequestEndTurn();

    /// Carries out the staged action. Returns false if nothing was staged or
    /// the action turned out to be illegal.
    bool CommitPending();

    /// Drops the staged action without doing it.
    void CancelPending();

    PendingKind Pending() const noexcept { return pending_; }
    bool HasPending() const noexcept { return pending_ != PendingKind::None; }

    /// Stages and immediately commits. For scripted runs, where the two-step
    /// prompt only gets in the way.
    bool Confirm();
    void EndTurn();

    /// True once one side has no one left standing.
    bool IsFightOver() const;

    /// True when the player won. Only meaningful once IsFightOver().
    bool PlayerWon() const;

    /// The player's remaining health, for carrying back into the run. Zero if
    /// they did not survive.
    int PlayerHp() const;

private:
    /// The card currently selected, or nullptr when browsing.
    const CardDef* SelectedCard() const;

    /// Where the caster is standing. nullopt if the player is dead.
    std::optional<Vec2> CasterTile() const;

    /// Tiles the selected card would hit at the current cursor. Empty when no
    /// card is selected or the aim is illegal.
    std::vector<Vec2> AffectedTiles() const;

    ftxui::Element InspectorPanel() const;
    ftxui::Element AimPanel(const CardDef& card) const;
    ftxui::Element RosterPanel() const;
    ftxui::Element LegendPanel() const;
    ftxui::Element HandPanel() const;
    ftxui::Element HelpBar() const;
    ftxui::Element ConfirmBar() const;
    ftxui::Element OutcomeBanner() const;

    /// True when the staged action could still be carried out. The board can
    /// change under a prompt -- a card can stop being affordable, an aim can
    /// stop being legal -- and a prompt for something impossible is worse than
    /// no prompt.
    bool PendingIsStillLegal() const;

    CombatState& state_;
    ActionResolver resolver_;
    TurnSystem turns_;

    std::string subtitle_;
    Vec2 cursor_{0, 0};
    std::optional<int> selected_card_;

    PendingKind pending_ = PendingKind::None;

    /// The tile the staged action was aimed at, so moving the cursor afterwards
    /// cannot silently retarget it.
    Vec2 pending_target_{0, 0};
};

}  // namespace bb::ui
