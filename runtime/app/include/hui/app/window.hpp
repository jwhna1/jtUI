#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hui/foundation/geometry.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

struct WindowOptions {
    std::string title {"jtUI"};
    SizeF size {1280.0F, 720.0F};
    bool resizable {true};
    bool frameless {false};
};

class Window {
public:
    explicit Window(WindowOptions options);

    [[nodiscard]] const WindowOptions& options() const noexcept;
    [[nodiscard]] const std::string& title() const noexcept;
    [[nodiscard]] bool visible() const noexcept;
    [[nodiscard]] Widget* content() const noexcept;

    void set_title(std::string title);
    void set_visible(bool visible) noexcept;
    void set_content(std::unique_ptr<Widget> root);

    // v1.20.2 (2026-05-14): 把已经从 content_ 里换出来的旧树真正析构。
    // set_content 不再立刻 delete 旧树（避免事件回调中"自杀"——例如 sidebar
    // 点击 → user lambda 调 rebuild → set_content 把旧 sidebar 销毁，但
    // Signal::emit 的栈还在旧 sidebar 内部访问 this/slots_ → use-after-free）。
    // 改成挂在 pending_destroy_ 上，由消息循环每次 DispatchMessage 后调本函数
    // 真正析构，保证所有事件 unwind 完成后才碰旧内存。
    void drain_pending_destroy() noexcept;

    [[nodiscard]] class Application* application() const noexcept { return application_; }

    // window 客户区尺寸变化(WM_SIZE / 最大化 / 拖动 resize handle)时 emit。
    // host 监听这个 signal 重新 build content tree(传新 size),让所有 widget
    // 按新窗口大小排版。Linux 没 GUI loop,这个 signal 不会触发。
    [[nodiscard]] Signal<SizeF>& on_resized() noexcept { return resized_; }

private:
    friend class Application;
    friend struct WindowRuntimeAccess;

    class Application* application_ {nullptr};
    WindowOptions options_;
    std::unique_ptr<Widget> content_ {};
    // 推迟析构的旧 content 队列。set_content 把旧树挪进来，消息循环 drain
    // 时真正 delete。见 drain_pending_destroy 注释。
    std::vector<std::unique_ptr<Widget>> pending_destroy_ {};
    bool visible_ {true};
    void* native_handle_ {nullptr};
    void* back_buffer_dc_ {nullptr};
    void* back_buffer_bitmap_ {nullptr};
    void* back_buffer_old_bitmap_ {nullptr};
    int back_buffer_width_ {0};
    int back_buffer_height_ {0};
    float dpi_scale_ {1.0F};
    float arranged_width_ {0.0F};
    float arranged_height_ {0.0F};
    bool animation_timer_active_ {false};
    bool deferred_repaint_pending_ {false};
    Widget* hovered_widget_ {nullptr};
    Widget* pressed_widget_ {nullptr};
    Widget* focused_widget_ {nullptr};
    // 跨帧复用的 PaintContext —— 每帧 clear() 保留 capacity，无分配。
    PaintContext paint_context_ {};
    // QueryPerformanceCounter 上次 tick 时间戳，用来算真实 delta（替代硬编码 1/60）
    std::int64_t last_tick_qpc_ {0};
    Signal<SizeF> resized_ {};
};

}  // namespace hui
