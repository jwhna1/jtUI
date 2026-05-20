#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "hui/app/window.hpp"
#include "hui/foundation/geometry.hpp"

namespace hui {

// 全局键盘快捷键。WM_KEYDOWN 进入 EventDispatcher::dispatch_key_down 之前先
// match application 的 shortcut 表;match 上则执行 callback,事件不再传给 widget。
// 典型场景:Ctrl+B 切侧栏、Ctrl+, 打开设置、Esc 关弹窗。
struct Shortcut {
    std::int32_t key_code{0};
    bool ctrl{false};
    bool shift{false};
    bool alt{false};
    std::function<void()> callback;
};

using ShortcutId = std::size_t;

// v1.4 (2026-04-28): 模态层 (ModalLayer)。
//
// 桌面端 dialog / flyout / 右键菜单都需要两个共用行为: 按 ESC 关闭 + 点弹层
// 外侧关闭。jtui v1.3 之前没有框架级支持, 业务层只能塞 hack — 例如 AboutDialog
// 完全无法响应 ESC, SidebarFlyoutMenu 必须再点一次入口按钮 toggle。把这两件
// 事提到 Application 层做成栈式注册 + 自动派发, 业务只填三段回调。
//
// 设计点:
//   1) 栈式优先级: ESC 优先派发给最后 push 的 layer (栈顶), 模拟"先关最近打开
//      的弹窗"的桌面肌肉记忆。
//   2) on_escape 返回 bool: true 表示 layer 消费 ESC 事件 (停止后续派发, 也不
//      传给 focused widget); false 表示 layer 决定不消费 (例如某 layer 状态
//      不允许此时关闭), 继续走下一层。
//   3) 点外侧 (outside_press): 业务自己提供 contains_inside(point) 判断哪些
//      point 算 inside (典型: 弹层卡片自身的 RectF + anchor 入口按钮的 RectF
//      也要算 inside, 否则点入口会先触发 outside_press 关弹层, 紧接着按钮 click
//      又把弹层 toggle 回来 — 闪烁 bug)。contains_inside 为空表示 layer 不参与
//      outside-press 检查 (例如全屏遮罩对话框, 用 bounds 检查卡片矩形)。
//   4) 业务接入流程: show 时 push_modal_layer 拿到 id, hide 时 pop_modal_layer(id);
//      避免 layer 跨次显示串期。
struct ModalLayer {
    // 按 ESC 时调; 返回 true 表示消费, 不再走后续 layer 也不传给 focused widget。
    std::function<bool()> on_escape{};
    // 判断指定 point 是否算 inside (本 layer 自己的 hot region, 包括弹层主体 +
    // 关联入口)。空 (未设置) 表示不参与 outside-press 检测。
    std::function<bool(PointF)> contains_inside{};
    // outside-press 触发时调 (典型: layer.set_visible(false))。
    std::function<void()> on_outside_press{};
};

using ModalLayerId = std::size_t;
using ShortcutId = std::size_t;

class Application {
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    Window& create_window(WindowOptions options = {});
    [[nodiscard]] const std::vector<std::unique_ptr<Window>>& windows() const noexcept;
    int run();

    // 注册 / 移除 / 匹配快捷键。register_shortcut 返回 id 用于后续 remove。
    // try_invoke_shortcut 由平台层在 WM_KEYDOWN 调用,匹配则触发 callback 返回 true。
    ShortcutId register_shortcut(Shortcut shortcut);
    void remove_shortcut(ShortcutId id);
    bool try_invoke_shortcut(std::int32_t key_code, bool ctrl, bool shift, bool alt) const;
    [[nodiscard]] std::size_t shortcut_count() const noexcept { return shortcuts_.size(); }

    // v1.4 (2026-04-28): ModalLayer 注册 / 派发。
    // push_modal_layer 返回 id, 业务在 hide 时调 pop_modal_layer 移除。
    // try_dispatch_modal_escape 由平台层在 WM_KEYDOWN ESC 时优先调, 命中即返回
    // true 跳过 widget 派发。
    // try_dispatch_modal_outside_press 由平台层在 WM_LBUTTONDOWN 之前调, 把
    // hit-test 拿到的逻辑坐标 point 喂进来, 命中任意 layer 的 outside (即 layer
    // 有 contains_inside 但 point 不 inside) 就触发对应 on_outside_press 并返回
    // true, 让平台层"消费"该次点击 (不再走 widget dispatch — 模态语义)。
    ModalLayerId push_modal_layer(ModalLayer layer);
    void pop_modal_layer(ModalLayerId id);
    bool try_dispatch_modal_escape();
    bool try_dispatch_modal_outside_press(PointF point);
    [[nodiscard]] std::size_t modal_layer_count() const noexcept { return modal_layers_.size(); }

    // 把 callback 排到 message pump 的下一个 iteration 执行(典型场景:在事件
    // dispatch / widget tick 内部需要"销毁旧 widget tree 重建新树"时,直接调
    // 会 UAF — dispatch_event_along_path 的 bubble phase 还在引用 path 上的
    // widget,tick_widget_tree 在 widget.tick 返回后还要访问 widget.children())。
    // 用 post(rebuild_fn) 把 rebuild 延后到所有当前栈帧退出之后。
    //
    // Win32:用 PostMessageW(custom WM_APP message),WndProc 在 message 到来时
    // drain pending callbacks。Linux:Application::run() 直接 return 0,post
    // 立即同步调 callback(因为没 GUI loop,不存在"延后到下一帧"语义)。
    //
    // 线程安全:可从任意线程(典型:HTTP worker)调。内部 mutex + 平台 wakeup
    // (PostMessageW)让 UI 线程在下次 message pump iteration drain。
    void post(std::function<void()> callback);

    // 平台层调用:把 pending callbacks 全部 drain 出来执行。WndProc 在收到自定
    // 义 WM_APP 消息时调本函数。callback 内部如果再 post,新 callback 进入 pending,
    // 不在本次 drain 内执行。
    void drain_pending_callbacks();

    // run_async - 在内部托管的 worker thread 上跑 work,完成后用 post() 把
    // completion 拿回 UI 线程跑。比裸 std::thread().detach() 安全:
    //   1. Application 析构时会等所有 in-flight worker 退出(避免 worker 在
    //      Application 死后访问 *this → UAF);
    //   2. completion 通过 post 走 message pump,不会跨线程触 UI。
    // 用法:
    //   app.run_async([url, account]() -> Result {
    //       return WebGatewayClient(url).LoginDesktop(account, password, ...);
    //   }, [self](Result r) {
    //       // 主线程:更新 ViewModel Property
    //       self->apply_login_result(r);
    //   });
    // 如果 work 抛异常,异常被 swallow,completion 不会被调用(避免乱传抛)。
    template <typename Work, typename Completion>
    void run_async(Work work, Completion completion);

    // v1.10 (2026-04-29): 测量一段 UTF-8 文本在指定 font_size / bold 下的视觉宽度
    // (logical px, 跟 RectF 同坐标系)。给 TextInput 等富文本编辑 widget 算精确
    // 光标位置 / 鼠标 hit-test。复用全局 DirectTextSession + IDWriteTextLayout::
    // GetMetrics, 与 PaintContext draw_text 字宽完全一致。失败 (DWrite 未初始化)
    // 返回 0。仅 UI thread 调用 (内部 thread_local DWriteFactory)。
    [[nodiscard]] static float measure_text_width(const std::string& utf8,
                                                    float font_size, bool bold);

    // v1.22 (2026-05-04): 文件拖放回调。
    //
    // 业务侧 (例如 ChatPage 输入卡片) 注册一份 callback, 用户从外部 (Explorer)
    // 拖文件释放到窗口客户区时, 平台层在 WM_DROPFILES 收到后调 callback,
    // 把 drop 点 (window 客户区 logical px, 与 widget RectF 同坐标) + 文件
    // 绝对路径列表 (UTF-8) 一起送过来。回调内部自己用 widget.frame().contains
    // 判定命中哪个目标 widget (例如 ChatPage 输入卡片), 不命中可直接忽略。
    //
    // 当前 v1 单实例 callback (set 覆盖); 业务多目标场景再扩 multi-listener。
    // 平台层在 create_window 时会自动 DragAcceptFiles + 配 UIPI 消息过滤, 业务
    // 不需要管 Win32 细节, 只设 callback 即可。
    using FilesDroppedCallback =
        std::function<void(PointF, std::vector<std::string>)>;
    void set_files_dropped_callback(FilesDroppedCallback cb) {
        files_dropped_callback_ = std::move(cb);
    }
    // 平台层 (application.cpp WM_DROPFILES 分支) 调用入口。
    void invoke_files_dropped(PointF point, std::vector<std::string> paths) {
        if (files_dropped_callback_) {
            files_dropped_callback_(point, std::move(paths));
        }
    }

    // v1.11 (2026-04-29): 测量指定 font_size / bold 下 DirectWrite 真实单行 line
    // height (logical px)。给 TextInput 等 widget 算 selection / cursor 的 y 偏
    // 移用 — 旧实现 hardcode 22 与真实 17-18 不一致, 加上 PARAGRAPH_ALIGNMENT_CENTER
    // 把 paragraph 居中到 layout box, 累积错位让选区高亮 / 光标偏移文字。
    // 失败 fallback font_size * 1.4F (经验值)。
    [[nodiscard]] static float measure_line_height(float font_size, bool bold);

private:
    // run_async 用的非模板内部:管 worker thread 计数,提供 enqueue + wait 接口。
    void register_async_worker();
    void release_async_worker();

private:
    struct ShortcutEntry {
        ShortcutId id;
        Shortcut shortcut;
    };
    struct ModalLayerEntry {
        ModalLayerId id;
        ModalLayer layer;
    };
    std::vector<std::unique_ptr<Window>> windows_ {};
    std::vector<ShortcutEntry> shortcuts_ {};
    ShortcutId next_shortcut_id_ {1};
    // modal_layers_ 顺序 = 推入顺序; 末尾即栈顶, ESC 派发倒序遍历, outside-press
    // 全量遍历 (每层都可能触发, 例如多层弹窗叠加时点外应同时关掉所有暴露在外的层)。
    std::vector<ModalLayerEntry> modal_layers_ {};
    ModalLayerId next_modal_layer_id_ {1};
    // pending_callbacks_ 跨 worker thread 写、UI thread 读 → 必须 mutex。
    // 之前没锁是 latent bug,worker 完成 push_back 与 UI drain 的 swap 会
    // 同时改 vector → 数据 corruption / crash。
    mutable std::mutex pending_mu_ {};
    std::vector<std::function<void()>> pending_callbacks_ {};
    // run_async in-flight tracking:析构时等到 0 才返回。
    std::atomic<int> in_flight_workers_ {0};
    std::mutex worker_done_mu_ {};
    std::condition_variable worker_done_cv_ {};

    // v1.22 (2026-05-04): 文件拖放回调 (单实例)。set 覆盖, 不同业务方需要协调。
    FilesDroppedCallback files_dropped_callback_ {};
};

template <typename Work, typename Completion>
inline void Application::run_async(Work work, Completion completion) {
    register_async_worker();
    // 不能在 lambda 里把 *this(Application 引用)拷贝(它 noncopyable),所以传 raw
    // pointer。Application 析构会等到 in_flight=0,所以 worker 内部 *this 一定有效。
    Application* self = this;
    std::thread([self, work = std::move(work),
                 completion = std::move(completion)]() mutable {
        try {
            auto result = work();
            // 把 completion + 结果包成 closure post 回 UI thread。result 必须可拷贝
            // 或可移动到 lambda 捕获(C++20 lambda init capture)。
            self->post([completion = std::move(completion),
                         result = std::move(result)]() mutable {
                completion(std::move(result));
            });
        } catch (...) {
            // work 抛异常:swallow,不调 completion。生产代码 work 应自己 try/catch
            // 把异常映射成 result.ok=false。
        }
        self->release_async_worker();
    }).detach();
}

}  // namespace hui
