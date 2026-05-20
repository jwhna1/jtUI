#include "hui/widgets/common/dialog.hpp"

#include <algorithm>

#include "hui/theme/theme.hpp"

// jtUI Dialog 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

void Dialog::set_open(bool open) noexcept {
    open_ = open;
    mark_dirty(DirtyFlags::Paint);
}

void Dialog::set_modal(bool modal) noexcept {
    if (modal_ == modal) {
        return;
    }
    modal_ = modal;
    mark_dirty(DirtyFlags::Paint);
}

bool Dialog::hit_test(PointF point) const noexcept {
    if (!visible() || !open_) {
        return false;
    }
    if (modal_) {
        // 模态：吸收父区域所有点击。Dialog frame 在生产代码里通常等于 root frame
        // （fullscreen scrim），但即便 frame 较小，也把命中扩到 parent->frame() 上，
        // 让 scrim 区域兜底。
        const auto* parent_elem = dynamic_cast<const Element*>(parent());
        if (parent_elem != nullptr && parent_elem->frame().contains(point)) {
            return true;
        }
    }
    return frame().contains(point);
}

void Dialog::set_title(std::string title) {
    title_ = std::move(title);
    mark_dirty(DirtyFlags::Paint);
}

void Dialog::set_message(std::string message) {
    message_ = std::move(message);
    mark_dirty(DirtyFlags::Paint);
}

void Dialog::set_confirm_text(std::string text) {
    confirm_text_ = std::move(text);
    mark_dirty(DirtyFlags::Paint);
}

RectF Dialog::card_rect() const noexcept {
    const float horizontal_margin = std::clamp(frame().width * 0.16F, 28.0F, 120.0F);
    const float vertical_margin = std::clamp(frame().height * 0.16F, 28.0F, 80.0F);
    return RectF{
        frame().x + horizontal_margin,
        frame().y + vertical_margin,
        frame().width - horizontal_margin * 2.0F,
        frame().height - vertical_margin * 2.0F,
    };
}

RectF Dialog::confirm_rect() const noexcept {
    const RectF card = card_rect();
    return RectF{
        card.x + card.width - 136.0F,
        card.y + card.height - 56.0F,
        112.0F,
        32.0F,
    };
}

DialogHitTarget Dialog::hit_target(PointF point) const noexcept {
    if (!open_ || !hit_test(point)) {
        return DialogHitTarget::None;
    }

    if (confirm_rect().contains(point)) {
        return DialogHitTarget::ConfirmButton;
    }
    return DialogHitTarget::Panel;
}

void Dialog::set_confirm_hovered(bool hovered) noexcept {
    if (confirm_hovered_ == hovered) {
        return;
    }

    confirm_hovered_ = hovered;
    mark_dirty(DirtyFlags::Paint);
}

void Dialog::set_confirm_pressed(bool pressed) noexcept {
    if (confirm_pressed_ == pressed) {
        return;
    }

    confirm_pressed_ = pressed;
    mark_dirty(DirtyFlags::Paint);
}

void Dialog::on_mouse_move(PointF point) {
    set_confirm_hovered(hit_target(point) == DialogHitTarget::ConfirmButton);
}

void Dialog::on_mouse_leave() {
    set_confirm_hovered(false);
}

void Dialog::on_press_changed(PointF point, bool pressed) {
    set_confirm_pressed(pressed && hit_target(point) == DialogHitTarget::ConfirmButton);
}

void Dialog::on_click(PointF point) {
    if (hit_target(point) == DialogHitTarget::ConfirmButton) {
        set_open(false);
    }
}

bool Dialog::on_event(Event& ev) {
    EventBase& base = event_base(ev);
    // 模态：所有 Mouse 事件在 Target phase 都被吸收，不冒到祖先。先走默认路由让
    // on_click / on_press_changed / on_mouse_move 收到事件（confirm 按钮逻辑要用），
    // 再 set handled = true 截停 bubble。
    if (base.phase == EventPhase::Target && open_ && modal_) {
        if (auto* mouse = std::get_if<MouseEvent>(&ev)) {
            switch (mouse->kind) {
            case EventKind::MouseClick:
            case EventKind::MouseDown:
            case EventKind::MouseUp:
            case EventKind::MouseMove:
            case EventKind::MouseLeave:
                Widget::on_event(ev);
                base.handled = true;
                return true;
            default:
                break;
            }
        }
    }
    return Widget::on_event(ev);
}

void Dialog::paint(PaintContext& context) const {
    if (!open_) {
        return;
    }

    const auto& c = theme::colors();

    context.fill_rounded_rect(frame(), c.bg_overlay, theme::radius().md);

    const RectF card = card_rect();

    context.fill_rounded_rect(card, c.bg_raised, theme::radius().lg);
    context.stroke_rounded_rect(card, c.border, theme::radius().lg, 1.0F);

    context.draw_text(RectF{card.x + 24.0F, card.y + 20.0F, card.width - 48.0F, 28.0F}, title_,
                      c.text_primary, TextAlignment::Leading, 15.0F, true);

    context.draw_text(RectF{card.x + 24.0F, card.y + 64.0F, card.width - 48.0F, 28.0F}, message_,
                      c.text_secondary, TextAlignment::Leading, 13.0F);

    const RectF confirm = confirm_rect();
    const Color fill = confirm_pressed_ ? c.accent_pressed
                                        : (confirm_hovered_ ? c.accent_hover : c.accent);
    context.fill_rounded_rect(confirm, fill, theme::radius().sm);
    context.draw_text(confirm, confirm_text_, c.accent_fg, TextAlignment::Center, 13.0F, true);
}

} // namespace hui
