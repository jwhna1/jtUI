#pragma once

#include <cstdint>
#include <variant>

#include "hui/foundation/geometry.hpp"

// jtUI event 数据定义
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>），创建于 2026-04-24
//
// 这份头放在 hui::object 层（而不是 hui::event），是因为 Widget::on_event(Event&)
// 需要它，而 hui::event 又依赖 hui::object（EventDispatcher 摸 Widget），形成循环。
// 把纯数据类型下沉一层即可。EventDispatcher 仍在 hui::event。
// `runtime/event/include/hui/event/events.hpp` 保留为转发头以兼容旧 include。

namespace hui {

enum class EventPhase : std::uint8_t {
    Capture,
    Target,
    Bubble,
};

enum class EventKind : std::uint8_t {
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseDown,
    MouseUp,
    MouseClick,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
    FocusIn,
    FocusOut,
};

enum class MouseButton : std::uint8_t {
    Left,
    Right,
    Middle,
};

// 旧版本兼容别名（已存在的 EventType 枚举）
enum class EventType : std::uint8_t {
    MouseMove = static_cast<std::uint8_t>(EventKind::MouseMove),
    MouseDown = static_cast<std::uint8_t>(EventKind::MouseDown),
    MouseUp = static_cast<std::uint8_t>(EventKind::MouseUp),
    KeyDown = static_cast<std::uint8_t>(EventKind::KeyDown),
    KeyUp = static_cast<std::uint8_t>(EventKind::KeyUp),
};

// 所有事件的公共部分。phase + handled 是分发链共用语义。
struct EventBase {
    EventPhase phase{EventPhase::Target};
    bool handled{false};

    void accept() noexcept {
        handled = true;
    }
};

struct MouseEvent : EventBase {
    EventKind kind{EventKind::MouseMove};
    EventType type{EventType::MouseMove};
    PointF position{};
    MouseButton button{MouseButton::Left};
    bool pressed{false};
    float wheel_delta{0.0F};
};

struct KeyEvent : EventBase {
    EventKind kind{EventKind::KeyDown};
    EventType type{EventType::KeyDown};
    std::int32_t key_code{0};
    bool repeat{false};
    // 修饰键状态。Shift 让 TextInput 等组件做"扩展选区"等行为；ctrl / alt 留给后续
    // gesture / shortcut / IME。WM_KEYDOWN 路径在 application.cpp 抓 GetKeyState 填进来。
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
};

struct TextInputEvent : EventBase {
    char32_t code_point{0};
};

struct FocusEvent : EventBase {
    EventKind kind{EventKind::FocusIn};
};

using Event = std::variant<MouseEvent, KeyEvent, TextInputEvent, FocusEvent>;

// 任何 Event 子类型 → 公共 EventBase 引用。dispatch 阶段读写 phase/handled 用。
[[nodiscard]] inline EventBase& event_base(Event& ev) noexcept {
    return std::visit([](auto& e) -> EventBase& { return e; }, ev);
}

[[nodiscard]] inline const EventBase& event_base(const Event& ev) noexcept {
    return std::visit([](const auto& e) -> const EventBase& { return e; }, ev);
}

}  // namespace hui
