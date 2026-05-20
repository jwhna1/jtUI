#pragma once

#include <cstdint>
#include <memory>

#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// Toolbar：水平 / 垂直工具栏容器。spec §13 提到的常规 widget 之一。
//
// 与 FlexBox 的区别：
//   - 自带视觉（背景 + 边框 + radius），使用方一行 set_orientation 就有"工具栏"
//     语义；FlexBox 只是布局原语，要用方自己画背景
//   - 简化的子 widget 添加（add_item）—— 子 widget 走 measure 拿固有尺寸
//   - 不支持复杂的 fill / shrink 权重，简单等距排列即可
//
// 复杂场景（grouped + separator + overflow menu）等真有需求再扩。

enum class ToolbarOrientation : std::uint8_t {
    Horizontal,
    Vertical,
};

class Toolbar : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Toolbar";
    }

    void set_orientation(ToolbarOrientation o) noexcept {
        if (orientation_ == o) return;
        orientation_ = o;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }
    [[nodiscard]] ToolbarOrientation orientation() const noexcept { return orientation_; }

    void set_padding(float p) noexcept {
        if (padding_ == p) return;
        padding_ = p;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }
    void set_gap(float g) noexcept {
        if (gap_ == g) return;
        gap_ = g;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }
    void set_show_background(bool b) noexcept {
        if (show_background_ == b) return;
        show_background_ = b;
        mark_dirty(DirtyFlags::Paint);
    }

    Widget& add_item(std::unique_ptr<Widget> item) {
        Widget* raw = item.get();
        append_child(std::move(item));
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
        return *raw;
    }

    void arrange(RectF frame_in) override {
        set_frame(frame_in);
        const float inner_x = frame_in.x + padding_;
        const float inner_y = frame_in.y + padding_;
        const float inner_w = std::max(frame_in.width - padding_ * 2.0F, 0.0F);
        const float inner_h = std::max(frame_in.height - padding_ * 2.0F, 0.0F);

        // 简单等距：横向时每个 child 用自己 measure 的宽度，高度撑满 inner_h；
        // 竖向反之。不做 overflow 处理，超出就裁掉
        if (orientation_ == ToolbarOrientation::Horizontal) {
            float cursor = inner_x;
            for (const auto& child : children()) {
                auto* w = dynamic_cast<Widget*>(child.get());
                if (w == nullptr) continue;
                const SizeF s = w->measure(SizeF{inner_w, inner_h});
                w->arrange(RectF{cursor, inner_y, s.width, inner_h});
                cursor += s.width + gap_;
            }
        } else {
            float cursor = inner_y;
            for (const auto& child : children()) {
                auto* w = dynamic_cast<Widget*>(child.get());
                if (w == nullptr) continue;
                const SizeF s = w->measure(SizeF{inner_w, inner_h});
                w->arrange(RectF{inner_x, cursor, inner_w, s.height});
                cursor += s.height + gap_;
            }
        }
    }

    void paint(PaintContext& context) const override {
        if (!show_background_) return;
        const auto& c = theme::colors();
        const float r = theme::radius().md;
        context.fill_rounded_rect(frame(), c.bg_surface, r);
        context.stroke_rounded_rect(frame(), c.border, r, 1.0F);
    }

private:
    ToolbarOrientation orientation_{ToolbarOrientation::Horizontal};
    float padding_{8.0F};
    float gap_{8.0F};
    bool show_background_{true};
};

}  // namespace hui
