#pragma once

#include <cstdint>

#include "hui/foundation/geometry.hpp"
#include "hui/object/element.hpp"
#include "hui/object/event.hpp"

namespace hui {

class PaintContext;

enum class WindowAction : std::uint8_t {
    None,
    DragMove,
    Minimize,
    ToggleMaximize,
    Close,
};

// v1.21 (2026-05-04): hover 时显示什么形态的鼠标光标。
//
// Default 维持 Win32 类默认 (箭头)。IBeam 文本输入 / 文本选区的"工字光标"。
// Hand 给将来的链接 / pill 用 (用户暗示需求时再切, 当前不强加)。
//
// host application 在 WM_SETCURSOR / WM_MOUSEMOVE 里读 hover widget.cursor_kind()
// 调 SetCursor, 不需要 widget 自己接 Win32 消息。
enum class CursorKind : std::uint8_t {
    Default,
    IBeam,
    Hand,
};

class Widget : public Element {
  public:
    virtual ~Widget() override = default;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Widget";
    }

    // v0.3 (2026-05-15): release_child 的类型化 public 包装。Node::release_child
    // 是 protected 且返回 unique_ptr<Node>；业务侧拿到 Widget* 时调本 API，
    // 用模板参数 T 拿回 std::unique_ptr<T>。调用方负责保证 target 真的是
    // T 的实例（一般是 release(raw_ptr_of_type_T) 直接用调用方持有的 raw ptr）。
    //
    // 用法（跨 rebuild 保活 VideoPlayer）：
    //     // build_root 时记录 raw ptr：
    //     persist.video_raw = persist.video.get();
    //     root->append_child(std::move(persist.video));
    //
    //     // 下一次 rebuild 前从旧 root 释放：
    //     persist.video = old_root->release_child<jtui::VideoPlayer>(persist.video_raw);
    //
    // 见 examples/jtui_studio/main.cpp 的 PersistentWidgets 用法。
    template <typename T>
    [[nodiscard]] std::unique_ptr<T> release_child(const T* target) noexcept {
        auto generic = Node::release_child(target);
        if (!generic) return nullptr;
        return std::unique_ptr<T>(static_cast<T*>(generic.release()));
    }

    [[nodiscard]] bool enabled() const noexcept {
        return enabled_;
    }

    void set_enabled(bool enabled) noexcept {
        if (enabled_ == enabled) {
            return;
        }

        enabled_ = enabled;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool focused() const noexcept {
        return focused_;
    }

    void set_focused(bool focused) noexcept {
        if (focused_ == focused) {
            return;
        }

        focused_ = focused;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool hovered() const noexcept {
        return hovered_;
    }

    void set_hovered(bool hovered) noexcept {
        if (hovered_ == hovered) {
            return;
        }

        hovered_ = hovered;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool pressed() const noexcept {
        return pressed_;
    }

    // paint() 是否读 pressed()。默认 false：大多数组件（Switch / Tabs / Checkbox
    // / Gauge / MetricCard / TextInput…）按下时视觉不变，按下去多走一遍 paint
    // 纯属浪费，点击响应会慢一拍。Button 之类会覆写为 true。
    [[nodiscard]] virtual bool paint_reacts_to_pressed() const noexcept {
        return false;
    }

    void set_pressed(bool pressed) noexcept {
        if (pressed_ == pressed) {
            return;
        }

        pressed_ = pressed;
        if (paint_reacts_to_pressed()) {
            mark_dirty(DirtyFlags::Paint);
        }
    }

    [[nodiscard]] virtual bool hit_test(PointF point) const noexcept {
        return visible() && frame().contains(point);
    }

    // 容器是否对子树做"绘制裁切"。true 时 paint_widget_tree 把 clip 与自己 frame
    // 取交集再传给 children,让超出自己边界的 child paint 命令被跳过(整个 child
    // 不在 viewport 内时直接 skip;部分超出时 child 仍 paint 整体,边缘像素可能
    // 溢出 — v1 接受,v2 加 PaintContext::push_clip 才严格)。ScrollView 用。
    [[nodiscard]] virtual bool clips_children() const noexcept {
        return false;
    }

    // v1.20 (2026-05-14): widget 自己 paint() 内的命令是否被严格 clip 到自己 frame。
    // 默认 false（保持向后兼容：旧 widget paint 命令可以略微越界做"装饰溢出"，
    // 比如 hover 阴影、focus ring 这种）。
    // true 时 runtime 在 widget.paint(context) 前后自动 push_clip(intersect(parent,
    // self.frame)) / pop_clip，类似 CSS overflow:hidden 对自身 painted content。
    // 典型用例：image bg cover 模式 —— 业务把图等比放大到溢出 widget 自己 frame，
    // runtime 自动截掉超出部分，业务零 push_clip 心智。比 clips_children() 更
    // 适合"自绘大背景的 widget"，因为 clips_children 只截 children paint，不截
    // widget 自己 paint() 内的命令。
    //
    // **重要限制（v1.4 文档化）**：runtime 自动 push 的是 `push_clip(self.frame)`
    // 矩形裁剪，**不裁圆角扇区**。如果 widget 同时设了 corner_radius 且 paint
    // 命令在自己 frame 内但越过圆角扇区（典型：靠 4 个角的小三角 / 多边形顶点），
    // 圆角外的几何仍会画到屏幕。业务侧解法：要么收紧顶点距 corner ≥ radius，
    // 要么缩小 corner_radius 让圆角扇区影响范围更小。
    // 实战案例：jtui_cinema 的 ThumbnailArt 几何插画放角落小三角时溢出过圆角，
    // 修法是把卡片 radius_lg(20) → radius_sm(10) + 顶点内缩。详见 [[jtui-clips-self-rounded-gap]]。
    [[nodiscard]] virtual bool clips_self() const noexcept {
        return false;
    }

    virtual void on_mouse_move(PointF) {}

    virtual void on_mouse_leave() {}

    virtual void on_press_changed(PointF, bool) {}

    virtual void on_click(PointF) {}

    [[nodiscard]] virtual bool accepts_focus() const noexcept {
        return false;
    }

    virtual bool on_text_input(char) {
        return false;
    }

    virtual bool on_key_down(std::int32_t) {
        return false;
    }

    // 统一事件入口。EventDispatcher 在 capture/target/bubble 三个 phase 调到这里。
    // 默认实现：
    //   - capture / bubble 两个 phase 不做任何副作用、不消费事件 —— 想拦截事件
    //     的祖先 widget 自己 override 后判断 phase 并 set base.handled = true；
    //   - target phase 把事件按 EventKind 路由到旧的 on_xxx 虚函数，让现有 15+
    //     widget 不改一行就能享受新 dispatch 链；
    //   - on_text_input / on_key_down 的返回值表示是否消费，会被同步到 ev.handled。
    // override 后想保留默认 fan-out 行为，调 `Widget::on_event(ev)` 即可。
    virtual bool on_event(Event& ev) {
        EventBase& base = event_base(ev);
        if (base.phase != EventPhase::Target) {
            return false;
        }

        if (auto* mouse = std::get_if<MouseEvent>(&ev)) {
            switch (mouse->kind) {
            case EventKind::MouseMove:
                on_mouse_move(mouse->position);
                return false;
            case EventKind::MouseLeave:
                on_mouse_leave();
                return false;
            case EventKind::MouseDown:
                on_press_changed(mouse->position, true);
                return false;
            case EventKind::MouseUp:
                on_press_changed(mouse->position, false);
                return false;
            case EventKind::MouseClick:
                on_click(mouse->position);
                return false;
            default:
                return false;
            }
        }
        if (auto* key = std::get_if<KeyEvent>(&ev)) {
            if (key->kind == EventKind::KeyDown) {
                const bool consumed = on_key_down(key->key_code);
                if (consumed) {
                    base.handled = true;
                }
                return consumed;
            }
            return false;
        }
        if (auto* text = std::get_if<TextInputEvent>(&ev)) {
            // 旧 on_text_input 接口是 char。Code point 超出 ASCII 暂不分发到旧虚函数，
            // 等后续 IME / wide-char 通道补上来再扩。
            if (text->code_point <= 0xFFU) {
                const bool consumed = on_text_input(static_cast<char>(text->code_point));
                if (consumed) {
                    base.handled = true;
                }
                return consumed;
            }
            return false;
        }
        return false;
    }

    [[nodiscard]] virtual WindowAction window_action_at(PointF) const noexcept {
        return WindowAction::None;
    }

    // v1.21 (2026-05-04): hover 时鼠标光标形态。Application 在 WM_SETCURSOR /
    // WM_MOUSEMOVE 里查 hover widget 的本接口决定 SetCursor。默认 Default 走
    // Win32 类默认箭头, TextInput / 选区类 widget override 成 IBeam。
    [[nodiscard]] virtual CursorKind cursor_kind() const noexcept {
        return CursorKind::Default;
    }

    // ─── z-order 约定（v1.4 文档化） ───────────────────────────────────
    //
    // jtUI 默认 z-order = append 顺序，**后 append 的 widget 在视觉上更靠前 +
    // hit_test 优先收到 click**（即"最后挂上的覆盖前面的"）。这是 paint_widget_tree
    // 递归 children 时 append 顺序就是 paint 顺序，hit_test 反向遍历找最先命中。
    //
    // 业务侧加新 widget 时要审视 append 顺序与可视层级：
    //   - 想覆盖其他 widget 的 widget（如 modal scrim / popover）必须**最后 append**
    //   - 透明 / 装饰性 widget（如 grid_decoration）必须**最先 append**
    //   - 可点 button 加在 frame 重叠区域时，要 append 在被覆盖 widget **之后**
    //     否则其他 widget 抢走 hit_test（典型坑：folders_app 的 nav links Text 后
    //     append 拦住前 append 的 About button click）。
    //
    // 没有显式 z-index API，靠 append 顺序约定。如需 z-index 支持，加 backlog。
    virtual void paint(PaintContext&) const {}

    // v1.18 (2026-05-03): 前景层绘制, 在 children paint + clip 之后调用 — 让
    // widget 能画"后画 = 上层"的元素, 不被自己的 children 覆盖。
    // 典型场景: ScrollView 的滚动条画在 viewport 内右侧, children paint 后才
    // 调用 paint_overlay 让滚动条在内容之上。默认 noop, 不需要 overlay 的 widget
    // 不必 override。注意 paint_overlay 在 push_clip(self.frame) 之外调用,
    // 即可以画到 self.frame 范围内任意位置 (滚动条在 frame 内最右侧)。
    virtual void paint_overlay(PaintContext&) const {}

    // v1.19 (2026-05-04): 命中拦截 — 返回 true 表示该 widget 在 point 命中时
    // 不让 hit_test 递归 children, 自己成为 leaf hit target (state.pressed)。
    //
    // 典型场景: ScrollView 滚动条 thumb 区域。旧路径 hit_test 一路下钻到消息
    // 内容 widget, state.pressed = leaf, 后续 MouseMove 全部送 leaf, ScrollView
    // 收不到 press 期间的 MouseMove → 拖动 thumb 不响应。
    //
    // 业务 widget 想"在某区域抢断 children"时 override 即可, 默认 false 不影响
    // 现有 widget 行为。
    [[nodiscard]] virtual bool intercepts_children_at(PointF) const { return false; }

    [[nodiscard]] virtual bool tick(float) {
        return false;
    }

    // measure / arrange 两遍布局。容器（FlexBox 等）重写。
    // 默认实现把 widget 当作叶子：measure 返回当前 frame 的 size 作为首选尺寸，
    // arrange 则将自己的 frame 设为父给定 frame，不递归 children。
    [[nodiscard]] virtual SizeF measure(SizeF /*available*/) const {
        return SizeF{frame().width, frame().height};
    }

    virtual void arrange(RectF frame_in) {
        set_frame(frame_in);
    }

  private:
    bool enabled_{true};
    bool focused_{false};
    bool hovered_{false};
    bool pressed_{false};
};

} // namespace hui
