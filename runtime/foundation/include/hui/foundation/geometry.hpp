#pragma once

namespace hui {

struct PointF {
    float x {0.0F};
    float y {0.0F};
};

struct SizeF {
    float width {0.0F};
    float height {0.0F};
};

struct RectF {
    float x {0.0F};
    float y {0.0F};
    float width {0.0F};
    float height {0.0F};

    [[nodiscard]] bool empty() const noexcept {
        return width <= 0.0F || height <= 0.0F;
    }

    [[nodiscard]] bool contains(PointF point) const noexcept {
        return !empty() && point.x >= x && point.y >= y && point.x <= (x + width) &&
               point.y <= (y + height);
    }
};

}  // namespace hui
