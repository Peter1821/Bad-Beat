#include "ui/GameApp.h"

#include <string>
#include <utility>

#include "cards/CardDatabase.h"
#include "combat/Encounter.h"
#include "core/Rules.h"
#include "run/Rewards.h"
#include "ui/Theme.h"

namespace bb::ui {
namespace {

EncounterTier TierFor(NodeType type) {
    switch (type) {
        case NodeType::Elite: return EncounterTier::Elite;
        case NodeType::Boss:  return EncounterTier::Boss;
        default:              return EncounterTier::Normal;
    }
}

/// Whether entering this node starts a fight, as opposed to resolving on the
/// spot.
bool IsFight(NodeType type) {
    return type == NodeType::Combat || type == NodeType::Elite || type == NodeType::Boss;
}

}  // namespace

GameApp::GameApp(std::uint64_t seed)
    : seed_source_(seed),
      meta_path_(MetaProgress::DefaultPath()),
      meta_(MetaProgress::Load(meta_path_)),
      run_(seed) {
    ShowMap();
}

void GameApp::StartNewRun() {
    ++runs_played_;
    run_ = RunState(seed_source_.NextSeed());
    just_unlocked_.clear();
    run_banked_ = false;
    ShowMap();
}

void GameApp::BankRunIntoMeta() {
    if (run_banked_) return;
    run_banked_ = true;

    just_unlocked_ = meta_.RecordRun(run_.IsComplete(), run_.NodesCleared());

    // A failed write costs the player their unlocks but must never stop them
    // playing, so the result is deliberately ignored here.
    meta_.Save(meta_path_);
}

void GameApp::ShowMap() {
    combat_screen_.reset();
    combat_.reset();
    reward_screen_.reset();
    shop_screen_.reset();
    event_screen_.reset();

    map_screen_ = std::make_unique<MapScreen>(run_);
    phase_ = Phase::Map;
}

void GameApp::FinishNode() {
    reward_screen_.reset();
    shop_screen_.reset();
    event_screen_.reset();

    run_.ClearNode();
    phase_ = (!run_.IsAlive() || run_.IsComplete()) ? Phase::RunOver : Phase::Outcome;
    if (phase_ == Phase::RunOver) BankRunIntoMeta();
}

void GameApp::EnterSelectedNode() {
    if (map_screen_ == nullptr) return;

    const int target = map_screen_->Selection();
    if (target < 0) return;
    if (!run_.MoveTo(target)) return;

    const NodeType type = run_.Map().Node(target).type;
    last_node_type_ = type;

    map_screen_.reset();

    if (!IsFight(type)) {
        ResolveInstantNode(type);
        return;
    }

    combat_ = MakeEncounter(run_.Seed(), run_.NodesCleared(), TierFor(type), run_.Hp(),
                            run_.DeckList());
    combat_screen_ = std::make_unique<CombatScreen>(*combat_);
    combat_screen_->SetSubtitle(NodeName(type) + " — row " +
                                std::to_string(run_.CurrentRow()) + " of " +
                                std::to_string(run_.Map().RowCount()));
    phase_ = Phase::Combat;
}

void GameApp::ResolveInstantNode(NodeType type) {
    last_fight_won_ = true;

    switch (type) {
        case NodeType::Rest:
            run_.Heal(run_.MaxHp() * rules::kRestHealPercent / 100);
            break;

        case NodeType::Shop:
            shop_screen_ = std::make_unique<ShopScreen>(
                run_, RollShopStock(run_.GetRng(), meta_.AvailableCards(), 4));
            phase_ = Phase::Shop;
            return;

        case NodeType::Event:
            event_screen_ = std::make_unique<EventScreen>(RollEvent(run_.GetRng()));
            phase_ = Phase::Event;
            return;

        default:
            break;
    }

    FinishNode();
}

void GameApp::ApplyEventChoice() {
    if (event_screen_ == nullptr) return;

    const EventOption* option = event_screen_->Chosen();
    if (option == nullptr) return;

    switch (option->effect) {
        case EventEffect::Heal:       run_.Heal(option->amount); break;
        case EventEffect::LoseHealth: run_.SetHp(run_.Hp() - option->amount); break;
        case EventEffect::GainGold:   run_.AddGold(option->amount); break;
        case EventEffect::LoseGold:   run_.TrySpendGold(option->amount); break;
        case EventEffect::GainCard:   run_.AddCard(option->card); break;
        case EventEffect::Nothing:    break;
    }
}

void GameApp::BankResult() {
    if (combat_screen_ == nullptr) return;

    last_fight_won_ = combat_screen_->PlayerWon();

    // Damage taken in the fight is damage carried into the next one. Attrition
    // is most of what makes a run tense -- without it every node is the first.
    run_.SetHp(combat_screen_->PlayerHp());

    // Drop the fight before anything else: nothing after this point should be
    // reading from a finished battlefield.
    combat_screen_.reset();
    combat_.reset();

    if (!last_fight_won_) {
        phase_ = Phase::RunOver;
        BankRunIntoMeta();
        return;
    }

    const int gold = GoldFor(TierFor(last_node_type_), run_.GetRng());
    run_.AddGold(gold);

    // The boss pays out but has nowhere to spend it, so skip straight to the
    // ending rather than offering a card for a deck that is finished with.
    if (last_node_type_ == NodeType::Boss) {
        FinishNode();
        return;
    }

    reward_screen_ = std::make_unique<RewardScreen>(
        RollCardReward(run_.GetRng(), meta_.AvailableCards(), TierFor(last_node_type_), 3),
        gold);
    phase_ = Phase::Reward;
}

void GameApp::Advance() {
    switch (phase_) {
        case Phase::Map:
            EnterSelectedNode();
            break;

        case Phase::Combat:
            // Only meaningful once the fight has resolved; mid-fight the confirm
            // key belongs to the combat screen.
            if (combat_screen_ != nullptr && combat_screen_->IsFightOver()) BankResult();
            break;

        case Phase::Reward:
            if (reward_screen_ == nullptr) break;
            reward_screen_->Choose();
            if (const auto taken = reward_screen_->Taken()) run_.AddCard(*taken);
            FinishNode();
            break;

        case Phase::Shop:
            if (shop_screen_ == nullptr) break;
            shop_screen_->Activate();
            if (shop_screen_->IsDone()) FinishNode();
            break;

        case Phase::Event:
            if (event_screen_ == nullptr) break;
            if (!event_screen_->IsResolved()) {
                // First press picks the option and shows what it did; the second
                // moves on. Applying and leaving in one keystroke would hide the
                // consequence.
                event_screen_->Choose();
                ApplyEventChoice();
                break;
            }
            FinishNode();
            break;

        case Phase::Outcome:
            ShowMap();
            break;

        case Phase::RunOver:
            StartNewRun();
            break;
    }
}

bool GameApp::OnEvent(ftxui::Event event) {
    using namespace ftxui;

    const bool confirm = event == Event::Return || event == Event::Character(' ');
    if (!confirm) return false;

    switch (phase_) {
        case Phase::Combat:
            // Mid-fight, confirm is "play the card" -- leave it for the screen.
            if (combat_screen_ == nullptr || !combat_screen_->IsFightOver()) return false;
            Advance();
            return true;

        case Phase::Map:
        case Phase::Reward:
        case Phase::Shop:
        case Phase::Event:
        case Phase::Outcome:
        case Phase::RunOver:
            Advance();
            return true;
    }
    return false;
}

ftxui::Component GameApp::Component() {
    using namespace ftxui;

    auto renderer = Renderer([this] { return Render(); });

    // The app's handler runs first so it can claim the confirm key; anything it
    // does not claim falls through to whichever screen is showing.
    return CatchEvent(renderer, [this](Event event) {
        if (OnEvent(event)) return true;

        if (phase_ == Phase::Combat && combat_screen_ != nullptr) {
            return combat_screen_->HandleEvent(event);
        }
        if (phase_ == Phase::Map && map_screen_ != nullptr) {
            return map_screen_->HandleEvent(event);
        }
        if (phase_ == Phase::Reward && reward_screen_ != nullptr) {
            return reward_screen_->HandleEvent(event);
        }
        if (phase_ == Phase::Shop && shop_screen_ != nullptr) {
            return shop_screen_->HandleEvent(event);
        }
        if (phase_ == Phase::Event && event_screen_ != nullptr) {
            return event_screen_->HandleEvent(event);
        }
        return false;
    });
}

ftxui::Element GameApp::OutcomeView() const {
    using namespace ftxui;

    const bool rested = last_node_type_ == NodeType::Rest;

    return vbox({
               // The badge has to sit in an hbox with a filler beside it, or the
               // inverted background stretches the full width of the terminal
               // and reads as a rendering fault.
               hbox({text(" BAD BEAT ") | bold | inverted, filler()}),
               separator(),
               filler(),
               text(rested ? "PATCHED UP" : "NODE CLEARED") | bold | color(theme::kPlayer) |
                   center,
               text("") | center,
               text("row " + std::to_string(run_.CurrentRow()) + " of " +
                    std::to_string(run_.Map().RowCount())) |
                   center,
               text("") | center,
               hbox({
                   filler(),
                   text("health ") | color(theme::kMuted),
                   text(std::to_string(run_.Hp()) + "/" + std::to_string(run_.MaxHp())) |
                       bold | color(run_.Hp() * 2 <= run_.MaxHp() ? theme::kEnemy
                                                                  : theme::kPlayer),
                   text("    deck ") | color(theme::kMuted),
                   text(std::to_string(run_.DeckList().size()) + " cards") | bold,
                   filler(),
               }),
               text("") | center,
               text(rested ? "Back on your feet. It will not last."
                           : "Wounds carry into the next fight.") |
                   color(theme::kMuted) | center,
               filler(),
               separator(),
               hbox({
                   text(" enter ") | bold | color(theme::kAccent),
                   text("back to the route"),
                   filler(),
                   text(" q ") | bold | color(theme::kAccent),
                   text("quit"),
               }),
           }) |
           border;
}

ftxui::Element GameApp::RunOverView() const {
    using namespace ftxui;

    const bool survived = run_.IsAlive() && run_.IsComplete();

    Elements lines{
        hbox({text(" BAD BEAT ") | bold | inverted, filler()}),
        separator(),
        filler(),
        (survived ? text("RUN COMPLETE") | bold | color(theme::kPlayer)
                  : text("BAD BEAT") | bold | color(theme::kEnemy)) |
            center,
        text("") | center,
        text(survived ? "You walked away from the table."
                      : "The board did not let you use it.") |
            color(theme::kMuted) | center,
        text("") | center,
        hbox({
            filler(),
            text("cleared ") | color(theme::kMuted),
            text(std::to_string(run_.NodesCleared()) + " of " +
                 std::to_string(run_.Map().RowCount())) |
                bold,
            filler(),
        }),
    };

    lines.push_back(hbox({
        filler(),
        text("runs ") | color(theme::kMuted),
        text(std::to_string(meta_.RunsPlayed())) | bold,
        text("    won ") | color(theme::kMuted),
        text(std::to_string(meta_.RunsWon())) | bold,
        text("    best ") | color(theme::kMuted),
        text("row " + std::to_string(meta_.BestRow())) | bold,
        filler(),
    }));

    lines.push_back(hbox({
        filler(),
        text("seed ") | color(theme::kMuted),
        text(std::to_string(run_.Seed())) | color(theme::kMuted),
        filler(),
    }));

    // Anything this run earned, called out once. Progress the player cannot see
    // is progress that may as well not exist.
    if (!just_unlocked_.empty()) {
        lines.push_back(text("") | center);
        lines.push_back(text("UNLOCKED") | bold | color(theme::kAccent) | center);
        for (const CardId card : just_unlocked_) {
            lines.push_back(hbox({
                filler(),
                text(GetCard(card).name) | bold,
                text("  —  ") | color(theme::kMuted),
                text(DescribeCard(GetCard(card))) | color(theme::kMuted),
                filler(),
            }));
        }
    }

    lines.push_back(filler());
    lines.push_back(separator());
    lines.push_back(hbox({
        text(" enter ") | bold | color(theme::kAccent),
        text(survived ? "run it again" : "deal me in again"),
        filler(),
        text(" q ") | bold | color(theme::kAccent),
        text("quit"),
    }));

    return vbox(std::move(lines)) | border;
}

void GameApp::RequestQuit() { quit_pending_ = true; }

void GameApp::CancelQuit() { quit_pending_ = false; }

bool GameApp::RunIsInProgress() const noexcept {
    // Once the run-over screen is up the result is already banked, so leaving
    // costs nothing.
    return phase_ != Phase::RunOver;
}

ftxui::Element GameApp::QuitBar() const {
    using namespace ftxui;

    const bool costly = RunIsInProgress();

    const std::string detail =
        costly ? "this run is unfinished — it will not be recorded"
               : "your progress is saved";

    return hbox({
               text(" ? ") | bold | inverted |
                   color(costly ? theme::kDanger : theme::kAccent),
               text(" Quit? ") | bold,
               text(detail) | color(theme::kMuted),
               filler(),
               text(" enter ") | bold | color(theme::kAccent),
               text("yes"),
               text("   "),
               text(" esc ") | bold | color(theme::kAccent),
               text("no"),
           }) |
           border;
}

ftxui::Element GameApp::Render() const {
    ftxui::Element screen = RenderPhase();
    if (!quit_pending_) return screen;

    // Stacked below the screen rather than replacing it, so the board you are
    // about to abandon stays visible while you decide.
    return ftxui::vbox({std::move(screen), QuitBar()});
}

ftxui::Element GameApp::RenderPhase() const {
    switch (phase_) {
        case Phase::Map:
            if (map_screen_ != nullptr) return map_screen_->Render();
            break;
        case Phase::Combat:
            if (combat_screen_ != nullptr) return combat_screen_->Render();
            break;
        case Phase::Reward:
            if (reward_screen_ != nullptr) return reward_screen_->Render();
            break;
        case Phase::Shop:
            if (shop_screen_ != nullptr) return shop_screen_->Render();
            break;
        case Phase::Event:
            if (event_screen_ != nullptr) return event_screen_->Render();
            break;
        case Phase::Outcome:
            return OutcomeView();
        case Phase::RunOver:
            return RunOverView();
    }
    return ftxui::text("");
}

}  // namespace bb::ui
