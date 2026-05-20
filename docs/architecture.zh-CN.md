# jtUI 架构

深入剖析 jtUI 的内部机制。如果你打算编写框架级 widget、为 runtime 做贡献，或者只是想搞清楚为什么某个特定模式（整树 rebuild、`translate_subtree`……）才是规范做法，请阅读本文。

应用层使用请先看 [快速上手](getting-started.zh-CN.md)；公开 API 列表见 [API 参考](api-reference.zh-CN.md)。

[English](architecture.md)

---

## 目录

1. [模块布局与依赖方向](#1-模块布局与依赖方向)
2. [渲染管线](#2-渲染管线)
3. [Dirty 冒泡与局部重绘](#3-dirty-冒泡与局部重绘)
4. [事件分发（capture–target–bubble）](#4-事件分发capturetargetbubble)
5. [Theme token 与解析](#5-theme-token-与解析)
6. [动画 timer 与 `tick()`](#6-动画-timer-与-tick)
7. [整树 rebuild 作为状态模型](#7-整树-rebuild-作为状态模型)
8. [Z-order 与 hit testing](#8-z-order-与-hit-testing)
9. [跨平台模型](#9-跨平台模型)
10. [性能注释](#10-性能注释)
11. [扩展点](#11-扩展点)

---

## 1. 模块布局与依赖方向

jtUI 分为三层，每一层都有自己独立的目录树。上层依赖下层；`runtime/` 中的任何东西都不依赖 `platform/` 或 `widgets/`。

```
                  ┌─────────────────────────────┐
                  │   examples/  (10 demos)     │
                  │   ffi/c_api/ (C ABI shell)  │
                  └──────────────┬──────────────┘
                                 ↓ depends on
                  ┌─────────────────────────────┐
                  │   api/   jtui:: namespace   │
                  │           umbrella header   │
                  └──────────────┬──────────────┘
                                 ↓
        ┌─────────────────┬──────┴──────┬───────────────────┐
        ↓                 ↓             ↓                   ↓
  widgets/basic    widgets/common   widgets/media    platform/win32
   (Panel/Text/     (Button/Tabs/    (VideoPlayer/    (Direct2D 1.1 /
    ScrollView/      Dialog/...)      AudioPlayer/     MF / WASAPI /
    FlexBox/...)                      WaveformView)    codicon font)
        │                 │             │                   │
        └────────┬────────┴─────────────┴───────────────────┘
                 ↓
            ┌─────────────────────────────────────────┐
            │             runtime/                    │
            │  app/        Application · Window       │
            │  render/     PaintContext · DrawCommand │
            │  event/      EventDispatcher · Event    │
            │  layout/     FlexBox · measure/arrange  │
            │  theme/      SemanticColor · elevation  │
            │  property/   Signal<T> · Property<T>    │
            │  object/     Node · Element · Widget    │
            │  foundation/ Color · i18n · log · codi  │
            │  media/      IVideoDecoder · IAudio…    │
            └─────────────────────────────────────────┘
```

`runtime/` 内部自下而上的顺序为：`foundation` → `object` → `property` → `theme` → `layout` → `event` → `render` → `media` → `app`。每个模块导出一个 CMake target（`hui::foundation`、`hui::object` ……），互相之间不存在循环依赖。

`api/include/jtui/jtui.hpp` 是 umbrella 头文件，通过别名将所有公开类型引入 `jtui::` 命名空间。应用代码只需要包含这一个头文件。`widgets/common/include/hui/widgets/common/about_card.hpp`（v1.4 新增）是框架级 widget 的典型示例，它位于 `api/` umbrella 之外，但仍通过 `jtui::install_about_card` 对外暴露。

---

## 2. 渲染管线

jtUI 将"画什么"（`PaintContext`）与"怎么画"（平台后端）分离。正是这种分离让 Linux stub 模式以及未来的 macOS / Skia 后端可以加入，而 widget 代码完全不必改动。

### 2.1 PaintContext → DrawCommand IR

每次 `Widget::paint(PaintContext&)` 调用都会向 `PaintContext` 内部的 `std::vector<DrawCommand>` 追加 `DrawCommand` 记录。`DrawCommand` 是一个带 tag 的联合体：

```cpp
enum class DrawCommandKind : uint8_t {
    FillRect, StrokeRect, FillRoundedRect, StrokeRoundedRect,
    FillEllipse, StrokeEllipse, Line, Bezier, FillPolygon,
    FillGradientRect, FillShadow, FillBackdropBlur,
    Text, DrawTexture, PushClip, PopClip,
};

struct DrawCommand {
    DrawCommandKind kind;
    RectF           bounds;
    Color           color;
    Color           gradient_end_color;
    float           corner_radius;
    float           stroke_thickness;
    float           font_size;
    PointF          end_point;
    PointF          shadow_offset;
    PointF          bezier_c1, bezier_c2;
    float           shadow_blur, shadow_spread;
    PolygonGeom     polygon;          // vector<PointF>
    PixelBuffer     pixel_buffer;
    std::string     text;
    bool            bold;
    TextAlignment   text_alignment;
};
```

widget 从不直接调用 D2D，它们只产出命令。后端负责把命令列表 replay 到平台的 render target 上。

### 2.2 后端 replay（`platform/win32`）

`runtime/app/src/application.cpp`（约第 1438 行）有 `paint_widget_tree(const Widget& widget, PaintContext& context, RectF clip_rect)`，它会：

1. 如果 widget 为 `!visible()`，或其 frame 落在 `clip_rect` 之外，直接 early return。
2. 如果 `clips_self()` 为 true：emit `PushClip(intersect(parent_clip, self.frame))`。
3. 调用 `widget.paint(context)`。
4. 用更新后的 clip 递归处理子节点。
5. 如果 `clips_self()` 为 true：emit `PopClip`。
6. 在 clip 之外调用 `widget.paint_overlay(context)`（让 widget 能在子节点"之上"绘制）。

`paint_widget_tree` 把命令列表填满之后，`replay_paint_commands` 遍历列表，把每个 `DrawCommand` 翻译成对应的 D2D 1.1 / DirectWrite 调用。

Win32 后端使用双缓冲的内存 DC + D2D `ID2D1DCRenderTarget`。像毛玻璃和盒阴影这类大块图元会走 `CLSID_D2D1GaussianBlur` / `CLSID_D2D1Shadow` Effect（在 D2D 1.1+ 上从 render target 查询出 `ID2D1DeviceContext`）。

### 2.3 DPI 感知

所有 `DrawCommand` 的坐标都是**逻辑像素**（与 DPI 无关）。replay 时后端会读取 `window.dpi_scale`，在发起 D2D 调用之前把几何尺寸乘上去。`application.cpp` 中的 `scaled_command` 辅助函数会缩放 bounds、end_point、polygon.points、bezier_c1/c2、shadow_offset、font_size——任何承载物理尺度的字段。

如果你新增了一个携带 `PointF` 字段的 `DrawCommandKind`，**必须同步更新 `scaled_command` 让它乘上 DPI scale**，否则 HiDPI 用户会看到这条命令的几何错位，而其他命令位置正常。（这是 v1.22.1 修复过的真实 bug。）

---

## 3. Dirty 冒泡与局部重绘

jtUI 不会每帧都重绘整个窗口。它为每个 widget 维护 dirty flags，只重绘真正失效的 widget，然后把它们的 frame 求并集，得到尽可能小的 dirty rectangle 交给 D2D。

### 3.1 `DirtyFlags`

```cpp
enum class DirtyFlags : uint32_t {
    None      = 0,
    Structure = 1 << 0,   // 子节点列表发生变化
    Layout    = 1 << 1,   // 尺寸/位置发生变化
    Paint     = 1 << 2,   // 像素发生变化
    Style     = 1 << 3,   // theme token 发生变化
};
```

按位可组合。`set_frame` 会翻转 Layout|Paint。`set_visible` 会翻转 Paint。`theme::Theme::set(...)` 会翻转每个 widget 的 Style。

### 3.2 subtree-dirty 摘要

朴素做法是每次 paint pass 都要遍历整棵树才能找到"所有 dirty widget"——每帧 O(N)，当 N > 1000 时开销很大。jtUI 为每个 widget 维护一份 **subtree-dirty 摘要**，它是该 widget 自身 `dirty_flags_` 与所有子节点摘要的按位 OR。当任何叶子 widget 把自己标记为 dirty 时，对应的位会通过 `propagate_subtree_dirty(parent, flags)` 向上冒泡到根节点。之后重绘只会下降到摘要非零的分支。

```cpp
void Element::mark_dirty(DirtyFlags flags) noexcept {
    if (flags == DirtyFlags::None) return;
    dirty_flags_ |= flags;
    propagate_subtree_dirty(parent(), flags);
}

// 递归：把 flag OR 到父节点的 subtree_dirty_，若父节点已覆盖该 flag
// 则提前停止（避免重复遍历）。
void Element::propagate_subtree_dirty(Node* node, DirtyFlags flags) noexcept {
    while (node != nullptr) {
        Element* e = dynamic_cast<Element*>(node);
        if (e == nullptr) return;
        if ((e->subtree_dirty_ & flags) == flags) return;   // 早停
        e->subtree_dirty_ |= flags;
        node = e->parent();
    }
}
```

这个早停优化让冒泡的成本保持在大约 O(depth)，而不是 O(siblings)。

### 3.3 局部重绘

`invalidate_dirty_window` 会计算所有 dirty widget 的 frame 的 AABB（凭借摘要可以跳过干净的子树），只把这块矩形交给 Win32 失效：

```cpp
RectF dirty_bounds{};
if (dirty_bounds_widget_tree(*content, DirtyFlags::Paint, dirty_bounds)) {
    InvalidateRect(hwnd, &dirty_bounds, FALSE);
}
```

随后的 `WM_PAINT` 只会重绘 dirty rect。对典型的 UI 来说，这意味着 hover / press 过渡只会重绘单个按钮，而不是整个窗口。

### 3.4 `set_frame` 与 `translate_subtree` 的取舍

这是写动画时最需要内化的模式：

- **`set_frame(new_rect)`** 会触发 `mark_dirty(Layout|Paint)`。dirty rect = 新位置。旧位置的像素并没有进入 dirty rect——D2D 不会重绘它们，因此它们会留在屏幕上 → 出现残影。

- **`translate_subtree(dx, dy)`** 会直接修改 `frame_.x/y` 而不会触发 mark dirty。**你必须在某个 frame 覆盖整个受影响区域的容器 widget 上 mark dirty**，这样局部重绘才会把新旧位置一起重画。

`ScrollView` 与 `CarouselAnimator` 都使用这个模式。可参考 `widgets/basic/src/scroll_view.cpp:155`（ScrollView 的滚动回调）以及 `examples/jtui_cinema/carousel_animator.hpp`（轮播滑动）。

---

## 4. 事件分发（capture–target–bubble）

jtUI 的事件模型与 DOM 事件一致：每个 pointer / key / text 事件都会按三个阶段穿过 widget 树。

### 4.1 三个阶段

```
   root  ──→  panel  ──→  card  ──→  button     (Capture: top-down)
                                       ↓
                                  on_event()
                                       ↓
   root  ←──  panel  ←──  card  ←──  button     (Bubble: bottom-up)
```

1. **Capture**：从 root 到 target，每个 widget 收到 `on_event(Event{phase=Capture})`。容器 widget 可以在这里截获。
2. **Target**：只有命中的叶子 widget 才会收到 `on_event(Event{phase=Target})`。高层虚函数钩子（`on_click`、`on_press_changed` 等）通常都是在这一阶段被调起。
3. **Bubble**：从 target 一路回到 root，每一层祖先都会收到 `on_event(Event{phase=Bubble})`。设置 `ev.handled = true` 即可中止冒泡。

### 4.2 Hit testing

`hit_test(PointF)` 决定哪个叶子成为 target。默认实现：

```cpp
bool Widget::hit_test(PointF point) const noexcept {
    return visible() && frame().contains(point);
}
```

遍历是**从后往前**递归子节点（最后一个子节点最先检查 → 最后被绘制的最先被命中 → 符合 z-order 约定）。第一个返回 true 的 widget 即成为 target。

在容器中将 `intercepts_hit()` 设为 `true`，即使子节点也愿意接受命中，容器仍会强制夺取该 hit（`ScrollView` 就用这种方式让 thumb 而不是内层内容接到按下事件）。

### 4.3 `EventState`

跨消息保持的状态：

```cpp
struct EventState {
    Widget* hovered{nullptr};
    Widget* pressed{nullptr};
    Widget* focused{nullptr};
};
```

`Application` 为每个窗口维护一份 `EventState`，并把它传给 `EventDispatcher` 调用。每次事件到来时，dispatcher 会先**净化**这些指针——如果某个 `Widget*` 已经不在树中（rebuild 之后被释放），就会被置空。正是这一步阻止了"在点击中 rebuild"模式下的 UAF。

### 4.4 焦点

`Widget::accepts_focus()` 决定 `Tab` / 直接焦点指派是否落到这个 widget 上。默认值：`enabled()`。`Button` / `TextInput` / `Switch` / `Slider` 等都沿用默认值。非交互式 widget（`Text`、`Panel`、`CodiconIcon`）则将其重写为 `false`。

`Tab` / `Shift+Tab` 按文档顺序遍历 widget 树，跳过不可获焦的 widget。

---

## 5. Theme token 与解析

`Theme` 是全局单例。所有 widget 通过 `theme::colors()` / `theme::elevation()` 等接口读取 token——默认情况下每个 widget 不持有自己的 palette 状态。

### 5.1 SemanticColor 映射

每个 `ThemeMode`（Dark / Light）都把语义化名称映射到基础色板。映射定义在 `runtime/theme/src/theme.cpp`：

```cpp
const SemanticColor& Theme::colors_dark() noexcept {
    static SemanticColor c{
        .bg_base      = Color::from_hex("#0A0A0A"),
        .bg_surface   = Color::from_hex("#161616"),
        .accent       = Color::from_hex("#22C55E"),
        // ...
    };
    return c;
}
```

品牌示例会定义自己的 `brand::Palette` 和 `brand::active()` 提供示例专属色（`bg_card`、`bg_card_hover` 等），但内置 widget（`Button`、`Tabs`、`Dialog`）读取的仍然是框架层面的 `SemanticColor`。

### 5.2 `TokenOverride`

用于在不切换全局主题的前提下做 per-widget 定制：

```cpp
class Button {
public:
    void set_colors(Color idle, Color hover, Color pressed, Color text) {
        if (!override_) override_ = std::make_unique<TokenOverride>();
        override_->set_accent(idle);
        override_->set_accent_hover(hover);
        // ...
    }

    void paint(PaintContext& ctx) const override {
        const auto color = theme::resolve(override_.get()).accent;
        // ...
    }
};
```

`theme::resolve(override)` 如果对应字段已设置，就返回 `*override`，否则回落到全局的 `theme::colors()`。品牌侧的 `Button::set_colors(...)` 正是借此实现，而不会把单实例状态泄漏到全局 token 里。

### 5.3 主题切换

`Theme::set(ThemeMode::Light)` 会发出 `Theme::on_changed()` signal。应用代码通常会将其挂到整树 rebuild：

```cpp
jtui::theme::Theme::on_changed().connect([rebuild](auto) {
    rebuild();
});
```

整树 rebuild 会从新主题里重新读取每个 widget 的颜色。也可以把它挂到 `root.invalidate(DirtyFlags::Paint)`，做只刷新像素的处理（之所以可行，是因为每次 paint 时都会重新读取语义色 token）。

---

## 6. 动画 timer 与 `tick()`

jtUI 每个窗口共享一个 60 fps 的 `WM_TIMER`。只要树中任意 widget 的 `tick(delta)` 返回 `true`，timer 就会持续运行；当所有 widget 都返回 false 时，timer 会被关闭以节省 CPU。

### 6.1 tick 循环

```cpp
// application.cpp 约第 2211 行
case WM_TIMER:
    if (wparam == kAnimationTimerId) {
        const float delta_seconds = compute_tick_delta(window);
        if (tick_widget_tree(*content, delta_seconds)) {
            repaint_dirty_window_now(window, FALSE, /*defer_large=*/false);
        } else {
            stop_animation_timer(window);
        }
    }
```

`tick_widget_tree(widget, delta)` 会递归调用每个 widget 的 `widget.tick(delta)`，并把返回值按位 OR 起来。只要有一个返回 `true`，timer 就在这一帧存活。

### 6.2 按下 / 抬起时的同步 tick

为了消除 `WM_LBUTTONDOWN` 与第一次 `WM_TIMER` 触发之间约 16 ms 的延迟，`WM_LBUTTONDOWN` 和 `WM_LBUTTONUP` 各自会在**同步 paint 之前先跑一次同步 tick**：

```cpp
case WM_LBUTTONUP: {
    EventDispatcher::dispatch_pointer_up(content, point, state, activation);
    if (activation) activate_widget(window, activation, point);

    ensure_animation_timer(window);
    tick_widget_tree(*content, 1.0F / 60.0F);   // <-- 同步的第一次 tick
    repaint_dirty_window_now(window, FALSE, false);
}
```

这对按钮反馈来说很棒（pressed 视觉状态立刻就能看到），但对动画却有一个不那么直观的后果：

- **弹簧式动画**（`step = diff * k`）在第 1 帧 `diff` 很大时会跨出一大步——一个 760 px 的轮播滑动在第 1 帧就跳了 72 px，看起来像"瞬移之后再滑"。
- **进度 + 缓动动画**（`step = lerp(start, target, eased(progress))`）在第 1 帧的步长几乎为零，因为 `eased(0.02) ≈ 0.0001`——它能从静止状态平滑起步。

对轮播这类"从 A 移到 B"的动画，永远优先选择 progress + ease-in-out。参考实现见 `examples/jtui_cinema/carousel_animator.hpp`。

### 6.3 真实墙钟 `delta`

`compute_tick_delta` 使用 `QueryPerformanceCounter`，而不是硬编码的 `1/60`。这对音频锁定动画（例如波形进度）至关重要——一旦消息循环掉帧，下一次 tick 会前进 2/60 而不是 1/60，从而让画面与 WASAPI 时钟保持同步。

---

## 7. 整树 rebuild 作为状态模型

jtUI 推荐的状态模型是"把状态放在一个 struct 里，写 `build_root(state) → unique_ptr<Panel>`，每次状态变更都把整棵树替换一次"。听起来很重，但实际上很快，而且能规避细粒度响应式所带来的一类 bug。

### 7.1 为什么不慢

- 典型一屏只有几百个 widget；构造 `Panel`/`Text`/`Button` 成本很低（不分配 D2D 资源，不上传 GPU——只是 `std::make_unique` 加 `append_child`）。
- 局部重绘 dirty-rect 逻辑依旧生效：尽管所有 widget 都是重新构造的，dirty 合并出的区域只覆盖视觉上确实发生变化的那些地方（背景色、文本内容），D2D 也只重绘那些区域。
- profile 数据显示，在一台现代 Windows 桌面上，中等复杂度的 hero 页面每次 rebuild 大约 0.5–2 ms。

### 7.2 为什么在点击回调里也安全

朴素的"在点击 handler 里 rebuild"会导致 UAF：点击来自一个即将被 `window.set_content(new_root)` 销毁的按钮。jtUI 通过**延迟析构**来处理这个问题：

```cpp
// Window::set_content
void Window::set_content(std::unique_ptr<Widget> content) {
    if (content_) pending_destroy_.push_back(std::move(content_));
    content_ = std::move(content);
    mark_dirty(DirtyFlags::Structure);
}

// Application 会在每轮消息循环末尾把 pending_destroy_ 排空：
void Application::drain_pending_destroy(Window& window) {
    window.pending_destroy_.clear();
}
```

按钮的 `on_clicked` lambda 在其父 widget 真正被释放之前就已完成调用栈展开。正因如此，`state.foo = ...; rebuild();` 这种写法才是安全的。

### 7.3 保留重型 widget

`VideoPlayer` / `AudioPlayer` / `WaveformView` 内部持有 decoder 与 WASAPI 状态。rebuild 时销毁它们会打断播放。规范做法是 `release_child<T>` + 在 app 作用域内持有一个 `PersistentWidgets` 结构：

```cpp
struct PersistentWidgets {
    std::unique_ptr<jtui::VideoPlayer> video;
    jtui::VideoPlayer* video_raw{nullptr};
};

auto build_root = [&persist](...) {
    if (!persist.video) {
        persist.video = std::make_unique<jtui::VideoPlayer>();
        persist.video->set_source("intro.mp4");
    }
    persist.video->set_frame({...});
    persist.video_raw = persist.video.get();
    root->append_child(std::move(persist.video));
    return root;
};

auto rebuild = [&]() {
    if (auto* old = window.content()) {
        if (persist.video_raw) {
            persist.video = old->release_child<jtui::VideoPlayer>(persist.video_raw);
            persist.video_raw = nullptr;
        }
    }
    window.set_content(build_root(...));
};
```

每次 rebuild 都会在 `set_content` 把旧树替换之前，把这些持久化的 widget 从旧树里"救出来"，然后再挂到新树上。范例见 `examples/jtui_studio/main.cpp`。

### 7.4 保留动画进度

动画 widget 在 rebuild 时会被重建，从而把 `progress_` 重置为 `0.0F`。要在 rebuild 之间保留动画状态（例如轮播 offset、滚动位置），就把状态放到你的 `AppState` 结构里，让动画器通过回调把数据写回：

```cpp
animator->set_on_current_changed([&state](float new_current) {
    state.carousel_offset = new_current;
});
animator->set_offsets(state.carousel_offset, state.carousel_target);
```

下一次 rebuild 出来的新动画器会读 `state.carousel_offset`，从那里继续往下。完整模式见 `examples/jtui_cinema/carousel_animator.hpp`。

---

## 8. Z-order 与 hit testing

**没有显式的 z-index API。** Z-order = 追加顺序，后追加的在上层。这是在排版 widget 时最需要内化的约定。

### 8.1 影响

- 模态 scrim + popover 必须是 root 最后追加的子节点。
- 背景装饰（网格、水印）必须是最早追加的几个子节点。
- 与其他 widget 在同一 X/Y 区域叠放的 overlay 按钮，必须**在**它要盖住的 widget 之后追加——否则底下的 widget 会通过 hit_test 抢走点击。

### 8.2 常见坑

- **`folders_app` 当初发布了一个有 bug 的 About 按钮**，因为 navbar 右侧的链接（`Features` / `Pricing` Text widget）在 X 重叠的位置被追加到 About 按钮之后。点击 About 按钮命中了 Text widget（它不消费点击，但默认也不放行）。修复方式：把 Text widget 挪出 X 重叠区，或在按钮之前追加。
- **`jtui_pro` 当初发布了一个有 bug 的 About 按钮**，因为它已经在完全相同的 X 公式上有一个 `bg_btn`（背景模式切换）。Z-order 让 `bg_btn` 完全盖住了 `about_btn`。修复方式：把 About 按钮的 X 公式整体左移一格。



如果确实需要显式 z-index，请提 issue——它在 backlog 里，但目前还没实现。

---

## 9. 跨平台模型

jtUI 当前只在 **Windows 运行时**可用。Linux 与 macOS 只支持交叉编译 + 单元测试。

### 9.1 为什么只支持 Windows

渲染和媒体后端与 Microsoft 自家 API 深度绑定：

- **Direct2D 1.1** 提供硬件加速 2D + Effects（`CLSID_D2D1Shadow`、`CLSID_D2D1GaussianBlur`）
- **DirectWrite** 处理文字 shaping + 彩色 emoji 字体 + 自定义 `IDWriteFactory5` 字体集合（codicons）
- **Media Foundation** 解码视频（MP4 中的 H.264）
- **WASAPI 共享模式**输出音频

目前没有 GTK / Cocoa / SDL 兜底。

### 9.2 Linux 构建

`Application::run()` 在非 Windows 平台直接返回 0。渲染命令不会被 replay（没有 D2D target），但 widget 树照样可以构造，无像素的单元测试可以跑。

`platform/win32/CMakeLists.txt` 由 `if(WIN32)` 包裹。Linux 原生构建会跳过它。用 MinGW-w64 交叉编译时会启用 `_WIN32`，platform/win32 也会被一并引入。

### 9.3 路线图：SDL3 + Skia

未来会有一个 `platform/skia/`，提供同样的后端接口，但底层换成 Skia + SDL3，从而支持 Linux 原生 + macOS。widget 目录和 runtime 设计上就是后端无关的——`PaintContext` 产出的是 IR，只有 replay 阶段是平台相关的。

---

## 10. 性能注释

### 10.1 帧预算

- 空闲（无动画）：不消耗 CPU。`WM_TIMER` 处于停止状态。
- 动画运行中：在现代桌面上，典型 hero 页面（约 200 个 widget）每轮 `tick_widget_tree` 大约 0.1–1 ms。
- 整树 rebuild：约 0.5–2 ms（大头是 `std::make_unique<Widget>` 调用）。
- D2D replay：约 1–4 ms，取决于 dirty rect 大小和 Effect（Shadow / GaussianBlur）数量。

在 2024 年的笔记本上，典型品牌示例跑 60 fps 动画大约占用一颗 CPU 核心的 15–25%。

### 10.2 GPU effect 缓存

`CLSID_D2D1Shadow` 与 `CLSID_D2D1GaussianBlur` 这两个 Effect 对象创建很昂贵（冷启动约 10–30 ms）。它们被缓存在 `DirectTextSession`（每个 render target 一份）里，跨帧复用。每个 Effect 每次命令时只是重新设置参数再绘制。

新 render target 上第一帧使用 Effect 会有冷启动开销（主题切换时能看到一次卡顿）；后续帧很平滑。

### 10.3 避免在动画中频繁 `set_frame`

如 § 3.4 所述——`set_frame` 会触发 `mark_dirty(Layout|Paint)` 并向父节点冒泡。对很多 widget 每秒做 60 次这种操作非常浪费。优先选择 `translate_subtree` 再加一次容器级别的 dirty mark。

### 10.4 文本绘制是最慢的图元

DirectWrite layout 在内部已有缓存，但每条 `DrawText` 命令仍然比 `FillRect` 慢约 10 倍。对于有大量小文本标签的 widget（例如图表的坐标刻度），把它们整合成更少的 `set_runs` 调用（在同一个 Text widget 里分段）会比拆成很多独立的 `Text` widget 更快。

### 10.5 动画节流：WM_MOUSEWHEEL

快速滚动时 `WM_MOUSEWHEEL` 可能淹没消息队列，把 `WM_TIMER` 饿死。v1.24 的做法是：当 ScrollView 是滚轮目标时，在 `WM_MOUSEWHEEL` handler 里直接 tick——参见 `application.cpp` 约第 2152 行。

---

## 11. 扩展点

### 11.1 自定义 widget

继承 `hui::Widget`，重写 `paint(PaintContext&)`，按需重写 `tick(float)`、`hit_test(PointF)`、事件钩子和 `type_name()`。两个完整示例：`examples/jtui_cinema/thumbnail_art.hpp`（paint 密集型）与 `examples/jtui_invest/animated_widgets.hpp`（tick 密集型）。

自定义 widget 只针对公开的 `hui::` / `jtui::` 头编译——无需改动框架源码。

### 11.2 自定义 decoder

实现 `IVideoDecoder` 或 `IAudioDecoder`（见 `runtime/media/include/hui/media/decoder.hpp`），并通过 factory 注册。默认 Windows factory 返回的是 `MFVideoDecoder` / `WasapiAudioOutput`；自定义 factory 可以替换它们。可用于补齐 MF 自带不到的编解码器（MKV 中的 VP9、AV1）或换用别的解码后端（libav、GStreamer）。

### 11.3 自定义后端

`runtime/render/PaintContext` 产出 IR；`application.cpp::replay_paint_commands` 是唯一把 IR 翻译成 D2D 的地方。把它替换成 Skia 或 SDL_Renderer 的实现，就是支持 macOS / Linux 原生的路径。widget 目录与事件分发器都是后端无关的。

### 11.4 FFI

`ffi/c_api/include/hui/c_api.h` 暴露稳定的 C ABI。从 Rust、Go、Python 等绑定时使用它。目前 C API 覆盖 `Application` / `Window` 生命周期以及基础 widget 创建；后续会按需补充更多 widget 绑定。当前接口面见 `ffi/c_api/include/hui/c_api.h`。

---

## 参见

- [快速上手](getting-started.zh-CN.md) — 教程
- [API 参考](api-reference.zh-CN.md) — 所有公开类型
- [README](../README.zh-CN.md) — 项目概览

要深入阅读实现：可以从 `runtime/object/include/hui/object/widget.hpp`（中心抽象）和 `runtime/app/src/application.cpp`（消息循环 + paint 后端集成）开始，其他一切都从这两处发散开来。
