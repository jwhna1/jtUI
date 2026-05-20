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

class RadialGauge : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "RadialGauge";
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
        displayed_value_ = std::clamp(displayed_value_, minimum_, maximum_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_value(float value) noexcept {
        value_ = std::clamp(value, minimum_, maximum_);
        if (!initialized_) {
            displayed_value_ = value_;
            initialized_ = true;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    // 主色调。进度超过 warning_threshold（默认 0.78）会切到 theme.warning。
    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) {
            return;
        }
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_warning_threshold(float t) noexcept {
        warning_threshold_ = std::clamp(t, 0.0F, 1.0F);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool tick(float delta_seconds) override {
        const float delta = value_ - displayed_value_;
        if (std::abs(delta) < 0.001F) {
            if (displayed_value_ != value_) {
                displayed_value_ = value_;
                mark_dirty(DirtyFlags::Paint);
                return true;
            }
            displayed_value_ = value_;
            return false;
        }

        const float speed = animation_speed_ * std::max(delta_seconds, 0.0F);
        displayed_value_ += delta * std::clamp(speed, 0.0F, 1.0F);
        mark_dirty(DirtyFlags::Paint);
        return true;
    }

    void paint(PaintContext& context) const override {
        constexpr float pi = 3.14159265358979323846F;
        constexpr float start_degrees = 136.0F;
        constexpr float sweep_degrees = 268.0F;
        constexpr int segments = 44;

        const auto& c = theme::colors();
        const Color accent = theme::color_for(tone_);
        const Color warning = c.warning;

        const float range = maximum_ - minimum_;
        const float progress = range <= 0.0F ? 0.0F : (displayed_value_ - minimum_) / range;
        const float clamped_progress = std::clamp(progress, 0.0F, 1.0F);
        const float size = std::min(frame().width, frame().height);
        const float center_x = frame().x + frame().width * 0.5F;
        const float center_y = frame().y + size * 0.54F;
        const float outer_radius = size * 0.41F;
        const float inner_radius = outer_radius - 12.0F;

        for (int index = 0; index < segments; ++index) {
            const float segment_progress = static_cast<float>(index) / static_cast<float>(segments - 1);
            const float angle = (start_degrees + sweep_degrees * segment_progress) * pi / 180.0F;
            const PointF start {
                center_x + std::cos(angle) * inner_radius,
                center_y + std::sin(angle) * inner_radius,
            };
            const PointF end {
                center_x + std::cos(angle) * outer_radius,
                center_y + std::sin(angle) * outer_radius,
            };

            const bool active = segment_progress <= clamped_progress;
            const Color color = active
                ? (segment_progress > warning_threshold_ ? warning : accent)
                : c.track;
            context.line(start, end, color, active ? 3.0F : 2.0F);
        }

        const int percentage = static_cast<int>(clamped_progress * 100.0F + 0.5F);
        context.draw_text(
            RectF {frame().x, frame().y + size * 0.34F, frame().width, 30.0F},
            std::to_string(percentage) + unit_,
            c.text_primary, TextAlignment::Center, 20.0F, true);
        context.draw_text(
            RectF {frame().x + 10.0F, frame().y + size * 0.58F, frame().width - 20.0F, 22.0F},
            label_, c.text_muted, TextAlignment::Center, 12.0F);
    }

private:
    float minimum_ {0.0F};
    float maximum_ {1.0F};
    float value_ {0.0F};
    float displayed_value_ {0.0F};
    float animation_speed_ {7.0F};
    float warning_threshold_ {0.78F};
    bool initialized_ {false};
    std::string label_ {"Radial Gauge"};
    std::string unit_ {"%"};
    theme::Tone tone_ {theme::Tone::Accent};
};

}  // namespace hui
