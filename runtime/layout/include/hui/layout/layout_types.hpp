#pragma once

namespace hui {

enum class LengthUnit {
    Auto,
    Pixel,
    Fill,
};

struct Length {
    LengthUnit unit {LengthUnit::Auto};
    float value {0.0F};

    [[nodiscard]] static constexpr Length auto_value() noexcept {
        return Length {};
    }

    [[nodiscard]] static constexpr Length pixels(float px) noexcept {
        return Length {LengthUnit::Pixel, px};
    }

    [[nodiscard]] static constexpr Length fill(float weight = 1.0F) noexcept {
        return Length {LengthUnit::Fill, weight};
    }
};

struct Edges {
    float left {0.0F};
    float top {0.0F};
    float right {0.0F};
    float bottom {0.0F};
};

enum class LayoutDirection {
    Row,
    Column,
    Stack,
};

}  // namespace hui
