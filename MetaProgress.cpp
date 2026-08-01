#include "run/MetaProgress.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "cards/CardDatabase.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace bb {
namespace {

constexpr int kFileVersion = 1;

/// Cards available from the very first run, so a fresh save still has a reward
/// pool worth drawing from.
bool IsFreeFromTheStart(CardId card) {
    return card == CardId::LongShot || card == CardId::SplitPot ||
           card == CardId::Shove || card == CardId::Check;
}

}  // namespace

const std::vector<Unlock>& MetaProgress::AllUnlocks() {
    // Ordered by how hard they are to reach, so the run-over screen reads as a
    // ladder. Early ones come from getting deeper into a run; later ones need
    // finishing it, more than once.
    static const std::vector<Unlock> unlocks = {
        {CardId::Backhand, "reach row 2 of a run", 2, 0},
        {CardId::ChipUp, "reach row 3 of a run", 3, 0},
        {CardId::Cooler, "reach the boss", 5, 0},
        {CardId::Raise, "win a run", 0, 1},
        {CardId::PocketAces, "win 2 runs", 0, 2},
        {CardId::TheNuts, "win 3 runs", 0, 3},
    };
    return unlocks;
}

std::string MetaProgress::DefaultPath() {
#ifdef _WIN32
    char buffer[MAX_PATH] = {};
    const DWORD length = ::GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        std::string path(buffer, length);
        const std::size_t slash = path.find_last_of("\\/");
        if (slash != std::string::npos) {
            return path.substr(0, slash + 1) + "badbeat_meta.txt";
        }
    }
#endif
    return "badbeat_meta.txt";
}

MetaProgress MetaProgress::Load(const std::string& path) {
    MetaProgress meta;

    std::ifstream file(path);
    if (!file) return meta;

    std::string line;
    while (std::getline(file, line)) {
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) continue;

        const std::string key = line.substr(0, equals);
        const std::string value = line.substr(equals + 1);

        // Anything unparseable is skipped rather than fatal. A save file that
        // has been hand-edited or half-written should cost progress at worst,
        // never a crash on launch.
        try {
            if (key == "runs") meta.runs_played_ = std::max(0, std::stoi(value));
            else if (key == "wins") meta.runs_won_ = std::max(0, std::stoi(value));
            else if (key == "best_row") meta.best_row_ = std::max(0, std::stoi(value));
        } catch (const std::exception&) {
            continue;
        }
    }

    return meta;
}

bool MetaProgress::Save(const std::string& path) const {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;

    file << "version=" << kFileVersion << '\n'
         << "runs=" << runs_played_ << '\n'
         << "wins=" << runs_won_ << '\n'
         << "best_row=" << best_row_ << '\n';

    return static_cast<bool>(file);
}

bool MetaProgress::MeetsCondition(const Unlock& unlock) const {
    if (unlock.rows_needed > 0 && best_row_ >= unlock.rows_needed) return true;
    if (unlock.wins_needed > 0 && runs_won_ >= unlock.wins_needed) return true;
    return false;
}

std::vector<CardId> MetaProgress::RecordRun(bool won, int rows_cleared) {
    // Snapshot what was locked before, so the caller can be told precisely what
    // this run earned rather than what is unlocked in total.
    std::vector<CardId> before;
    for (const Unlock& unlock : AllUnlocks()) {
        if (MeetsCondition(unlock)) before.push_back(unlock.card);
    }

    ++runs_played_;
    if (won) ++runs_won_;
    best_row_ = std::max(best_row_, rows_cleared);

    std::vector<CardId> earned;
    for (const Unlock& unlock : AllUnlocks()) {
        if (!MeetsCondition(unlock)) continue;
        if (std::find(before.begin(), before.end(), unlock.card) != before.end()) continue;
        earned.push_back(unlock.card);
    }
    return earned;
}

bool MetaProgress::IsUnlocked(CardId card) const {
    if (IsFreeFromTheStart(card)) return true;

    for (const Unlock& unlock : AllUnlocks()) {
        if (unlock.card == card) return MeetsCondition(unlock);
    }

    // Anything with no unlock rule attached is available -- a card added to the
    // pool without a condition should show up, not vanish.
    return true;
}

std::vector<CardId> MetaProgress::AvailableCards() const {
    std::vector<CardId> available;
    for (const CardId card : RewardPool()) {
        if (IsUnlocked(card)) available.push_back(card);
    }
    return available;
}

}  // namespace bb
