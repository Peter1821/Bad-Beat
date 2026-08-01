#include "map/RunMap.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <utility>

namespace bb {
namespace {

/// Picks a type for a node partway up the map.
///
/// Row 0 is always a plain fight: the player opens at full health with a known
/// deck, so a rest would be wasted and an elite would be a coin flip before they
/// have anything to make decisions with.
NodeType RollType(Rng& rng, int row, bool rest_already_on_row, bool shop_already_on_row) {
    if (row == 0) return NodeType::Combat;

    const int roll = rng.Range(1, 100);
    if (roll <= 18 && !rest_already_on_row) return NodeType::Rest;
    if (roll <= 30 && !shop_already_on_row) return NodeType::Shop;
    if (roll <= 48) return NodeType::Event;
    if (roll <= 68) return NodeType::Elite;
    return NodeType::Combat;
}

}  // namespace

RunMap RunMap::Generate(Rng& rng, int rows, int width, int paths) {
    assert(rows >= 2 && width >= 1 && paths >= 1);

    RunMap map;
    map.rows_ = rows;

    const int last_row = rows - 1;

    // Walk each route from the bottom, recording which slots it touches and the
    // edges between them. Slots nothing walks through are never built.
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> links;

    for (int path = 0; path < paths; ++path) {
        int column = rng.Range(0, width - 1);

        for (int row = 0; row < last_row; ++row) {
            // Everything on the final ordinary row feeds the single boss, which
            // sits in the middle column. Putting it at the edge would leave the
            // far side of the map joining it across two columns, which draws as
            // a long smear rather than a line.
            const int next_column =
                (row + 1 == last_row)
                    ? width / 2
                    : std::clamp(column + rng.Range(-1, 1), 0, width - 1);

            auto& targets = links[{row, column}];
            const std::pair<int, int> destination{row + 1, next_column};
            if (std::find(targets.begin(), targets.end(), destination) == targets.end()) {
                targets.push_back(destination);
            }

            column = next_column;
        }
    }

    // Every slot that any route touched, in row-major order so node indices
    // read bottom-to-top, left-to-right.
    std::map<std::pair<int, int>, int> index_of;

    const auto claim = [&](int row, int column) {
        const std::pair<int, int> key{row, column};
        if (index_of.count(key) != 0) return;

        index_of[key] = 0;  // placeholder; real indices assigned below
    };

    for (const auto& [from, targets] : links) {
        claim(from.first, from.second);
        for (const auto& to : targets) claim(to.first, to.second);
    }

    int next_index = 0;
    for (auto& [key, index] : index_of) index = next_index++;

    map.nodes_.resize(index_of.size());

    // At most one of each service per row, so a row never turns into a shopping
    // arcade and the player always has a fight available if they want one.
    bool rest_on_row[64] = {};
    bool shop_on_row[64] = {};

    for (const auto& [key, index] : index_of) {
        MapNode& node = map.nodes_[static_cast<std::size_t>(index)];
        node.row = key.first;
        node.column = key.second;

        if (node.row == last_row) {
            node.type = NodeType::Boss;
        } else {
            node.type = RollType(rng, node.row, rest_on_row[node.row], shop_on_row[node.row]);
            if (node.type == NodeType::Rest) rest_on_row[node.row] = true;
            if (node.type == NodeType::Shop) shop_on_row[node.row] = true;
        }
    }

    for (const auto& [from, targets] : links) {
        MapNode& node = map.nodes_[static_cast<std::size_t>(index_of.at(from))];
        for (const auto& to : targets) node.next.push_back(index_of.at(to));
        std::sort(node.next.begin(), node.next.end());
    }

    return map;
}

std::vector<int> RunMap::NodesInRow(int row) const {
    std::vector<int> result;
    for (int i = 0; i < NodeCount(); ++i) {
        if (nodes_[static_cast<std::size_t>(i)].row == row) result.push_back(i);
    }
    return result;
}

std::vector<int> RunMap::OptionsFrom(int index) const {
    if (index < 0) return NodesInRow(0);
    if (index >= NodeCount()) return {};
    return nodes_[static_cast<std::size_t>(index)].next;
}

int RunMap::BossIndex() const {
    const auto top = NodesInRow(rows_ - 1);
    return top.empty() ? -1 : top.front();
}

}  // namespace bb
