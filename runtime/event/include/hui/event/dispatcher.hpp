#pragma once

#include <cstdint>
#include <vector>

#include "hui/event/events.hpp"
#include "hui/foundation/geometry.hpp"

// jtUI 事件分发器
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>），创建于 2026-04-24
//
// 职责：
//   1) 在 widget 树上做命中测试，返回命中目标或 root->target 路径
//   2) 维护每窗口的 hover / press / focus 状态机
//   3) Press capture：按下期间 pointer_move 送给 pressed 而不是 hovered
//   4) Capture → Target → Bubble 三阶段事件分发（dispatch_event_along_path）
//   5) 键盘 / 字符沿 focused widget 的祖先链做三阶段分发
//
// Widget::on_event(Event&) 是统一入口，默认实现把 Target phase 的事件路由到旧的
// on_xxx 虚函数（on_click / on_press_changed / on_mouse_move / on_text_input /
// on_key_down），现有 widget 不改一行就能享受新分发链。想拦截事件的祖先 widget
// 自己 override on_event，按 phase 判定后调 base.handled = true 即可。

namespace hui {

class Widget;

struct EventState {
    Widget* hovered{nullptr};
    Widget* pressed{nullptr};
    Widget* focused{nullptr};
};

class EventDispatcher {
  public:
    // 从 root 递归命中最深叶子（dynamic_cast<Widget>），visible + frame 均命中才算
    [[nodiscard]] static Widget* hit_test(Widget& root, PointF point);

    // root 到命中叶子的 widget 链（含两端），用于 capture/bubble 分发
    static void build_path(Widget& root, PointF point, std::vector<Widget*>& out);

    // 沿 focused widget 的 parent 链回到 root，得到 root → focused 的链。
    // 用于键盘 / 字符等没有指针位置的事件。
    static void build_focus_path(Widget* focused, std::vector<Widget*>& out);

    // 沿 path 走 Capture（root → target.parent）→ Target（target）→ Bubble
    // （target.parent → root）三阶段。任意阶段 widget.on_event 把 base.handled
    // 置 true 即终止后续分发。返回事件最终是否被消费。
    static bool dispatch_event_along_path(const std::vector<Widget*>& path, Event& event);

    // 指针移动。返回是否需要重绘（hover/pressed 状态是否变化）
    //   - 如果 state.pressed 非空：move 送给 pressed，同步维护 hovered
    //   - 否则：按命中目标切换 hovered，送 mouse_move
    static bool dispatch_pointer_move(Widget* root, PointF point, EventState& state);

    // 指针按下：命中目标被置为 pressed，返回命中的 widget。
    // v1.3 (2026-04-28): button 参数标识 L/R/M, 默认 Left 保持旧调用兼容。
    // 沿 path 分发的 MouseDown 事件 .button 字段会被填上, widget 通过 on_event
    // 拦 MouseDown 时能区分左/右键 (例如 MarkdownText 右键弹复制 / 业务层右键菜单)。
    static Widget* dispatch_pointer_down(Widget* root, PointF point, EventState& state,
                                         MouseButton button = MouseButton::Left);

    // 指针抬起：命中目标若和 pressed 相同则触发 click；清 pressed；同步 hover
    // out_activation 填入当该次抬起需要触发 click 的 widget（调用方负责处理
    // window_action 优先级，再走 dispatch_event_along_path 触发 MouseClick 链）
    // v1.3 (2026-04-28): button 默认 Left, 与 down 对称。
    static bool dispatch_pointer_up(Widget* root, PointF point, EventState& state,
                                    Widget*& out_activation,
                                    MouseButton button = MouseButton::Left);

    // 指针离开窗口：清 hover（不动 pressed，pressed 由 release capture 决定）
    static bool dispatch_pointer_leave(EventState& state);

    // 鼠标滚轮：沿命中链走三阶段。delta 正向上、负向下（对齐 Windows WHEEL_DELTA / 120）。
    // 返回 true 表示链上有 widget 消费（典型：ScrollView 在 Capture/Target phase 改 offset）。
    static bool dispatch_pointer_wheel(Widget* root, PointF point, float delta);

    // 焦点切换，返回 true 表示变化了
    static bool set_focus(EventState& state, Widget* widget);

    // 收集 root 子树里所有 accepts_focus 的 widget，DFS 顺序（与 widget tree 自然
    // 顺序一致，等同 web tabindex 默认顺序）。
    static void collect_focusable(Widget& root, std::vector<Widget*>& out);

    // Tab 焦点循环：把 state.focused 切到下一个（reverse=true 切上一个）focusable widget。
    // 当前没 focused 时从头/尾起步；只有一个 focusable 时无操作。返回 true 表示焦点变化。
    static bool advance_focus(EventState& state, Widget* root, bool reverse);

    // 键盘 / 字符分发到 focused，沿 focus 祖先链走三阶段。返回 true 表示链上有人消费。
    // shift / ctrl / alt 由 host 在 dispatch 前抓平台 modifier 状态填入，TextInput 等组件
    // 在 on_event 里读 KeyEvent.shift 决定是否扩选区等行为。
    static bool dispatch_key_down(EventState& state, std::int32_t key_code,
                                  bool shift = false, bool ctrl = false, bool alt = false);
    static bool dispatch_text_input(EventState& state, char ch);
    // v1.2 (2026-04-28): 宽字符派发. WM_CHAR 在 Unicode 窗口下接收 UTF-16 code
    // unit (BMP 内中文/日文/韩文都是单 unit), 用这个版本派发给 focused widget.
    // 旧 char 重载内部直接 promote 到 char32_t 走同一条路径, 不破坏现有 ASCII 调用.
    static bool dispatch_text_input(EventState& state, char32_t code_point);

    // 从 widget 树里移除引用（当 content 替换或 widget 销毁时用）
    static void invalidate(EventState& state);

    // v1.2 (2026-04-28): sanitize EventState 防御 widget 子树替换的悬空指针。
    //
    // 背景: Window::set_content 替换整树时显式清了 hovered/pressed/focused, 但
    // 内部容器 (ScrollView::set_content / Panel::clear_children / FlexBox 重建)
    // 替换子树时不会通知 Window, EventState 仍持有被销毁 widget 的 raw pointer。
    // 下次鼠标 / 键盘事件调 apply_hover_state(state.hovered, ...) 解引用就 UAF。
    //
    // 高频场景 (chat streaming / 媒体宿主时间线 60-240fps 重建) 必崩。修法: 事件
    // 分发前先走一遍 widget_in_tree(state.X, *root), 不在树里直接置 nullptr。
    // 单次调用 O(n), n = widget 树大小; 200-1000 widgets 无感, 比加 widget→window
    // 析构反向链路 (要改 Widget 基类 + 所有 children 容器) 侵入小得多。
    //
    // 后续 jtUI v0.3 候选: Widget 析构时通过 parent chain 找到 owner Window 主动
    // 调 invalidate, 取代每次事件 sanitize 的 O(n) 遍历。
    static void sanitize(EventState& state, Widget* root);
};

} // namespace hui
