#pragma once

#include <cstdint>
#include <vector>

#include "cards/TargetPattern.h"
#include "combat/Battlefield.h"
#include "core/Vec2.h"

namespace bb {

/// The single source of truth for what a card touches.
///
/// The UI calls ComputeAffectedTiles every frame while the cursor moves, to
/// paint the highlight. The resolver calls the *same* function when the card is
/// confirmed. Nothing else is allowed to compute affected tiles, because the
/// moment the preview and the outcome are produced by different code they will
/// eventually disagree -- and a card that hits something other than what it
/// highlighted is the single worst bug this game could ship.

/// Why an aim was refused, if it was.
///
/// The UI shows this to the player. "Can't target there" with no reason is the
/// fastest way to make a targeting system feel broken, and each of these maps to
/// a different thing the player should do about it.
enum class AimRejection : std::uint8_t {
    None,           ///< The aim is legal.
    OffBoard,
    OutOfRange,
    NotAdjacent,
    NeedsTarget,    ///< Card must be pointed at an actor.
    NeedsDirection, ///< Direction card with the cursor sat on the caster.
    NotWalkable,    ///< Movement card aimed somewhere nobody could stand.
    NoLineOfSight,  ///< Terrain is in the way.
};

/// Human-readable form of a rejection, for display.
const char* DescribeRejection(AimRejection rejection);

/// Checks an aim and says why it failed.
AimRejection ExplainAim(const TargetPattern& pattern,
                        Vec2 caster,
                        Vec2 aim,
                        const Battlefield& field);

/// Whether `aim` is somewhere this pattern may legally be pointed.
bool IsLegalAim(const TargetPattern& pattern,
                Vec2 caster,
                Vec2 aim,
                const Battlefield& field);

/// Every tile the pattern may legally be aimed at, in reading order. Used to
/// paint the range indicator and to let the UI reject an illegal confirm.
std::vector<Vec2> LegalAimTiles(const TargetPattern& pattern,
                                Vec2 caster,
                                const Battlefield& field);

/// Every tile the card affects when aimed at `aim`, in reading order, with
/// duplicates removed and off-board tiles dropped.
///
/// Returns an empty list when the aim is illegal, so a caller that forgot to
/// check legality fails safe rather than firing into a wall.
std::vector<Vec2> ComputeAffectedTiles(const TargetPattern& pattern,
                                       Vec2 caster,
                                       Vec2 aim,
                                       const Battlefield& field);

/// The orthogonal step pointing from `from` towards `to`.
///
/// Direction-origin cards still let the player wave a normal tile cursor around;
/// the direction is derived from wherever they pointed. One aiming interaction
/// for every card is worth more than the precision of a dedicated direction
/// picker. Ties on the diagonal resolve to the horizontal.
Vec2 DominantDirection(Vec2 from, Vec2 to);

}  // namespace bb
