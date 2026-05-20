#pragma once

#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

class MetricCard : public Widget {
public:
    MetricCard() = default;

    MetricCard(std::string title, std::string value, std::string caption)
        : title_(std::move(title)), value_(std::move(value)), caption_(std::move(caption)) {
    }

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "MetricCard";
    }

    void set_title(std::string title) {
        title_ = std::move(title);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_value(std::string value) {
        value_ = std::move(value);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_caption(std::string caption) {
        caption_ = std::move(caption);
        mark_dirty(DirtyFlags::Paint);
    }

    // Tone 决定卡片上方那条强调色条和 hover 时的边框色。
    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) {
            return;
        }
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const Color accent = theme::color_for(tone_);
        const float r = theme::radius().lg;

        context.fill_rounded_rect(frame(), c.bg_raised, r);
        context.stroke_rounded_rect(frame(), hovered() ? accent : c.border, r, 1.0F);
        context.fill_rounded_rect(
            RectF {frame().x + 16.0F, frame().y + 18.0F, 34.0F, 6.0F}, accent, 3.0F);

        context.draw_text(
            RectF {frame().x + 16.0F, frame().y + 34.0F, frame().width - 32.0F, 22.0F},
            title_, c.text_secondary, TextAlignment::Leading, 12.0F);
        context.draw_text(
            RectF {frame().x + 16.0F, frame().y + 56.0F, frame().width - 32.0F, 28.0F},
            value_, c.text_primary, TextAlignment::Leading, 20.0F, true);
        context.draw_text(
            RectF {frame().x + 16.0F, frame().y + frame().height - 26.0F,
                   frame().width - 32.0F, 18.0F},
            caption_, c.text_muted, TextAlignment::Leading, 11.0F);
    }

private:
    std::string title_ {"Metric"};
    std::string value_ {"0"};
    std::string caption_ {"Ready"};
    theme::Tone tone_ {theme::Tone::Accent};
};

}  // namespace hui
