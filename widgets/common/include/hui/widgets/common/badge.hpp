#pragma once

#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

enum class BadgeTone {
    Accent,
    Success,
    Warning,
    Danger,
    Info,
    Neutral,
};

class Badge : public Widget {
public:
    Badge() = default;

    explicit Badge(std::string text)
        : text_(std::move(text)) {
    }

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Badge";
    }

    void set_text(std::string text) {
        text_ = std::move(text);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_tone(BadgeTone tone) noexcept {
        if (tone_ == tone) {
            return;
        }
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        Color bg = c.accent;
        Color fg = c.accent_fg;
        switch (tone_) {
        case BadgeTone::Accent:  bg = c.accent;    fg = c.accent_fg; break;
        case BadgeTone::Success: bg = c.success;   fg = c.accent_fg; break;
        case BadgeTone::Warning: bg = c.warning;   fg = c.text_inverse; break;
        case BadgeTone::Danger:  bg = c.danger;    fg = c.text_inverse; break;
        case BadgeTone::Info:    bg = c.info;      fg = c.text_inverse; break;
        case BadgeTone::Neutral: bg = c.bg_raised; fg = c.text_primary; break;
        }
        context.fill_rounded_rect(frame(), bg, frame().height / 2.0F);
        context.draw_text(frame(), text_, fg, TextAlignment::Center, 12.0F, true);
    }

private:
    std::string text_ {"Badge"};
    BadgeTone tone_ {BadgeTone::Accent};
};

}  // namespace hui
