#pragma once

// Card —— 升级版 Panel：自带 hover / pressed 视觉态、padding 内容区、点击信号。
// 设计稿里所有"块"普遍是 Card —— Panel 只负责底色，Card 负责"它能被点击、它能高亮"。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/foundation/geometry.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

struct CardPadding {
    float top {0.0F};
    float right {0.0F};
    float bottom {0.0F};
    float left {0.0F};

    static constexpr CardPadding all(float v) noexcept { return {v, v, v, v}; }
    static constexpr CardPadding xy(float x, float y) noexcept { return {y, x, y, x}; }
};

class Card : public Widget {
public:
    Card() = default;

    [[nodiscard]] std::string_view type_name() const noexcept override { return "Card"; }

    // 背景三态。hover/pressed 不显式设置时自动派生（在 base 基础上 +/- 亮度）。
    void set_background_color(Color c) noexcept {
        bg_ = c;
        bg_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_hover_background_color(Color c) noexcept {
        hover_bg_ = c;
        hover_bg_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_pressed_background_color(Color c) noexcept {
        pressed_bg_ = c;
        pressed_bg_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_border(Color color, float thickness = 1.0F) noexcept {
        border_color_ = color;
        border_thickness_ = thickness;
        mark_dirty(DirtyFlags::Paint);
    }

    // hover 时的边框（不设置时仍用原 border）。clickable 卡常用 accent border 高亮。
    void set_hover_border(Color color, float thickness = 1.0F) noexcept {
        hover_border_color_ = color;
        hover_border_thickness_ = thickness;
        hover_border_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_corner_radius(float r) noexcept {
        radius_ = std::max(0.0F, r);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_padding(CardPadding p) noexcept {
        padding_ = p;
    }

    void set_padding(float v) noexcept {
        padding_ = CardPadding::all(v);
    }

    [[nodiscard]] CardPadding padding() const noexcept { return padding_; }

    // 内容区域 —— children 用绝对坐标定位，业务调 content_rect() 取减掉 padding 后的矩形。
    [[nodiscard]] RectF content_rect() const noexcept {
        const RectF f = frame();
        const float w = std::max(0.0F, f.width - padding_.left - padding_.right);
        const float h = std::max(0.0F, f.height - padding_.top - padding_.bottom);
        return RectF{f.x + padding_.left, f.y + padding_.top, w, h};
    }

    void set_clickable(bool clickable) noexcept {
        clickable_ = clickable;
    }

    [[nodiscard]] bool clickable() const noexcept { return clickable_; }

    // 标题（可选）—— 如果设置，paint 时在卡片左上角绘制一行小标题，content_rect 自动下移。
    void set_title(std::string title) {
        title_ = std::move(title);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<>& on_clicked() noexcept { return clicked_; }

    void on_click(PointF) override {
        if (!enabled() || !clickable_) return;
        clicked_.emit();
    }

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const Color bg = resolve_bg();
        if (bg.a > 0.001F) {
            if (radius_ <= 0.0F) {
                context.fill_rect(f, bg);
            } else {
                context.fill_rounded_rect(f, bg, radius_);
            }
        }

        // 标题行（可选）
        if (!title_.empty()) {
            const auto& c = theme::colors();
            constexpr float kTitleH = 18.0F;
            context.draw_text(
                RectF{f.x + padding_.left, f.y + padding_.top, f.width - padding_.left - padding_.right, kTitleH},
                title_, c.text_secondary, TextAlignment::Leading, 12.0F, true);
        }

        const Color border = resolve_border_color();
        const float thickness = resolve_border_thickness();
        if (border.a > 0.001F && thickness > 0.0F) {
            if (radius_ <= 0.0F) {
                context.stroke_rect(f, border, thickness);
            } else {
                context.stroke_rounded_rect(f, border, radius_, thickness);
            }
        }
    }

private:
    [[nodiscard]] Color resolve_bg() const noexcept {
        const Color base = bg_set_ ? bg_ : theme::colors().bg_surface;
        if (clickable_ && pressed()) {
            return pressed_bg_set_ ? pressed_bg_ : darken(base, 0.04F);
        }
        if (clickable_ && hovered()) {
            return hover_bg_set_ ? hover_bg_ : lighten(base, 0.06F);
        }
        return base;
    }

    [[nodiscard]] Color resolve_border_color() const noexcept {
        if (clickable_ && hovered() && hover_border_set_) {
            return hover_border_color_;
        }
        return border_color_;
    }

    [[nodiscard]] float resolve_border_thickness() const noexcept {
        if (clickable_ && hovered() && hover_border_set_) {
            return hover_border_thickness_;
        }
        return border_thickness_;
    }

    static Color lighten(Color c, float k) noexcept {
        return Color{
            std::clamp(c.r + k, 0.0F, 1.0F),
            std::clamp(c.g + k, 0.0F, 1.0F),
            std::clamp(c.b + k, 0.0F, 1.0F),
            c.a,
        };
    }

    static Color darken(Color c, float k) noexcept {
        return Color{
            std::clamp(c.r - k, 0.0F, 1.0F),
            std::clamp(c.g - k, 0.0F, 1.0F),
            std::clamp(c.b - k, 0.0F, 1.0F),
            c.a,
        };
    }

    Color bg_ {};
    bool bg_set_ {false};
    Color hover_bg_ {};
    bool hover_bg_set_ {false};
    Color pressed_bg_ {};
    bool pressed_bg_set_ {false};

    Color border_color_ {};
    float border_thickness_ {0.0F};
    Color hover_border_color_ {};
    float hover_border_thickness_ {0.0F};
    bool hover_border_set_ {false};

    float radius_ {12.0F};
    CardPadding padding_ {};
    bool clickable_ {false};
    std::string title_ {};

    Signal<> clicked_ {};
};

}  // namespace hui
