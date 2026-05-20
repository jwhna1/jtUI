#pragma once

#include <algorithm>
#include <cstdint>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// 基础 Slider：水平拖动的轨道 + 拇指。范围默认 [0,1]。
// 拖动时 emit changed(value)。键盘 ←/→ 调 ±step；Home/End 跳极值；
// PageUp/PageDown 大步进（10×step）。
class Slider : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Slider";
    }

    [[nodiscard]] float minimum() const noexcept { return minimum_; }
    [[nodiscard]] float maximum() const noexcept { return maximum_; }
    [[nodiscard]] float value() const noexcept { return value_; }
    [[nodiscard]] float step() const noexcept { return step_; }

    void set_range(float minimum, float maximum) noexcept {
        minimum_ = minimum;
        maximum_ = maximum < minimum ? minimum : maximum;
        value_ = std::clamp(value_, minimum_, maximum_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_value(float value) noexcept {
        const float clamped = std::clamp(value, minimum_, maximum_);
        if (clamped == value_) {
            return;
        }
        value_ = clamped;
        changed_.emit(value_);
        mark_dirty(DirtyFlags::Paint);
    }

    // 单步步长（键盘 ←/→ 一次的增量）。0 或负值会被回落到 (max-min)/100 默认值。
    void set_step(float step) noexcept {
        step_ = step > 0.0F ? step : 0.0F;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<float>& on_changed() noexcept { return changed_; }

    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled();
    }

    // 点中轨道即跳到对应位置；按下后拖动追踪。
    void on_click(PointF point) override {
        if (!enabled()) {
            return;
        }
        set_value_from_x(point.x);
    }

    void on_mouse_move(PointF point) override {
        if (pressed() && enabled()) {
            set_value_from_x(point.x);
        }
    }

    bool on_key_down(std::int32_t key_code) override {
        if (!enabled()) return false;
        constexpr std::int32_t vk_left = 0x25;
        constexpr std::int32_t vk_right = 0x27;
        constexpr std::int32_t vk_home = 0x24;
        constexpr std::int32_t vk_end = 0x23;
        constexpr std::int32_t vk_pageup = 0x21;
        constexpr std::int32_t vk_pagedown = 0x22;
        const float effective_step = step_ > 0.0F ? step_ : (maximum_ - minimum_) * 0.01F;
        const float page_step = effective_step * 10.0F;
        switch (key_code) {
        case vk_left:
            set_value(value_ - effective_step);
            return true;
        case vk_right:
            set_value(value_ + effective_step);
            return true;
        case vk_pageup:
            set_value(value_ + page_step);  // PageUp = 大步增（与 web 习惯一致）
            return true;
        case vk_pagedown:
            set_value(value_ - page_step);
            return true;
        case vk_home:
            set_value(minimum_);
            return true;
        case vk_end:
            set_value(maximum_);
            return true;
        default:
            return false;
        }
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const float track_h = 6.0F;
        const RectF track{
            frame().x,
            frame().y + (frame().height - track_h) * 0.5F,
            frame().width,
            track_h,
        };

        // disabled：track 用 bg_raised + knob 用 text_disabled，整体灰化
        if (!enabled()) {
            context.fill_rounded_rect(track, c.bg_raised, track_h * 0.5F);
            const float t = normalized();
            const RectF filled{track.x, track.y, track.width * t, track.height};
            context.fill_rounded_rect(filled, c.border_strong, track_h * 0.5F);
            const float knob_size = 18.0F;
            const float knob_x = track.x + track.width * t - knob_size * 0.5F;
            const RectF knob{knob_x, frame().y + (frame().height - knob_size) * 0.5F, knob_size,
                              knob_size};
            context.fill_ellipse(knob, c.text_disabled);
            context.stroke_rounded_rect(knob, c.bg_raised, knob_size * 0.5F, 2.0F);
            return;
        }

        context.fill_rounded_rect(track, c.track, track_h * 0.5F);

        const float t = normalized();
        const RectF filled{track.x, track.y, track.width * t, track.height};
        context.fill_rounded_rect(filled, c.accent, track_h * 0.5F);

        const float knob_size = 18.0F;
        const float knob_x = track.x + track.width * t - knob_size * 0.5F;
        const RectF knob{knob_x, frame().y + (frame().height - knob_size) * 0.5F, knob_size,
                          knob_size};
        context.fill_ellipse(knob, hovered() ? c.accent_hover : c.accent);
        context.stroke_rounded_rect(knob, c.accent_fg, knob_size * 0.5F, 2.0F);

        // focused 焦点环：knob 外围 1.5px accent 圈
        if (focused()) {
            const RectF ring{knob.x - 3.0F, knob.y - 3.0F, knob.width + 6.0F, knob.height + 6.0F};
            context.stroke_rounded_rect(ring, c.accent, knob_size * 0.5F + 3.0F, 1.5F);
        }
    }

private:
    [[nodiscard]] float normalized() const noexcept {
        const float span = maximum_ - minimum_;
        if (span <= 0.0F) {
            return 0.0F;
        }
        return std::clamp((value_ - minimum_) / span, 0.0F, 1.0F);
    }

    void set_value_from_x(float x) noexcept {
        if (frame().width <= 0.0F) {
            return;
        }
        const float t = std::clamp((x - frame().x) / frame().width, 0.0F, 1.0F);
        set_value(minimum_ + t * (maximum_ - minimum_));
    }

    float minimum_ {0.0F};
    float maximum_ {1.0F};
    float value_ {0.0F};
    float step_ {0.0F};  // 0 = 用 (max-min)/100 默认；正值由用户指定
    Signal<float> changed_ {};
};

}  // namespace hui
