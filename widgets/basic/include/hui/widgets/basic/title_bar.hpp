#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

enum class TitleBarHitTarget : std::uint8_t {
    None,
    DragRegion,
    MinimizeButton,
    MaximizeButton,
    CloseButton,
};

class TitleBar : public Widget {
  public:
    TitleBar() = default;

    explicit TitleBar(std::string title) : title_(std::move(title)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "TitleBar";
    }

    [[nodiscard]] const std::string& title() const noexcept {
        return title_;
    }

    void set_title(std::string title) {
        title_ = std::move(title);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] RectF minimize_button_rect() const noexcept {
        return RectF{frame().x + frame().width - 138.0F, frame().y, 46.0F, frame().height};
    }

    [[nodiscard]] RectF maximize_button_rect() const noexcept {
        return RectF{frame().x + frame().width - 92.0F, frame().y, 46.0F, frame().height};
    }

    [[nodiscard]] RectF close_button_rect() const noexcept {
        return RectF{frame().x + frame().width - 46.0F, frame().y, 46.0F, frame().height};
    }

    [[nodiscard]] TitleBarHitTarget hit_target(PointF point) const noexcept {
        if (!hit_test(point)) {
            return TitleBarHitTarget::None;
        }
        if (close_button_rect().contains(point)) {
            return TitleBarHitTarget::CloseButton;
        }
        if (maximize_button_rect().contains(point)) {
            return TitleBarHitTarget::MaximizeButton;
        }
        if (minimize_button_rect().contains(point)) {
            return TitleBarHitTarget::MinimizeButton;
        }
        return TitleBarHitTarget::DragRegion;
    }

    void set_hovered_target(TitleBarHitTarget target) noexcept {
        if (hovered_target_ == target) {
            return;
        }

        hovered_target_ = target;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_pressed_target(TitleBarHitTarget target) noexcept {
        if (pressed_target_ == target) {
            return;
        }

        pressed_target_ = target;
        mark_dirty(DirtyFlags::Paint);
    }

    void on_mouse_move(PointF point) override {
        set_hovered_target(hit_target(point));
    }

    void on_mouse_leave() override {
        set_hovered_target(TitleBarHitTarget::None);
    }

    void on_press_changed(PointF point, bool pressed) override {
        set_pressed_target(pressed ? hit_target(point) : TitleBarHitTarget::None);
    }

    [[nodiscard]] WindowAction window_action_at(PointF point) const noexcept override {
        switch (hit_target(point)) {
        case TitleBarHitTarget::DragRegion:
            return WindowAction::DragMove;
        case TitleBarHitTarget::MinimizeButton:
            return WindowAction::Minimize;
        case TitleBarHitTarget::MaximizeButton:
            return WindowAction::ToggleMaximize;
        case TitleBarHitTarget::CloseButton:
            return WindowAction::Close;
        case TitleBarHitTarget::None:
            break;
        }

        return WindowAction::None;
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        context.fill_rect(frame(), c.bg_raised);

        const RectF title_rect{frame().x + 16.0F, frame().y, frame().width - 170.0F,
                               frame().height};
        context.draw_text(title_rect, title_, c.text_primary, TextAlignment::Leading, 13.0F, true);

        const auto paint_button = [&context, &c, this](RectF rect, TitleBarHitTarget target,
                                                       std::string label) {
            Color fill = c.bg_raised;
            if (hovered_target_ == target) {
                fill = target == TitleBarHitTarget::CloseButton ? c.danger : c.field_bg_hover;
            }
            if (pressed_target_ == target) {
                fill = target == TitleBarHitTarget::CloseButton ? c.danger_hover : c.field_bg_active;
            }

            const Color label_color =
                (pressed_target_ == target || hovered_target_ == target) &&
                        target == TitleBarHitTarget::CloseButton
                    ? c.text_inverse
                    : c.text_primary;

            context.fill_rounded_rect(
                RectF{rect.x + 6.0F, rect.y + 7.0F, rect.width - 12.0F, rect.height - 14.0F}, fill,
                7.0F);
            context.draw_text(rect, std::move(label), label_color, TextAlignment::Center, 13.0F,
                              true);
        };

        paint_button(minimize_button_rect(), TitleBarHitTarget::MinimizeButton, "-");
        paint_button(maximize_button_rect(), TitleBarHitTarget::MaximizeButton, "[]");
        paint_button(close_button_rect(), TitleBarHitTarget::CloseButton, "x");
    }

  private:
    std::string title_{"jtUI"};
    TitleBarHitTarget hovered_target_{TitleBarHitTarget::None};
    TitleBarHitTarget pressed_target_{TitleBarHitTarget::None};
};

} // namespace hui
