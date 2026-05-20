#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// 浮动提示气泡。设计成全屏 overlay widget（set_frame 到整窗），
// 自身的可见性由 anchor hover 状态 + 延迟控制。host 代码负责把 anchor
// widget 的 hover 状态转给 tooltip（因为事件分发还没全链路贯通 bubble）。
//
// 典型用法：
//   tooltip->set_text("Export project");
//   tooltip->set_anchor_rect(button->frame());
//   button->on_hover_changed().connect([t=tooltip_ptr](bool h){ t->set_hovered(h); });
//
// 动画：淡入 150ms，按下/移出立即消失；配合 show_delay_ms_ 防闪。
class Tooltip : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Tooltip";
    }

    void set_text(std::string text) {
        text_ = std::move(text);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_anchor_rect(RectF rect) noexcept {
        anchor_ = rect;
        mark_dirty(DirtyFlags::Paint);
    }

    // Host 代码在 anchor hover 变化时调；内部做延迟与动画。
    void set_hovered_anchor(bool hovered) noexcept {
        if (anchor_hovered_ == hovered) return;
        anchor_hovered_ = hovered;
        if (!hovered) {
            // 立即消失
            wait_ms_ = 0.0F;
            opacity_ = 0.0F;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    void set_show_delay_ms(float ms) noexcept { show_delay_ms_ = std::max(0.0F, ms); }

    [[nodiscard]] bool visible_now() const noexcept { return opacity_ > 0.01F; }

    // 不吃命中 —— tooltip 只显示不拦截点击。
    [[nodiscard]] bool hit_test(PointF) const noexcept override { return false; }

    bool tick(float delta) override {
        const float delta_ms = delta * 1000.0F;
        bool dirty = false;
        if (anchor_hovered_) {
            if (wait_ms_ < show_delay_ms_) {
                wait_ms_ += delta_ms;
                if (wait_ms_ >= show_delay_ms_) dirty = true;
            }
            if (wait_ms_ >= show_delay_ms_ && opacity_ < 1.0F) {
                opacity_ = std::min(1.0F, opacity_ + delta * 6.67F);  // 150ms 淡入
                dirty = true;
            }
        } else {
            wait_ms_ = 0.0F;
            if (opacity_ > 0.0F) {
                opacity_ = 0.0F;
                dirty = true;
            }
        }
        if (dirty) mark_dirty(DirtyFlags::Paint);
        return opacity_ > 0.0F && opacity_ < 1.0F;
    }

    void paint(PaintContext& context) const override {
        if (opacity_ <= 0.01F || text_.empty()) return;

        const auto& c = theme::colors();
        const float alpha = std::clamp(opacity_, 0.0F, 1.0F);

        // 气泡尺寸按文本长度粗略估。没有 measure_text 时就取一个经验字符宽度。
        const float char_w = 7.5F;
        const float pad_x = 10.0F;
        const float pad_y = 6.0F;
        const float content_w = std::max(40.0F, static_cast<float>(text_.size()) * char_w);
        const float tip_w = content_w + pad_x * 2.0F;
        const float tip_h = 28.0F;

        // 放在 anchor 上方居中；不够空间就放下面。
        float tip_x = anchor_.x + (anchor_.width - tip_w) * 0.5F;
        float tip_y = anchor_.y - tip_h - 8.0F;
        if (tip_y < frame().y + 4.0F) {
            tip_y = anchor_.y + anchor_.height + 8.0F;
        }
        tip_x = std::clamp(tip_x, frame().x + 4.0F, frame().x + frame().width - tip_w - 4.0F);

        Color bg = c.bg_raised;
        bg.a *= alpha;
        Color border = c.border_strong;
        border.a *= alpha;
        Color fg = c.text_primary;
        fg.a *= alpha;

        const RectF tip{tip_x, tip_y, tip_w, tip_h};
        context.fill_rounded_rect(tip, bg, theme::radius().sm);
        context.stroke_rounded_rect(tip, border, theme::radius().sm, 1.0F);
        context.draw_text(
            RectF{tip.x + pad_x, tip.y + pad_y, tip.width - pad_x * 2.0F, tip.height - pad_y * 2.0F},
            text_, fg, TextAlignment::Center, 12.0F);
    }

private:
    std::string text_ {};
    RectF anchor_ {};
    bool anchor_hovered_ {false};
    float show_delay_ms_ {400.0F};
    float wait_ms_ {0.0F};
    float opacity_ {0.0F};
};

}  // namespace hui
