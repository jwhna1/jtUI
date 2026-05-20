#include "hui/event/dispatcher.hpp"

#include <algorithm>

#include "hui/object/widget.hpp"

// jtUI 事件分发器实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>），创建于 2026-04-24

namespace hui {

namespace {

void apply_hover_state(Widget* widget, PointF point, bool hovered) {
    if (widget == nullptr) {
        return;
    }

    widget->set_hovered(hovered);
    if (hovered) {
        widget->on_mouse_move(point);
    } else {
        widget->on_mouse_leave();
    }
}

// 按下/抬起的视觉状态（pressed 标志）由 dispatcher 直接设，因为这是目标 widget
// 自身的 paint 状态；on_press_changed 业务逻辑则通过 dispatch_event_along_path
// (MouseDown/MouseUp) 在 capture/target/bubble 链上分发，让父 widget 有机会拦截。
void apply_pressed_state(Widget* widget, bool pressed) {
    if (widget == nullptr) {
        return;
    }
    widget->set_pressed(pressed);
}

void dispatch_press_event_to_path(const std::vector<Widget*>& path, PointF point, bool pressed,
                                    MouseButton button) {
    if (path.empty()) {
        return;
    }
    MouseEvent mouse{};
    mouse.kind = pressed ? EventKind::MouseDown : EventKind::MouseUp;
    mouse.type = pressed ? EventType::MouseDown : EventType::MouseUp;
    mouse.position = point;
    mouse.pressed = pressed;
    mouse.button = button;  // v1.3: 透传 L/R/M, 让 widget 区分左右键
    Event event = mouse;
    EventDispatcher::dispatch_event_along_path(path, event);
}

Widget* hit_test_recursive(Widget& widget, PointF point) {
    if (!widget.hit_test(point)) {
        return nullptr;
    }

    // v1.19 (2026-05-04): widget 可声明在某区域抢断 children — 让自己成为 leaf
    // hit target (state.pressed), 后续 press 期间 MouseMove 全部送给自己。
    // 典型: ScrollView 滚动条 thumb 区域。
    if (widget.intercepts_children_at(point)) {
        return &widget;
    }

    const auto& children = widget.children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto* child_widget = dynamic_cast<Widget*>((*it).get());
        if (child_widget == nullptr) {
            continue;
        }

        if (Widget* hit = hit_test_recursive(*child_widget, point)) {
            return hit;
        }
    }

    return &widget;
}

void build_path_recursive(Widget& widget, PointF point, std::vector<Widget*>& path) {
    if (!widget.hit_test(point)) {
        return;
    }

    path.push_back(&widget);

    // v1.19: 与 hit_test_recursive 同款 — 抢断 children, 自己作为 path 末端。
    if (widget.intercepts_children_at(point)) {
        return;
    }

    const auto& children = widget.children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto* child_widget = dynamic_cast<Widget*>((*it).get());
        if (child_widget == nullptr) {
            continue;
        }

        const std::size_t before = path.size();
        build_path_recursive(*child_widget, point, path);
        if (path.size() > before) {
            return;
        }
    }
}

} // namespace

Widget* EventDispatcher::hit_test(Widget& root, PointF point) {
    return hit_test_recursive(root, point);
}

void EventDispatcher::build_path(Widget& root, PointF point, std::vector<Widget*>& out) {
    out.clear();
    build_path_recursive(root, point, out);
}

void EventDispatcher::build_focus_path(Widget* focused, std::vector<Widget*>& out) {
    out.clear();
    while (focused != nullptr) {
        out.push_back(focused);
        Node* p = focused->parent();
        focused = dynamic_cast<Widget*>(p);
    }
    // 当前顺序是 focused → root，反转得到 root → focused（与 build_path 保持一致）
    std::reverse(out.begin(), out.end());
}

bool EventDispatcher::dispatch_event_along_path(const std::vector<Widget*>& path, Event& event) {
    if (path.empty()) {
        return false;
    }

    EventBase& base = event_base(event);

    // Capture: root → target.parent（不含 target）
    base.phase = EventPhase::Capture;
    if (path.size() > 1) {
        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            Widget* node = path[i];
            if (node == nullptr) {
                continue;
            }
            node->on_event(event);
            if (base.handled) {
                return true;
            }
        }
    }

    // Target
    base.phase = EventPhase::Target;
    if (Widget* target = path.back(); target != nullptr) {
        target->on_event(event);
        if (base.handled) {
            return true;
        }
    }

    // Bubble: target.parent → root
    base.phase = EventPhase::Bubble;
    if (path.size() > 1) {
        for (std::size_t i = path.size() - 1; i > 0; --i) {
            Widget* ancestor = path[i - 1];
            if (ancestor == nullptr) {
                continue;
            }
            ancestor->on_event(event);
            if (base.handled) {
                return true;
            }
        }
    }

    return base.handled;
}

bool EventDispatcher::dispatch_pointer_move(Widget* root, PointF point, EventState& state) {
    bool state_changed = false;

    Widget* hit = (root != nullptr) ? hit_test_recursive(*root, point) : nullptr;

    if (state.pressed != nullptr) {
        // press capture：按下期间 move 优先送 pressed，同步维护 hover 的视觉状态。
        //
        // v1.3 (2026-04-28): 改为走 on_event(MouseMove) 让 widget 通过统一接口
        // 拦 (MarkdownText 拖选 / 自定义 drag-and-drop)。Widget::on_event 默认
        // 实现仍把 target phase 的 MouseMove fan-out 到 on_mouse_move 虚函数,
        // 旧 widget (Button / ScrollView / TextInput 等) 不需改一行就保持原行为。
        // 旧版本直接调 state.pressed->on_mouse_move(point) 跳过 on_event 路径,
        // override on_event 的 widget 收不到 press 期间的 MouseMove (chat 拖选 bug 根因)。
        MouseEvent move{};
        move.kind = EventKind::MouseMove;
        move.type = EventType::MouseMove;
        move.position = point;
        Event event = move;
        EventBase& base = event_base(event);
        base.phase = EventPhase::Target;
        state.pressed->on_event(event);
        state_changed = true;
    }

    if (hit != state.hovered) {
        apply_hover_state(state.hovered, point, false);
        state.hovered = hit;
        state_changed = true;
    }

    apply_hover_state(state.hovered, point, state.hovered != nullptr);

    return state_changed || state.hovered != nullptr;
}

Widget* EventDispatcher::dispatch_pointer_down(Widget* root, PointF point, EventState& state,
                                                 MouseButton button) {
    if (root == nullptr) {
        return nullptr;
    }

    std::vector<Widget*> path;
    build_path_recursive(*root, point, path);
    Widget* hit = path.empty() ? nullptr : path.back();

    state.pressed = hit;
    apply_pressed_state(hit, true);
    if (!path.empty()) {
        // 按下事件沿三阶段链分发，让 capture/bubble 阶段的祖先有机会拦截或观察。
        dispatch_press_event_to_path(path, point, true, button);
    }
    return hit;
}

bool EventDispatcher::dispatch_pointer_up(Widget* root, PointF point, EventState& state,
                                          Widget*& out_activation,
                                          MouseButton button) {
    out_activation = nullptr;

    std::vector<Widget*> path;
    Widget* hit = nullptr;
    if (root != nullptr) {
        build_path_recursive(*root, point, path);
        hit = path.empty() ? nullptr : path.back();
    }

    if (state.pressed != nullptr) {
        if (state.pressed == hit) {
            // 抬起在同一个 widget 上：先沿 path 走 MouseUp 链；click 则交给上层
            // （application.cpp 处理 window_action 优先级再触发 MouseClick 分发）。
            dispatch_press_event_to_path(path, point, false, button);
            out_activation = hit;
        } else {
            // 抬起在外部：仅给 pressed 一个 MouseUp 通知（不构建 path，因为 hit
            // 不在 pressed 子树里，沿 path 没有正确语义）。
            MouseEvent mouse{};
            mouse.kind = EventKind::MouseUp;
            mouse.type = EventType::MouseUp;
            mouse.position = point;
            mouse.button = button;
            Event event = mouse;
            EventBase& base = event_base(event);
            base.phase = EventPhase::Target;
            state.pressed->on_event(event);
        }
        apply_pressed_state(state.pressed, false);
    }

    state.pressed = nullptr;
    return true;
}

bool EventDispatcher::dispatch_pointer_wheel(Widget* root, PointF point, float delta) {
    if (root == nullptr || delta == 0.0F) {
        return false;
    }

    std::vector<Widget*> path;
    build_path_recursive(*root, point, path);
    if (path.empty()) {
        return false;
    }

    MouseEvent mouse{};
    mouse.kind = EventKind::MouseWheel;
    mouse.position = point;
    mouse.wheel_delta = delta;
    Event event = mouse;
    return dispatch_event_along_path(path, event);
}

bool EventDispatcher::dispatch_pointer_leave(EventState& state) {
    if (state.hovered == nullptr) {
        return false;
    }

    apply_hover_state(state.hovered, PointF{}, false);
    state.hovered = nullptr;
    return true;
}

void EventDispatcher::collect_focusable(Widget& root, std::vector<Widget*>& out) {
    if (root.accepts_focus()) {
        out.push_back(&root);
    }
    for (const auto& child : root.children()) {
        if (auto* w = dynamic_cast<Widget*>(child.get())) {
            collect_focusable(*w, out);
        }
    }
}

bool EventDispatcher::advance_focus(EventState& state, Widget* root, bool reverse) {
    if (root == nullptr) {
        return false;
    }
    std::vector<Widget*> focusable;
    collect_focusable(*root, focusable);
    if (focusable.empty()) {
        return false;
    }

    std::size_t next_idx = 0;
    auto it = std::find(focusable.begin(), focusable.end(), state.focused);
    if (it == focusable.end()) {
        // 当前没 focused 或 focused 不在树里：从头/尾起步
        next_idx = reverse ? focusable.size() - 1U : 0U;
    } else {
        const std::size_t cur = static_cast<std::size_t>(it - focusable.begin());
        if (reverse) {
            next_idx = (cur + focusable.size() - 1U) % focusable.size();
        } else {
            next_idx = (cur + 1U) % focusable.size();
        }
    }
    return set_focus(state, focusable[next_idx]);
}

bool EventDispatcher::set_focus(EventState& state, Widget* widget) {
    if (state.focused == widget) {
        return false;
    }

    if (state.focused != nullptr) {
        state.focused->set_focused(false);
    }

    state.focused = widget;

    if (state.focused != nullptr) {
        state.focused->set_focused(true);
    }

    return true;
}

bool EventDispatcher::dispatch_key_down(EventState& state, std::int32_t key_code, bool shift,
                                        bool ctrl, bool alt) {
    if (state.focused == nullptr) {
        return false;
    }

    KeyEvent key{};
    key.kind = EventKind::KeyDown;
    key.type = EventType::KeyDown;
    key.key_code = key_code;
    key.shift = shift;
    key.ctrl = ctrl;
    key.alt = alt;
    Event event = key;

    std::vector<Widget*> path;
    build_focus_path(state.focused, path);
    return dispatch_event_along_path(path, event);
}

bool EventDispatcher::dispatch_text_input(EventState& state, char ch) {
    return dispatch_text_input(state,
        static_cast<char32_t>(static_cast<unsigned char>(ch)));
}

bool EventDispatcher::dispatch_text_input(EventState& state, char32_t code_point) {
    if (state.focused == nullptr) {
        return false;
    }

    TextInputEvent text{};
    text.code_point = code_point;
    Event event = text;

    std::vector<Widget*> path;
    build_focus_path(state.focused, path);
    return dispatch_event_along_path(path, event);
}

void EventDispatcher::invalidate(EventState& state) {
    state.hovered = nullptr;
    state.pressed = nullptr;
    state.focused = nullptr;
}

namespace {

// 注意: target 可能是悬空指针 (widget 已析构), 这里仅做指针 == 比较, 不解引用
// target, 所以不会触发 UAF。比较 raw pointer 的相等性是合法的, 即使一方已 dangling。
bool widget_in_tree(Widget* target, Widget& root) {
    if (target == nullptr) return false;
    if (&root == target) return true;
    for (const auto& child : root.children()) {
        if (auto* w = dynamic_cast<Widget*>(child.get())) {
            if (widget_in_tree(target, *w)) return true;
        }
    }
    return false;
}

}  // namespace

void EventDispatcher::sanitize(EventState& state, Widget* root) {
    if (root == nullptr) return;
    if (state.hovered != nullptr && !widget_in_tree(state.hovered, *root)) {
        state.hovered = nullptr;
    }
    if (state.pressed != nullptr && !widget_in_tree(state.pressed, *root)) {
        state.pressed = nullptr;
    }
    if (state.focused != nullptr && !widget_in_tree(state.focused, *root)) {
        state.focused = nullptr;
    }
}

} // namespace hui
