#pragma once

#include "hui/foundation/color.hpp"
#include "hui/media/texture_handle.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

class VideoView : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "VideoView";
    }

    [[nodiscard]] const TextureHandle& texture() const noexcept {
        return texture_;
    }

    void set_texture(TextureHandle texture) noexcept {
        texture_ = texture;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        context.fill_rect(frame(), Color::from_rgba8(23U, 28U, 36U));
        context.stroke_rect(frame(), Color::from_rgba8(84U, 96U, 112U), 1.0F);
        context.draw_text(
            frame(),
            texture_.valid() ? "VideoView" : "VideoView (placeholder)",
            Color::from_rgba8(226U, 232U, 240U),
            TextAlignment::Center);
    }

private:
    TextureHandle texture_ {};
};

}  // namespace hui
