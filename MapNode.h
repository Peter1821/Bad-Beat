#pragma once

#include <cstdint>
#include <vector>

namespace bb {

/// What happens when a node is entered.
///
/// Shop and Event are placed by the generator but have no behaviour yet -- they
/// arrive with their content in milestone 8. Nothing emits them until then, so
/// the map never shows a stop that does nothing.
enum class NodeType : std::uint8_t {
    Combat,
    Elite,   ///< Harder fight, better reward.
    Rest,    ///< Heal, or later upgrade a card -- never both.
    Shop,
    Event,
    Boss,
};

/// One stop on the run map.
struct MapNode {
    int row = 0;
    int column = 0;
    NodeType type = NodeType::Combat;

    /// Indices of the nodes reachable from here, all on the next row up.
    std::vector<int> next;
};

}  // namespace bb
