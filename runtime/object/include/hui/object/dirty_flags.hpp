#pragma once

#include <cstdint>

namespace hui {

enum class DirtyFlags : std::uint32_t {
    None = 0,
    Structure = 1U << 0U,
    Layout = 1U << 1U,
    Paint = 1U << 2U,
    All = (1U << 0U) | (1U << 1U) | (1U << 2U),
};

[[nodiscard]] constexpr DirtyFlags operator|(DirtyFlags lhs, DirtyFlags rhs) noexcept {
    return static_cast<DirtyFlags>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr DirtyFlags& operator|=(DirtyFlags& lhs, DirtyFlags rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr DirtyFlags operator&(DirtyFlags lhs, DirtyFlags rhs) noexcept {
    return static_cast<DirtyFlags>(
        static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr DirtyFlags operator~(DirtyFlags value) noexcept {
    return static_cast<DirtyFlags>(~static_cast<std::uint32_t>(value));
}

}  // namespace hui
