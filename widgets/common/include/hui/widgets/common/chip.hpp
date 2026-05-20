#pragma once

#include <string>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// 小圆角标签。点击可切换 selected；可选 closable（画叉号，目前只绘不响应点击，
// 交互等 PaintContext 能做 path/字符命中后再补）。
class Chip : public Widget {
public:
    Chip() = default;
    explicit Chip(std::string text) : text_(std::move(text)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override { return "Chip"; }

    void set_text(std::string text) {
        text_ = std::move(text);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) return;
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool selected() const noexcept { return selected_; }

    void set_selected(bool selected) noexcept {
        if (selected_ == selected) return;
        selected_ = selected;
        changed_.emit(selected_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_closable(bool closable) noexcept {
        if (closable_ == closable) return;
        closable_ = closable;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_clickable(bool clickable) noexcept {
        clickable_ = clickable;
    }

    [[nodiscard]] Signal<bool>& on_changed() noexcept { return changed_; }

    void on_click(PointF) override {
        if (clickable_ && enabled()) {
            set_selected(!selected_);
        }
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const Color tone_color = theme::color_for(tone_);

        Color bg = c.bg_surface_alt;
        Color border = c.border;
        Color fg = c.text_primary;

        if (!enabled()) {
            bg = c.bg_raised;
            border = c.border;
            fg = c.text_disabled;
        } else if (selected_) {
            bg = tone_color;
            border = tone_color;
            fg = c.accent_fg;
        } else if (hovered()) {
            bg = c.field_bg_hover;
            border = tone_color;
        }

        const float r = frame().height * 0.5F;  // 胶囊
        context.fill_rounded_rect(frame(), bg, r);
        context.stroke_rounded_rect(frame(), border, r, 1.0F);

        const float pad_x = 12.0F;
        const float close_w = closable_ ? 18.0F : 0.0F;
        context.draw_text(
            RectF{frame().x + pad_x, frame().y, frame().width - pad_x * 2.0F - close_w, frame().height},
            text_, fg, TextAlignment::Center, 12.0F, true);

        if (closable_) {
            context.draw_text(
                RectF{frame().x + frame().width - close_w - 4.0F, frame().y,
                      close_w, frame().height},
                "\xc3\x97",  // U+00D7 MULTIPLICATION SIGN
                fg, TextAlignment::Center, 14.0F, true);
        }
    }

private:
    std::string text_ {"Chip"};
    theme::Tone tone_ {theme::Tone::Accent};
    bool selected_ {false};
    bool closable_ {false};
    bool clickable_ {true};
    Signal<bool> changed_ {};
};

}  // namespace hui
