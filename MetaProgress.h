#pragma once

#include <string>
#include <vector>

#include "cards/CardDef.h"

namespace bb {

/// One thing you can earn by playing.
struct Unlock {
    CardId card = CardId::Jab;

    /// Shown on the run-over screen when it fires.
    std::string requirement;

    /// Furthest row reached in a single run, and total runs won, needed to earn
    /// it. Zero means the condition does not apply.
    int rows_needed = 0;
    int wins_needed = 0;
};

/// What survives between runs.
///
/// Deliberately thin -- a handful of counters and a set of unlocked cards. The
/// design doc calls for a vertical slice, and a deep meta layer would add
/// grinding to a game whose whole pitch is that a run fits in one sitting.
///
/// Stored as line-oriented key=value text rather than JSON: it is a dozen
/// values, it wants to be readable and hand-editable while debugging, and it is
/// not worth a parsing dependency.
class MetaProgress {
public:
    /// Reads from disk, or starts fresh if there is nothing to read. A corrupt
    /// or partial file is treated as fresh rather than as an error -- losing
    /// meta-progress is a disappointment, refusing to launch is a bug report.
    static MetaProgress Load(const std::string& path);

    /// Writes to disk. Returns false if the file could not be written.
    bool Save(const std::string& path) const;

    /// Default save location: beside the executable, so it travels with the
    /// build and is easy to find, inspect and delete.
    static std::string DefaultPath();

    int RunsPlayed() const noexcept { return runs_played_; }
    int RunsWon() const noexcept { return runs_won_; }
    int BestRow() const noexcept { return best_row_; }

    /// Folds a finished run into the totals. Returns the cards this run earned,
    /// so the ending screen can report them.
    std::vector<CardId> RecordRun(bool won, int rows_cleared);

    bool IsUnlocked(CardId card) const;

    /// Reward-pool cards currently available. Locked ones are simply absent --
    /// a card turning up for the first time should be a surprise.
    std::vector<CardId> AvailableCards() const;

    /// Every unlock in the game, in the order they are meant to be earned.
    static const std::vector<Unlock>& AllUnlocks();

private:
    bool MeetsCondition(const Unlock& unlock) const;

    int runs_played_ = 0;
    int runs_won_ = 0;
    int best_row_ = 0;
};

}  // namespace bb
