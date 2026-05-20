#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

// jtUI Dialog
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

enum class DialogHitTarget : std::uint8_t {
    None,
    Panel,
    ConfirmButton,
};

class Dialog : public Widget {
  public:
    Dialog() = default;

    Dialog(std::string title, std::string message)
        : title_(std::move(title)), message_(std::move(message)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Dialog";
    }

    [[nodiscard]] bool open() const noexcept {
        return open_;
    }

    void set_open(bool open) noexcept;

    [[nodiscard]] const std::string& title() const noexcept {
        return title_;
    }

    void set_title(std::string title);

    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

    void set_message(std::string message);

    void set_confirm_text(std::string text);

    [[nodiscard]] RectF card_rect() const noexcept;

    [[nodiscard]] RectF confirm_rect() const noexcept;

    [[nodiscard]] DialogHitTarget hit_target(PointF point) const noexcept;

    [[nodiscard]] bool modal() const noexcept {
        return modal_;
    }

    // 模态：open 时把命中区扩到父 frame，吸收 scrim 区域所有事件，让兄弟 widget
    // 收不到点击。同时 on_event 在 Target phase 把所有 Mouse 事件 base.handled = true，
    // 阻断 bubble 到 root 的祖先 click handler（如果将来 root 注册了的话）。默认开启。
    void set_modal(bool modal) noexcept;

    [[nodiscard]] bool hit_test(PointF point) const noexcept override;

    void set_confirm_hovered(bool hovered) noexcept;

    void set_confirm_pressed(bool pressed) noexcept;

    void on_mouse_move(PointF point) override;

    void on_mouse_leave() override;

    void on_press_changed(PointF point, bool pressed) override;

    void on_click(PointF point) override;

    bool on_event(Event& ev) override;

    void paint(PaintContext& context) const override;

  private:
    bool open_{false};
    bool modal_{true};
    bool confirm_hovered_{false};
    bool confirm_pressed_{false};
    std::string title_{"Dialog"};
    std::string message_{"Dialog body"};
    std::string confirm_text_{"Confirm"};
};

} // namespace hui
