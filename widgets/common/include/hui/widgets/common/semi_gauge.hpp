#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// 半圆仪表（200° 扫过角度，对应设计图里的 60% / 80% 两个半圆样式）。
// 结构：外圈 track（灰） + active 段（tone 色） + 中心大号数字 + 下方 label。
class SemiGauge : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "SemiGauge";
    }

    void set_label(std::string label) {
        label_ = std::move(label);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_unit(std::string unit) {
        unit_ = std::move(unit);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_range(float minimum, float maximum) noexcept {
        minimum_ = minimum;
        maximum_ = maximum < minimum ? minimum : maximum;
        value_ = std::clamp(value_, minimum_, maximum_);
        displayed_ = std::clamp(displayed_, minimum_, maximum_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_value(float value) noexcept {
        value_ = std::clamp(value, minimum_, maximum_);
        if (!initialized_) {
            displayed_ = value_;
            initialized_ = true;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) return;
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool tick(float delta) override {
        const float diff = value_ - displayed_;
        if (std::abs(diff) < 0.001F) {
            if (displayed_ != value_) {
                displayed_ = value_;
                mark_dirty(DirtyFlags::Paint);
                return true;
            }
            return false;
        }
        const float speed = std::clamp(7.0F * delta, 0.0F, 1.0F);
        displayed_ += diff * speed;
        mark_dirty(DirtyFlags::Paint);
        return true;
    }

    void paint(PaintContext& context) const override {
        constexpr float pi = 3.14159265358979323846F;
        constexpr float start_deg = 170.0F;   // 从左下开始
        constexpr float sweep_deg = 200.0F;   // 扫过 200 度（稍过半圆给视觉平衡）
        constexpr int segments = 40;

        const auto& c = theme::colors();
        const Color active = theme::color_for(tone_);

        const float range = maximum_ - minimum_;
        const float progress = range <= 0.0F ? 0.0F : (displayed_ - minimum_) / range;
        const float p = std::clamp(progress, 0.0F, 1.0F);

        const float size = std::min(frame().width, frame().height);
        const float cx = frame().x + frame().width * 0.5F;
        const float cy = frame().y + size * 0.62F;  // 往下偏，给上面的数字让位
        const float outer = size * 0.42F;
        const float inner = outer - 10.0F;

        for (int i = 0; i < segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
            const float angle = (start_deg + sweep_deg * t) * pi / 180.0F;
            const PointF s{cx + std::cos(angle) * inner, cy + std::sin(angle) * inner};
            const PointF e{cx + std::cos(angle) * outer, cy + std::sin(angle) * outer};
            const bool on = t <= p;
            context.line(s, e, on ? active : c.track, on ? 3.0F : 2.0F);
        }

        const int percent = static_cast<int>(p * 100.0F + 0.5F);
        context.draw_text(
            RectF{frame().x, frame().y + size * 0.36F, frame().width, 32.0F},
            std::to_string(percent) + unit_,
            c.text_primary, TextAlignment::Center, 24.0F, true);
        if (!label_.empty()) {
            context.draw_text(
                RectF{frame().x, frame().y + size * 0.70F, frame().width, 18.0F},
                label_, c.text_muted, TextAlignment::Center, 11.0F);
        }
    }

private:
    float minimum_ {0.0F};
    float maximum_ {1.0F};
    float value_ {0.0F};
    float displayed_ {0.0F};
    bool initialized_ {false};
    std::string label_ {"Gauge"};
    std::string unit_ {"%"};
    theme::Tone tone_ {theme::Tone::Accent};
};

}  // namespace hui
