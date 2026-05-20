#pragma once

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

class WindowFrame : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "WindowFrame";
    }

    [[nodiscard]] bool frameless() const noexcept {
        return frameless_;
    }

    void set_frameless(bool frameless) noexcept {
        frameless_ = frameless;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool outline_visible() const noexcept {
        return outline_visible_;
    }

    void set_outline_visible(bool visible) noexcept {
        outline_visible_ = visible;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        if (!outline_visible_) {
            return;
        }

        const Color border = frameless_ ? Color::from_rgba8(84U, 96U, 112U)
                                        : Color::from_rgba8(58U, 66U, 78U);
        context.stroke_rect(frame(), border, 1.0F);
    }

private:
    bool frameless_ {true};
    bool outline_visible_ {false};
};

}  // namespace hui
