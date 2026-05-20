#include "hui/widgets/basic/flex_box.hpp"

#include <algorithm>
#include <cstddef>

// jtUI FlexBox 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>），创建于 2026-04-24

namespace hui {

namespace {

bool is_row(LayoutDirection dir) noexcept {
    return dir == LayoutDirection::Row;
}

float main_of(SizeF size, LayoutDirection dir) noexcept {
    return is_row(dir) ? size.width : size.height;
}

float cross_of(SizeF size, LayoutDirection dir) noexcept {
    return is_row(dir) ? size.height : size.width;
}

SizeF make_size(float main_axis, float cross_axis, LayoutDirection dir) noexcept {
    return is_row(dir) ? SizeF{main_axis, cross_axis} : SizeF{cross_axis, main_axis};
}

RectF make_rect(float main_pos, float cross_pos, float main_size, float cross_size,
                LayoutDirection dir) noexcept {
    return is_row(dir) ? RectF{main_pos, cross_pos, main_size, cross_size}
                       : RectF{cross_pos, main_pos, cross_size, main_size};
}

} // namespace

Widget& FlexBox::append_flex(std::unique_ptr<Widget> child, FlexItem item) {
    Widget* raw = child.get();
    append_child(std::move(child));
    items_.push_back(item);
    mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    return *raw;
}

SizeF FlexBox::measure(SizeF available) const {
    const float inner_main =
        std::max(0.0F, main_of(available, direction_) -
                           (is_row(direction_) ? padding_.left + padding_.right
                                               : padding_.top + padding_.bottom));
    const float inner_cross =
        std::max(0.0F, cross_of(available, direction_) -
                           (is_row(direction_) ? padding_.top + padding_.bottom
                                               : padding_.left + padding_.right));

    float total_main = 0.0F;
    float max_cross = 0.0F;
    std::size_t visible_count = 0;
    std::size_t index = 0;
    for (const auto& node : children()) {
        const auto* widget = dynamic_cast<const Widget*>(node.get());
        if (widget == nullptr || !widget->visible()) {
            ++index;
            continue;
        }

        const FlexItem item = index < items_.size() ? items_[index] : FlexItem{};
        SizeF child_pref =
            widget->measure(make_size(inner_main, inner_cross, direction_));

        float main_size = main_of(child_pref, direction_);
        if (item.main.unit == LengthUnit::Pixel) {
            main_size = item.main.value;
        } else if (item.main.unit == LengthUnit::Fill) {
            main_size = 0.0F;
        }

        total_main += main_size;
        max_cross = std::max(max_cross, cross_of(child_pref, direction_));
        ++visible_count;
        ++index;
    }

    if (visible_count > 1) {
        total_main += gap_ * static_cast<float>(visible_count - 1);
    }

    const float outer_main = total_main + (is_row(direction_) ? padding_.left + padding_.right
                                                              : padding_.top + padding_.bottom);
    const float outer_cross = max_cross + (is_row(direction_) ? padding_.top + padding_.bottom
                                                              : padding_.left + padding_.right);
    return make_size(outer_main, outer_cross, direction_);
}

void FlexBox::arrange(RectF frame_in) {
    set_frame(frame_in);

    const float inner_main =
        std::max(0.0F, main_of(SizeF{frame_in.width, frame_in.height}, direction_) -
                           (is_row(direction_) ? padding_.left + padding_.right
                                               : padding_.top + padding_.bottom));
    const float inner_cross =
        std::max(0.0F, cross_of(SizeF{frame_in.width, frame_in.height}, direction_) -
                           (is_row(direction_) ? padding_.top + padding_.bottom
                                               : padding_.left + padding_.right));
    const float main_origin = is_row(direction_) ? frame_in.x + padding_.left
                                                 : frame_in.y + padding_.top;
    const float cross_origin = is_row(direction_) ? frame_in.y + padding_.top
                                                  : frame_in.x + padding_.left;

    struct Slot {
        Widget* widget;
        FlexItem item;
        float main_size;
        float cross_size;
    };

    std::vector<Slot> slots;
    slots.reserve(children().size());

    float fixed_total = 0.0F;
    float fill_weight_total = 0.0F;
    std::size_t visible_count = 0;
    std::size_t index = 0;
    for (const auto& node : children()) {
        auto* widget = dynamic_cast<Widget*>(node.get());
        ++index;
        if (widget == nullptr || !widget->visible()) {
            continue;
        }

        const FlexItem item = (index - 1) < items_.size() ? items_[index - 1] : FlexItem{};
        SizeF pref = widget->measure(make_size(inner_main, inner_cross, direction_));

        float main_size = 0.0F;
        if (item.main.unit == LengthUnit::Pixel) {
            main_size = item.main.value;
        } else if (item.main.unit == LengthUnit::Auto) {
            main_size = main_of(pref, direction_);
        } else {
            main_size = 0.0F;
            fill_weight_total += std::max(0.0F, item.main.value);
        }

        float cross_size = cross_of(pref, direction_);
        if (item.cross_stretch && align_ == FlexAlign::Stretch) {
            cross_size = inner_cross;
        } else {
            cross_size = std::min(cross_size, inner_cross);
        }

        fixed_total += main_size;
        slots.push_back(Slot{widget, item, main_size, cross_size});
        ++visible_count;
    }

    if (visible_count == 0) {
        return;
    }

    const float gap_total = gap_ * static_cast<float>(visible_count - 1);
    const float remaining = std::max(0.0F, inner_main - fixed_total - gap_total);

    if (fill_weight_total > 0.0F) {
        for (auto& slot : slots) {
            if (slot.item.main.unit == LengthUnit::Fill) {
                const float weight = std::max(0.0F, slot.item.main.value);
                slot.main_size = remaining * (weight / fill_weight_total);
            }
        }
    }

    float leading_space = 0.0F;
    float between_gap = gap_;
    if (fill_weight_total <= 0.0F) {
        const float used = fixed_total + gap_total;
        const float free_space = std::max(0.0F, inner_main - used);
        switch (justify_) {
        case FlexJustify::Start:
            break;
        case FlexJustify::Center:
            leading_space = free_space * 0.5F;
            break;
        case FlexJustify::End:
            leading_space = free_space;
            break;
        case FlexJustify::SpaceBetween:
            if (visible_count > 1) {
                between_gap = gap_ + free_space / static_cast<float>(visible_count - 1);
            } else {
                leading_space = free_space * 0.5F;
            }
            break;
        }
    }

    float cursor = main_origin + leading_space;
    for (auto& slot : slots) {
        float cross_pos = cross_origin;
        if (align_ == FlexAlign::Center) {
            cross_pos += std::max(0.0F, (inner_cross - slot.cross_size) * 0.5F);
        } else if (align_ == FlexAlign::End) {
            cross_pos += std::max(0.0F, inner_cross - slot.cross_size);
        }

        RectF target = make_rect(cursor, cross_pos, slot.main_size, slot.cross_size, direction_);
        slot.widget->arrange(target);
        cursor += slot.main_size + between_gap;
    }
}

} // namespace hui
