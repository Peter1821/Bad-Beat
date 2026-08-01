#include "cards/Targeting.h"

#include <algorithm>

namespace bb {
namespace {

/// Collects the shape's tiles before any board filtering.
std::vector<Vec2> RawShapeTiles(const TargetPattern& pattern,
                                Vec2 caster,
                                Vec2 aim,
                                const Battlefield& field) {
    std::vector<Vec2> tiles;
    const int size = pattern.size;

    switch (pattern.shape) {
        case Shape::Single:
            tiles.push_back(aim);
            break;

        case Shape::Line: {
            // Projects outward from the caster, not centred on the aim -- a line
            // card is "everything in front of me", so the aim only picks a
            // heading.
            const Vec2 step = DominantDirection(caster, aim);
            for (int i = 1; i <= size; ++i) tiles.push_back(caster + step * i);
            break;
        }

        case Shape::Cone: {
            // Depth i is (2i - 1) tiles wide, so the wedge opens by one tile on
            // each side per step.
            const Vec2 step = DominantDirection(caster, aim);
            const Vec2 across{step.y, step.x};  // perpendicular
            for (int depth = 1; depth <= size; ++depth) {
                for (int spread = -(depth - 1); spread <= depth - 1; ++spread) {
                    tiles.push_back(caster + step * depth + across * spread);
                }
            }
            break;
        }

        case Shape::Blast:
            for (int dy = -size; dy <= size; ++dy) {
                for (int dx = -size; dx <= size; ++dx) {
                    tiles.push_back(aim + Vec2{dx, dy});
                }
            }
            break;

        case Shape::Diamond:
            for (int dy = -size; dy <= size; ++dy) {
                for (int dx = -size; dx <= size; ++dx) {
                    const Vec2 offset{dx, dy};
                    if (ManhattanDistance({0, 0}, offset) <= size) tiles.push_back(aim + offset);
                }
            }
            break;

        case Shape::Ring:
            for (int dy = -size; dy <= size; ++dy) {
                for (int dx = -size; dx <= size; ++dx) {
                    const Vec2 offset{dx, dy};
                    if (ManhattanDistance({0, 0}, offset) == size) tiles.push_back(aim + offset);
                }
            }
            break;

        case Shape::Row:
            for (int x = 0; x < field.Width(); ++x) tiles.push_back({x, aim.y});
            break;

        case Shape::Column:
            for (int y = 0; y < field.Height(); ++y) tiles.push_back({aim.x, y});
            break;
    }

    return tiles;
}

}  // namespace

Vec2 DominantDirection(Vec2 from, Vec2 to) {
    const Vec2 delta = to - from;

    // A cursor sitting exactly on the caster has no heading; default to east so
    // the preview shows *something* rather than collapsing to nothing.
    if (delta.x == 0 && delta.y == 0) return kEast;

    if (Abs(delta.x) >= Abs(delta.y)) return delta.x > 0 ? kEast : kWest;
    return delta.y > 0 ? kSouth : kNorth;
}

const char* DescribeRejection(AimRejection rejection) {
    switch (rejection) {
        case AimRejection::None:          return "";
        case AimRejection::OffBoard:      return "off the board";
        case AimRejection::OutOfRange:    return "out of range";
        case AimRejection::NotAdjacent:   return "must be adjacent";
        case AimRejection::NeedsTarget:   return "needs a target";
        case AimRejection::NeedsDirection: return "pick a direction";
        case AimRejection::NotWalkable:   return "cannot stand there";
        case AimRejection::NoLineOfSight: return "no line of sight";
    }
    return "";
}

AimRejection ExplainAim(const TargetPattern& pattern,
                        Vec2 caster,
                        Vec2 aim,
                        const Battlefield& field) {
    if (!field.Contains(aim)) return AimRejection::OffBoard;

    switch (pattern.origin) {
        case OriginRule::Self:
            // A self-targeted card ignores the aim entirely, so wherever the
            // cursor happens to be is fine. Rejecting a wandering cursor would
            // make the preview blink out for a card whose target cannot change,
            // which reads as the card being broken.
            return AimRejection::None;

        case OriginRule::Direction:
            // Any tile but the caster's own names a heading. Sitting exactly on
            // the caster is the one ambiguous case, and it is not a range
            // problem -- the player just has not pointed anywhere yet.
            return aim == caster ? AimRejection::NeedsDirection : AimRejection::None;

        case OriginRule::AdjacentTile:
            if (!IsOrthogonallyAdjacent(caster, aim)) return AimRejection::NotAdjacent;
            break;

        case OriginRule::AnyTileInRange:
            if (ManhattanDistance(caster, aim) > pattern.range) return AimRejection::OutOfRange;
            break;

        case OriginRule::OccupiedTileInRange:
            if (ManhattanDistance(caster, aim) > pattern.range) return AimRejection::OutOfRange;
            if (!field.IsOccupied(aim)) return AimRejection::NeedsTarget;
            break;

        case OriginRule::WalkableTileInRange:
            // Movement cards must land somewhere a body can actually go, so the
            // preview never promises a step into a wall or onto another actor.
            // The caster's own tile is excluded too: a move to where you already
            // are is a wasted card, not a legal play.
            if (ManhattanDistance(caster, aim) > pattern.range) return AimRejection::OutOfRange;
            if (!field.IsWalkable(aim)) return AimRejection::NotWalkable;
            break;
    }

    if (pattern.requires_line_of_sight && !field.HasLineOfSight(caster, aim)) {
        return AimRejection::NoLineOfSight;
    }

    return AimRejection::None;
}

bool IsLegalAim(const TargetPattern& pattern,
                Vec2 caster,
                Vec2 aim,
                const Battlefield& field) {
    return ExplainAim(pattern, caster, aim, field) == AimRejection::None;
}

std::vector<Vec2> LegalAimTiles(const TargetPattern& pattern,
                                Vec2 caster,
                                const Battlefield& field) {
    std::vector<Vec2> tiles;
    for (int y = 0; y < field.Height(); ++y) {
        for (int x = 0; x < field.Width(); ++x) {
            const Vec2 tile{x, y};
            if (IsLegalAim(pattern, caster, tile, field)) tiles.push_back(tile);
        }
    }
    return tiles;
}

std::vector<Vec2> ComputeAffectedTiles(const TargetPattern& pattern,
                                       Vec2 caster,
                                       Vec2 aim,
                                       const Battlefield& field) {
    if (!IsLegalAim(pattern, caster, aim, field)) return {};

    // Self-origin cards ignore wherever the cursor happens to be.
    const Vec2 effective_aim = pattern.origin == OriginRule::Self ? caster : aim;

    std::vector<Vec2> tiles = RawShapeTiles(pattern, caster, effective_aim, field);

    // Drop anything off the board, then anything the caster cannot see. Sight is
    // traced to each affected tile independently, so a blast does not spill
    // through a wall it happens to overlap.
    tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
                               [&](Vec2 tile) {
                                   if (!field.Contains(tile)) return true;
                                   if (!pattern.requires_line_of_sight) return false;
                                   return !field.HasLineOfSight(caster, tile);
                               }),
                tiles.end());

    // Reading order, then unique -- shapes can and do overlap themselves.
    std::sort(tiles.begin(), tiles.end(), [](Vec2 a, Vec2 b) { return a < b; });
    tiles.erase(std::unique(tiles.begin(), tiles.end()), tiles.end());

    return tiles;
}

}  // namespace bb
