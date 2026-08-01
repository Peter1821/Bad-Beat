#pragma once

#include <cstdint>
#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "combat/CombatState.h"
#include "run/MetaProgress.h"
#include "run/RunState.h"
#include "ui/CombatScreen.h"
#include "ui/EventScreen.h"
#include "ui/MapScreen.h"
#include "ui/RewardScreen.h"
#include "ui/ShopScreen.h"

namespace bb::ui {

/// Which screen the player is looking at.
enum class Phase {
    Map,        ///< Choosing where to go next.
    Combat,     ///< A fight in progress.
    Reward,     ///< Picking a card after a won fight.
    Shop,       ///< Spending gold.
    Event,      ///< A text choice.
    Outcome,    ///< A node just resolved; the run continues.
    RunOver,    ///< The run finished, won or lost.
};

/// Owns the run and decides which screen is showing.
///
/// The state machine lives in the UI layer rather than in run/ on purpose: its
/// entire job is sequencing screens. RunState below it is pure progression and
/// has no idea a terminal exists, which is what keeps it testable.
class GameApp {
public:
    explicit GameApp(std::uint64_t seed);

    ftxui::Component Component();

    /// One frame, without event handling. Public so headless tooling can render
    /// the app with no terminal attached.
    ftxui::Element Render() const;

    Phase CurrentPhase() const noexcept { return phase_; }
    const RunState& Run() const noexcept { return run_; }
    const MetaProgress& Meta() const noexcept { return meta_; }

    /// The fight currently being played, or nullptr outside combat. Exposed so
    /// scripted checks can drive it directly.
    CombatScreen* ActiveCombat() { return combat_screen_.get(); }

    /// The route view, or nullptr when not on the map.
    MapScreen* ActiveMap() { return map_screen_.get(); }

    RewardScreen* ActiveReward() { return reward_screen_.get(); }
    ShopScreen* ActiveShop() { return shop_screen_.get(); }
    EventScreen* ActiveEvent() { return event_screen_.get(); }

    /// Moves past whatever is on screen: banks a finished fight, starts the
    /// next node from the outcome screen, or begins a fresh run once the
    /// current one has ended. Does nothing mid-fight.
    void Advance();

    /// Throws away the current run and starts another.
    ///
    /// The new seed is drawn from the old run's stream, so every run after the
    /// first is still reproducible from the seed the app started with.
    void StartNewRun();

    /// Quitting is a two-step like every other committing action, and the prompt
    /// says what walking out mid-run actually costs. Abandoning a run does not
    /// bank it, so anything it would have unlocked is lost with it.
    void RequestQuit();
    void CancelQuit();
    bool QuitPending() const noexcept { return quit_pending_; }

    /// True when leaving now would throw away an unfinished run.
    bool RunIsInProgress() const noexcept;

private:
    bool OnEvent(ftxui::Event event);

    /// Shows the route and waits for a choice.
    void ShowMap();

    /// Enters the node the player selected, running whatever it holds.
    void EnterSelectedNode();

    /// Folds the finished fight's result back into the run.
    void BankResult();

    /// Applies a node that resolves without a fight, such as a rest.
    void ResolveInstantNode(NodeType type);

    /// Applies the chosen event option to the run.
    void ApplyEventChoice();

    /// Clears whatever screen is up and shows the node-cleared summary.
    void FinishNode();

    /// The current screen on its own, before the quit prompt is layered on.
    ftxui::Element RenderPhase() const;

    ftxui::Element OutcomeView() const;
    ftxui::Element RunOverView() const;
    ftxui::Element QuitBar() const;

    /// Records the finished run into the save file and latches anything it
    /// unlocked, so the ending screen can report it.
    void BankRunIntoMeta();

    /// Source of seeds for successive runs. Kept separate from the run's own
    /// generator, which gets thrown away with it.
    Rng seed_source_;

    // Declared in construction order: the path is resolved before the progress
    // that is loaded from it.
    std::string meta_path_;
    MetaProgress meta_;

    /// Cards this run earned, shown once on the ending screen.
    std::vector<CardId> just_unlocked_;

    /// Guards against banking the same run twice -- the ending screen is
    /// rendered every frame, and the run is only over once.
    bool run_banked_ = false;

    bool quit_pending_ = false;

    RunState run_;
    Phase phase_ = Phase::Map;

    /// Runs finished so far, shown on the end screen so a session of repeated
    /// attempts has something to show for itself.
    int runs_played_ = 0;

    std::unique_ptr<CombatState> combat_;
    std::unique_ptr<CombatScreen> combat_screen_;
    std::unique_ptr<MapScreen> map_screen_;
    std::unique_ptr<RewardScreen> reward_screen_;
    std::unique_ptr<ShopScreen> shop_screen_;
    std::unique_ptr<EventScreen> event_screen_;

    /// What the node just resolved was, so the outcome screen can report it
    /// after the node itself has been torn down.
    NodeType last_node_type_ = NodeType::Combat;

    /// Whether the fight just finished was survived. Latched when the result is
    /// banked, because the CombatState is thrown away before the outcome screen
    /// is drawn.
    bool last_fight_won_ = false;
};

}  // namespace bb::ui
