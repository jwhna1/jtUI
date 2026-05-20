#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

class Switch : public Widget {
  public:
    Switch() = default;

    explicit Switch(std::string label) : label_(std::move(label)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Switch";
    }

    [[nodiscard]] bool checked() const noexcept {
        return checked_;
    }

    void set_checked(bool checked) noexcept {
        if (checked_ == checked) {
            return;
        }

        checked_ = checked;
        // 首次渲染之前的 set_checked（通常是 main.cpp 里初始化调用）直接 snap，
        // 避免窗口一出来从 0 动画到 1。渲染之后的 set_checked 交给 tick 平滑过渡。
        if (!animation_primed_) {
            position_ = checked_ ? 1.0F : 0.0F;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    void toggle() {
        set_checked(!checked_);
        changed_.emit(checked_);
    }

    void on_click(PointF) override {
        if (enabled()) {
            toggle();
        }
    }

    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled();
    }

    bool on_key_down(std::int32_t key_code) override {
        if (!enabled()) return false;
        if (key_code == 32 || key_code == 0x0D) {
            toggle();
            return true;
        }
        return false;
    }

    [[nodiscard]] const std::string& label() const noexcept {
        return label_;
    }

    void set_label(std::string label) {
        label_ = std::move(label);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<bool>& on_changed() noexcept {
        return changed_;
    }

    void emit_changed() {
        changed_.emit(checked_);
    }

    bool tick(float delta) override {
        const float target = checked_ ? 1.0F : 0.0F;
        if (position_ == target) {
            return false;
        }
        constexpr float rate = 20.0F;
        const float step = (target - position_) * std::clamp(delta * rate, 0.0F, 1.0F);
        float next = position_ + step;
        if (std::abs(next - target) < 0.005F) {
            next = target;
        }
        position_ = next;
        mark_dirty(DirtyFlags::Paint);
        return position_ != target;
    }

    void paint(PaintContext& context) const override {
        animation_primed_ = true;
        const auto& c = theme::colors();
        const float pos = std::clamp(position_, 0.0F, 1.0F);
        const RectF track{frame().x, frame().y + 4.0F, 48.0F, 26.0F};

        // Disabled：整体灰化，knob 走灰色，track 不插值。
        if (!enabled()) {
            const Color off = c.bg_raised;
            const Color on = c.border_strong;
            const Color track_color{
                off.r + (on.r - off.r) * pos,
                off.g + (on.g - off.g) * pos,
                off.b + (on.b - off.b) * pos,
                off.a + (on.a - off.a) * pos,
            };
            context.fill_rounded_rect(track, track_color, 13.0F);
            const float knob_x = track.x + 3.0F + 21.0F * pos;
            context.fill_ellipse(RectF{knob_x, track.y + 3.0F, 20.0F, 20.0F}, c.text_disabled);
            context.draw_text(
                RectF{frame().x + 62.0F, frame().y, frame().width - 62.0F, frame().height},
                label_, c.text_disabled, TextAlignment::Leading, 13.0F, true);
            return;
        }

        const Color off_color = c.track;
        const Color on_color = hovered() ? c.accent_hover : c.accent;
        const Color track_color{
            off_color.r + (on_color.r - off_color.r) * pos,
            off_color.g + (on_color.g - off_color.g) * pos,
            off_color.b + (on_color.b - off_color.b) * pos,
            off_color.a + (on_color.a - off_color.a) * pos,
        };
        context.fill_rounded_rect(track, track_color, 13.0F);

        const float knob_x = track.x + 3.0F + 21.0F * pos;
        context.fill_ellipse(RectF{knob_x, track.y + 3.0F, 20.0F, 20.0F}, c.accent_fg);

        // focused 焦点环：track 外围一圈
        if (focused()) {
            const RectF ring{track.x - 2.0F, track.y - 2.0F, track.width + 4.0F, track.height + 4.0F};
            context.stroke_rounded_rect(ring, c.accent, 15.0F, 1.5F);
        }

        context.draw_text(
            RectF{frame().x + 62.0F, frame().y, frame().width - 62.0F, frame().height}, label_,
            c.text_primary, TextAlignment::Leading, 13.0F, true);
    }

  private:
    bool checked_{false};
    float position_{0.0F};
    mutable bool animation_primed_{false};
    std::string label_{"Switch"};
    Signal<bool> changed_{};
};

} // namespace hui
