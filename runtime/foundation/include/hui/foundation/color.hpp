#pragma once

#include <cstdint>
#include <string_view>

namespace hui {

struct Color {
    float r {0.0F};
    float g {0.0F};
    float b {0.0F};
    float a {1.0F};

    [[nodiscard]] static constexpr Color from_rgba8(
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        std::uint8_t alpha = 255U) noexcept {
        return Color {
            static_cast<float>(red) / 255.0F,
            static_cast<float>(green) / 255.0F,
            static_cast<float>(blue) / 255.0F,
            static_cast<float>(alpha) / 255.0F,
        };
    }

    // "#RRGGBB" / "#RRGGBBAA" / "RRGGBB" / "RRGGBBAA"。非法输入返回全透明黑，便于
    // constexpr 场景下不用抛异常；token 编译期调用，一旦写错色值单测会炸出来。
    [[nodiscard]] static constexpr Color from_hex(std::string_view hex) noexcept {
        if (!hex.empty() && hex.front() == '#') {
            hex.remove_prefix(1);
        }
        if (hex.size() != 6U && hex.size() != 8U) {
            return Color {0.0F, 0.0F, 0.0F, 0.0F};
        }

        const auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        int values[4] = {0, 0, 0, 255};
        const std::size_t channels = hex.size() / 2U;
        for (std::size_t i = 0U; i < channels; ++i) {
            const int hi = nibble(hex[i * 2U]);
            const int lo = nibble(hex[i * 2U + 1U]);
            if (hi < 0 || lo < 0) {
                return Color {0.0F, 0.0F, 0.0F, 0.0F};
            }
            values[i] = (hi << 4) | lo;
        }

        return Color {
            static_cast<float>(values[0]) / 255.0F,
            static_cast<float>(values[1]) / 255.0F,
            static_cast<float>(values[2]) / 255.0F,
            static_cast<float>(values[3]) / 255.0F,
        };
    }
};

}  // namespace hui
