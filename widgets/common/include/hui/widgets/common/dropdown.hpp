#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// Dropdown：触发按钮 + 下拉列表。spec §13 提到的常规 widget 之一。
//
// v1 简化实现：
//   - 下拉列表画在自己 frame 下方（hit_test 在 open 时扩到包含 panel 的范围）
//   - 不做真平台层 popup window（spec 要求 popup 时再扩，与 Popover 共享方案）
//   - 键盘：Space / Enter 打开，Esc 关闭。打开后的 ↑/↓ navigation v2 候选
//   - 单选；多选 / 搜索过滤 v2

class Dropdown : public Widget {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Dropdown";
    }

    void set_items(std::vector<std::string> items) {
        items_ = std::move(items);
        if (items_.empty()) {
            selected_index_ = npos;
        } else if (selected_index_ != npos) {
            selected_index_ = std::min(selected_index_, items_.size() - 1U);
        }
        mark_dirty(DirtyFlags::Paint);
    }
    [[nodiscard]] const std::vector<std::string>& items() const noexcept { return items_; }

    void set_selected_index(std::size_t i) noexcept {
        const std::size_t prev = selected_index_;
        selected_index_ = items_.empty() ? npos : std::min(i, items_.size() - 1U);
        if (prev != selected_index_) {
            mark_dirty(DirtyFlags::Paint);
            changed_.emit(selected_index_);
        }
    }
    [[nodiscard]] std::size_t selected_index() const noexcept { return selected_index_; }

    void set_placeholder(std::string p) {
        placeholder_ = std::move(p);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool open() const noexcept { return open_; }

    void set_open(bool o) noexcept {
        if (open_ == o) return;
        open_ = o;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<std::size_t>& on_changed() noexcept { return changed_; }

    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled();
    }

    // 闭合时只命中 trigger frame；打开时扩到 trigger + panel 范围（让 panel 区域
    // 的 click 能落到自己，由 on_click 处理选中或 close）
    [[nodiscard]] bool hit_test(PointF point) const noexcept override {
        if (!visible()) return false;
        if (frame().contains(point)) return true;
        if (open_) {
            return panel_rect().contains(point);
        }
        return false;
    }

    void on_click(PointF point) override {
        if (!enabled()) return;
        if (frame().contains(point)) {
            set_open(!open_);
            return;
        }
        if (open_ && panel_rect().contains(point)) {
            const std::size_t hit = item_index_at(point);
            if (hit != npos) {
                set_selected_index(hit);
                set_open(false);
            }
            return;
        }
        // panel 外的 click（hit_test 应该不会让我们走到这里，但兜底关）
        if (open_) set_open(false);
    }

    void on_mouse_move(PointF point) override {
        if (!open_) {
            set_hovered_item(npos);
            return;
        }
        set_hovered_item(item_index_at(point));
    }

    void on_mouse_leave() override {
        set_hovered_item(npos);
    }

    bool on_key_down(std::int32_t key_code) override {
        if (!enabled()) return false;
        if (key_code == 32 || key_code == 0x0D) {  // Space / Enter
            set_open(!open_);
            return true;
        }
        if (key_code == 0x1B && open_) {  // Esc
            set_open(false);
            return true;
        }
        return false;
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const float r = theme::radius().md;

        // Trigger 按钮区域
        const Color bg = !enabled() ? c.bg_raised
                                    : (hovered() ? c.field_bg_hover : c.field_bg);
        const Color border = focused() ? c.field_border_focus : c.field_border;
        context.fill_rounded_rect(frame(), bg, r);
        context.stroke_rounded_rect(frame(), border, r, focused() ? 1.5F : 1.0F);

        // 当前值 / placeholder
        const std::string& label =
            (selected_index_ != npos && selected_index_ < items_.size())
                ? items_[selected_index_]
                : placeholder_;
        const Color label_color =
            !enabled() ? c.text_disabled
                       : (selected_index_ == npos ? c.text_muted : c.text_primary);
        context.draw_text(
            RectF{frame().x + 12.0F, frame().y, frame().width - 36.0F, frame().height},
            label, label_color, TextAlignment::Leading, 13.0F);

        // 右侧 ▼ 三角（Unicode U+25BE BLACK DOWN-POINTING SMALL TRIANGLE，UTF-8 EE 96 BE）
        context.draw_text(
            RectF{frame().x + frame().width - 24.0F, frame().y, 18.0F, frame().height},
            "\xe2\x96\xbe", c.text_muted, TextAlignment::Center, 11.0F);

        if (!open_ || items_.empty()) return;

        // Panel 背景
        const RectF panel = panel_rect();
        context.fill_rounded_rect(panel, c.bg_raised, r);
        context.stroke_rounded_rect(panel, c.border, r, 1.0F);

        // Items
        for (std::size_t i = 0; i < items_.size(); ++i) {
            const RectF item_rect = item_rect_at(i);
            if (i == selected_index_) {
                context.fill_rounded_rect(item_rect, c.accent_soft, theme::radius().sm);
            } else if (i == hovered_item_) {
                context.fill_rounded_rect(item_rect, c.field_bg_hover, theme::radius().sm);
            }
            const Color text_color =
                (i == selected_index_) ? c.accent : c.text_primary;
            context.draw_text(
                RectF{item_rect.x + 12.0F, item_rect.y, item_rect.width - 24.0F, item_rect.height},
                items_[i], text_color, TextAlignment::Leading, 13.0F);
        }
    }

    // 测试 / 计算用
    [[nodiscard]] RectF panel_rect() const noexcept {
        if (items_.empty()) return RectF{};
        constexpr float item_h = 30.0F;
        constexpr float panel_padding = 4.0F;
        const float h = item_h * static_cast<float>(items_.size()) + panel_padding * 2.0F;
        return RectF{frame().x, frame().y + frame().height + 4.0F, frame().width, h};
    }

private:
    [[nodiscard]] RectF item_rect_at(std::size_t i) const noexcept {
        constexpr float item_h = 30.0F;
        constexpr float panel_padding = 4.0F;
        const RectF panel = panel_rect();
        return RectF{panel.x + panel_padding,
                     panel.y + panel_padding + static_cast<float>(i) * item_h,
                     panel.width - panel_padding * 2.0F, item_h};
    }

    [[nodiscard]] std::size_t item_index_at(PointF point) const noexcept {
        const RectF panel = panel_rect();
        if (!panel.contains(point) || items_.empty()) return npos;
        constexpr float item_h = 30.0F;
        constexpr float panel_padding = 4.0F;
        const float local_y = point.y - (panel.y + panel_padding);
        if (local_y < 0.0F) return npos;
        const auto idx = static_cast<std::size_t>(local_y / item_h);
        return idx < items_.size() ? idx : npos;
    }

    void set_hovered_item(std::size_t i) noexcept {
        if (hovered_item_ == i) return;
        hovered_item_ = i;
        mark_dirty(DirtyFlags::Paint);
    }

    std::vector<std::string> items_{};
    std::size_t selected_index_{npos};
    std::size_t hovered_item_{npos};
    std::string placeholder_{"Select..."};
    bool open_{false};
    Signal<std::size_t> changed_{};
};

}  // namespace hui
