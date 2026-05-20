#include "hui/widgets/common/gauge.hpp"

#include <algorithm>
#include <cmath>

// jtUI Gauge 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

void Gauge::set_range(float minimum, float maximum) noexcept {
    minimum_ = minimum;
    maximum_ = maximum < minimum ? minimum : maximum;
    value_ = std::clamp(value_, minimum_, maximum_);
    displayed_value_ = std::clamp(displayed_value_, minimum_, maximum_);
    mark_dirty(DirtyFlags::Paint);
}

void Gauge::set_value(float value) noexcept {
    value_ = std::clamp(value, minimum_, maximum_);
    if (!initialized_) {
        displayed_value_ = value_;
        initialized_ = true;
    }
    mark_dirty(DirtyFlags::Paint);
}

void Gauge::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty(DirtyFlags::Paint);
}

void Gauge::set_tone(theme::Tone tone) noexcept {
    if (tone_ == tone) {
        return;
    }
    tone_ = tone;
    mark_dirty(DirtyFlags::Paint);
}

bool Gauge::tick(float delta_seconds) {
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

void Gauge::paint(PaintContext& context) const {
    const auto& c = theme::colors();
    const Color fill = theme::color_for(tone_);
    const Color fill_hover = theme::hover_color_for(tone_);

    const float range = maximum_ - minimum_;
    const float progress = range <= 0.0F ? 0.0F : (displayed_value_ - minimum_) / range;

    context.draw_text(RectF{frame().x, frame().y, frame().width, 20.0F}, label_,
                      c.text_secondary, TextAlignment::Leading, 13.0F, true);

    const RectF track{frame().x, frame().y + 30.0F, frame().width, 12.0F};
    context.fill_rounded_rect(track, c.track, 6.0F);
    context.fill_rounded_rect(RectF{track.x, track.y, track.width * progress, track.height},
                              hovered() ? fill_hover : fill, 8.0F);
    context.stroke_rounded_rect(track, c.border, 6.0F, 1.0F);

    const int percentage = static_cast<int>(progress * 100.0F + 0.5F);
    context.draw_text(RectF{frame().x, frame().y + 48.0F, frame().width, 20.0F},
                      std::to_string(percentage) + "%", c.text_primary,
                      TextAlignment::Center, 14.0F, true);
}

} // namespace hui
