#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

enum class CheckState {
    Unchecked,
    Checked,
    Indeterminate,  // 三态（父分组部分选中的 "-" 样式）
};

class Checkbox : public Widget {
  public:
    Checkbox() = default;

    explicit Checkbox(std::string label) : label_(std::move(label)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Checkbox";
    }

    [[nodiscard]] bool checked() const noexcept {
        return state_ == CheckState::Checked;
    }

    [[nodiscard]] CheckState state() const noexcept {
        return state_;
    }

    void set_checked(bool checked) noexcept {
        set_state(checked ? CheckState::Checked : CheckState::Unchecked);
    }

    void set_state(CheckState state) noexcept {
        if (state_ == state) return;
        state_ = state;
        mark_dirty(DirtyFlags::Paint);
    }

    void toggle() noexcept {
        // Indeterminate 一次点击走到 Checked（符合大多数 web UI 的习惯）
        set_state(state_ == CheckState::Checked ? CheckState::Unchecked : CheckState::Checked);
        changed_.emit(checked());
    }

    [[nodiscard]] Signal<bool>& on_changed() noexcept { return changed_; }

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
        // Space (32) / Enter (0x0D) 都触发 toggle。这是 web/native UI 一致行为。
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

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const bool is_on = state_ != CheckState::Unchecked;
        const RectF box{frame().x, frame().y + 6.0F, 18.0F, 18.0F};

        Color box_fill = c.field_bg;
        Color border = c.field_border;
        if (!enabled()) {
            box_fill = c.bg_raised;
            border = c.border;
        } else if (is_on) {
            box_fill = c.accent;
            border = c.accent;
        } else if (hovered()) {
            box_fill = c.field_bg_hover;
            border = c.border_strong;
        }

        context.fill_rounded_rect(box, box_fill, 4.0F);
        context.stroke_rounded_rect(box, border, 4.0F, 1.0F);

        // focused 焦点环：在 box 外围画一个稍大的 1.5px accent 圈，键盘可见
        if (focused() && enabled()) {
            const RectF ring{box.x - 2.0F, box.y - 2.0F, box.width + 4.0F, box.height + 4.0F};
            context.stroke_rounded_rect(ring, c.accent, 6.0F, 1.5F);
        }

        const Color mark_color = enabled() ? c.accent_fg : c.text_disabled;
        if (state_ == CheckState::Checked) {
            // 简易 check mark —— 白色小方块。后续 PaintContext 支持 path 后换 √ 折线。
            context.fill_rounded_rect(
                RectF{box.x + 4.0F, box.y + 4.0F, box.width - 8.0F, box.height - 8.0F},
                mark_color, 2.0F);
        } else if (state_ == CheckState::Indeterminate) {
            // 横线表示"部分选中"
            context.fill_rounded_rect(
                RectF{box.x + 3.0F, box.y + box.height * 0.5F - 1.0F,
                      box.width - 6.0F, 2.0F},
                mark_color, 1.0F);
        }

        const Color label_color = enabled() ? c.text_primary : c.text_disabled;
        context.draw_text(
            RectF{frame().x + 28.0F, frame().y, frame().width - 28.0F, frame().height}, label_,
            label_color, TextAlignment::Leading, 13.0F);
    }

  private:
    CheckState state_{CheckState::Unchecked};
    std::string label_{"Checkbox"};
    Signal<bool> changed_{};
};

} // namespace hui
