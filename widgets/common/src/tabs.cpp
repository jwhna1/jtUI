#include "hui/widgets/common/tabs.hpp"

#include <algorithm>
#include <cmath>

#include "hui/theme/theme.hpp"

// jtUI Tabs 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

void Tabs::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (items_.empty()) {
        selected_index_ = 0;
    } else {
        selected_index_ = std::min(selected_index_, items_.size() - 1U);
    }
    mark_dirty(DirtyFlags::Paint);
}

void Tabs::add_item(std::string item) {
    items_.push_back(std::move(item));
    mark_dirty(DirtyFlags::Paint);
}

void Tabs::set_selected_index(std::size_t index) noexcept {
    const std::size_t prev = selected_index_;
    if (items_.empty()) {
        selected_index_ = 0;
    } else {
        selected_index_ = std::min(index, items_.size() - 1U);
    }
    if (!animation_primed_) {
        indicator_position_ = static_cast<float>(selected_index_);
    }
    mark_dirty(DirtyFlags::Paint);
    if (prev != selected_index_) {
        changed_.emit(selected_index_);
    }
}

std::size_t Tabs::index_at(PointF point) const noexcept {
    if (items_.empty() || !hit_test(point)) {
        return npos;
    }

    const float tab_width = frame().width / static_cast<float>(items_.size());
    if (tab_width <= 0.0F) {
        return npos;
    }

    const std::size_t index = static_cast<std::size_t>((point.x - frame().x) / tab_width);
    if (index >= items_.size()) {
        return npos;
    }
    return index;
}

void Tabs::set_hovered_index(std::size_t index) noexcept {
    if (hovered_index_ == index) {
        return;
    }

    hovered_index_ = index;
    mark_dirty(DirtyFlags::Paint);
}

void Tabs::on_mouse_move(PointF point) {
    set_hovered_index(index_at(point));
}

void Tabs::on_mouse_leave() {
    set_hovered_index(npos);
}

void Tabs::on_click(PointF point) {
    if (!enabled()) {
        return;
    }

    const std::size_t index = index_at(point);
    if (index != npos) {
        set_selected_index(index);
    }
}

bool Tabs::on_key_down(std::int32_t key_code) {
    if (!enabled() || items_.empty()) {
        return false;
    }

    constexpr std::int32_t vk_left = 0x25;
    constexpr std::int32_t vk_right = 0x27;
    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_end = 0x23;

    switch (key_code) {
    case vk_left:
        if (selected_index_ > 0) {
            set_selected_index(selected_index_ - 1U);
        }
        return true;
    case vk_right:
        if (selected_index_ + 1U < items_.size()) {
            set_selected_index(selected_index_ + 1U);
        }
        return true;
    case vk_home:
        set_selected_index(0);
        return true;
    case vk_end:
        set_selected_index(items_.size() - 1U);
        return true;
    default:
        return false;
    }
}

bool Tabs::tick(float delta) {
    const float target = static_cast<float>(selected_index_);
    if (indicator_position_ == target) {
        return false;
    }
    // 指数衰减，约 150ms 稳定到目标 tab。
    constexpr float rate = 22.0F;
    const float step = (target - indicator_position_) * std::clamp(delta * rate, 0.0F, 1.0F);
    float next = indicator_position_ + step;
    if (std::abs(next - target) < 0.005F) {
        next = target;
    }
    indicator_position_ = next;
    mark_dirty(DirtyFlags::Paint);
    return indicator_position_ != target;
}

void Tabs::paint(PaintContext& context) const {
    // 首次 paint 之后解锁动画，后续 set_selected_index 才走 tick 平滑过渡。
    animation_primed_ = true;

    const auto& c = theme::colors();

    context.fill_rounded_rect(frame(), c.bg_surface, theme::radius().md);
    context.stroke_rounded_rect(frame(), c.border, theme::radius().md, 1.0F);

    if (items_.empty()) {
        return;
    }

    const float tab_width = frame().width / static_cast<float>(items_.size());
    for (std::size_t index = 0; index < items_.size(); ++index) {
        const RectF tab_rect{
            frame().x + static_cast<float>(index) * tab_width,
            frame().y,
            tab_width,
            frame().height,
        };
        const bool selected = index == selected_index_;
        const bool hovered = index == hovered_index_;

        const Color tab_fill = selected ? c.bg_raised
                                        : (hovered ? c.field_bg_hover : c.bg_surface);
        context.fill_rounded_rect(
            RectF{tab_rect.x + 4.0F, tab_rect.y + 4.0F, tab_rect.width - 8.0F,
                  tab_rect.height - 8.0F},
            tab_fill, theme::radius().sm);

        context.draw_text(tab_rect, items_[index],
                          selected ? c.text_primary : c.text_secondary,
                          TextAlignment::Center, 13.0F, selected);
    }

    // 指示条脱离每个 tab 的循环，按插值位置单独画一条，选中切换时平滑滑过去。
    const float indicator_x = frame().x + indicator_position_ * tab_width;
    context.fill_rect(
        RectF{indicator_x, frame().y + frame().height - 3.0F, tab_width, 3.0F}, c.accent);
}

} // namespace hui
