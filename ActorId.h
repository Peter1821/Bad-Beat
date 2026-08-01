#pragma once

#include <cstdint>

namespace bb {

/// Stable handle for an actor.
///
/// Actors live in a vector owned by CombatState and are referred to by id
/// everywhere else. Ids rather than pointers or indices mean that spawning,
/// killing or reordering actors mid-fight can never leave another system
/// holding a dangling reference or an index that now names someone else.
enum class ActorId : std::uint32_t {};

/// The null handle. Ids issued to real actors start at 1, so a
/// default-constructed or cleared slot is unambiguously empty.
inline constexpr ActorId kNoActor{0};

constexpr bool IsValid(ActorId id) noexcept { return id != kNoActor; }

}  // namespace bb
