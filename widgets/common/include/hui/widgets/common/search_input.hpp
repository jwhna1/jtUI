#pragma once

// SearchInput —— 搜索框组件。结构 = 圆角描边底 + 前缀 search 图标 + borderless TextInput
// + 后缀快捷键徽章（如 [F]）。
//
// 用法：
//   auto s = std::make_unique<jtui::SearchInput>("Search");
//   s->set_shortcut_label("F");
//   s->set_frame(RectF{x, y, w, 36});
//   s->relayout();                              // jtUI 惯例：set_frame 之后手动 layout 子
//   s->text_property().changed().connect(...);  // 订阅输入变化
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <memory>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/foundation/geometry.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/property.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"
#include "hui/widgets/basic/icon.hpp"
#include "hui/widgets/common/text_input.hpp"

namespace hui {

class SearchInput : public Widget {
public:
    SearchInput() {
        build_();
    }

    explicit SearchInput(std::string placeholder) {
        build_();
        text_input_->set_placeholder(std::move(placeholder));
    }

    [[nodiscard]] std::string_view type_name() const noexcept override { return "SearchInput"; }

    void set_placeholder(std::string s) { text_input_->set_placeholder(std::move(s)); }

    // 设置后缀显示的快捷键提示徽章；空串关闭显示。
    void set_shortcut_label(std::string s) {
        shortcut_ = std::move(s);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] const std::string& shortcut_label() const noexcept { return shortcut_; }

    void set_corner_radius(float r) noexcept {
        radius_ = r;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_background_color(Color c) noexcept {
        bg_ = c;
        bg_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_border(Color color, float thickness = 1.0F) noexcept {
        border_color_ = color;
        border_thickness_ = thickness;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_icon_color(Color c) noexcept {
        icon_->set_color(c);
        icon_color_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_shortcut_badge_color(Color bg, Color fg) noexcept {
        badge_bg_ = bg;
        badge_fg_ = fg;
        badge_color_set_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] TextInput* text_input() noexcept { return text_input_; }

    [[nodiscard]] Property<std::string>& text_property() noexcept {
        return text_input_->text_property();
    }

    [[nodiscard]] const std::string& text() const noexcept { return text_input_->text(); }

    void set_text(std::string t) { text_input_->set_text(std::move(t)); }

    // jtUI 惯例：业务调 set_frame 后再调 relayout 同步内部子 widget 位置。
    // 像 gallery sections 的 apply_layout 模式。
    void relayout() {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const float h = f.height;
        const float pad = h * 0.25F;          // 内左右内边距
        const float icon_size = h * 0.5F;     // 图标尺寸
        const float badge_w = shortcut_.empty() ? 0.0F : (h * 0.7F);
        const float badge_gap = shortcut_.empty() ? 0.0F : (h * 0.2F);

        // 前缀图标
        icon_->set_size_px(icon_size);
        icon_->set_frame(RectF{
            f.x + pad,
            f.y + (h - icon_size) * 0.5F,
            icon_size,
            icon_size,
        });

        // 中间 TextInput：图标右侧到（快捷键徽章 - gap）之间。
        // v1.19 (2026-05-14): TextInput frame 改为满高 = h（之前用 h * 0.7 = 25.2px
        // 偏矮，DirectWrite line_height 可能 > 25.2，TextInput 内部居中算法会算
        // 出负偏移让 placeholder/cursor 上溢被 SearchInput 边缘剪）。满高让
        // TextInput 内单行居中有充足空间，placeholder 与 cursor 居中显示。
        const float input_x = f.x + pad + icon_size + h * 0.25F;
        const float input_right = f.x + f.width - pad - badge_w - badge_gap;
        const float input_w = std::max(0.0F, input_right - input_x);
        text_input_->set_frame(RectF{input_x, f.y, input_w, h});
    }

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const Color bg = bg_set_ ? bg_ : theme::colors().bg_surface_alt;
        if (bg.a > 0.001F) {
            context.fill_rounded_rect(f, bg, radius_);
        }
        if (border_color_.a > 0.001F && border_thickness_ > 0.0F) {
            context.stroke_rounded_rect(f, border_color_, radius_, border_thickness_);
        }

        // 后缀快捷键徽章
        if (!shortcut_.empty()) {
            const float h = f.height;
            const float badge_w = h * 0.7F;
            const float badge_h = h * 0.55F;
            const float bx = f.x + f.width - h * 0.25F - badge_w;
            const float by = f.y + (h - badge_h) * 0.5F;
            const RectF badge{bx, by, badge_w, badge_h};

            const Color bg_c = badge_color_set_ ? badge_bg_ : theme::colors().bg_raised;
            const Color fg_c = badge_color_set_ ? badge_fg_ : theme::colors().text_secondary;
            context.fill_rounded_rect(badge, bg_c, badge.height * 0.25F);
            context.draw_text(badge, shortcut_, fg_c, TextAlignment::Center,
                              badge_h * 0.55F, true);
        }
    }

private:
    void build_() {
        auto icon = std::make_unique<CodiconIcon>("search");
        icon_ = icon.get();
        append_child(std::move(icon));

        auto input = std::make_unique<TextInput>();
        input->set_borderless(true);
        text_input_ = input.get();
        append_child(std::move(input));
    }

    CodiconIcon* icon_ {nullptr};
    TextInput* text_input_ {nullptr};

    std::string shortcut_ {};
    float radius_ {12.0F};
    Color bg_ {};
    bool bg_set_ {false};
    Color border_color_ {};
    float border_thickness_ {0.0F};
    bool icon_color_set_ {false};
    Color badge_bg_ {};
    Color badge_fg_ {};
    bool badge_color_set_ {false};
};

}  // namespace hui
