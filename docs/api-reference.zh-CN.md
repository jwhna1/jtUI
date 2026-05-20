# jtUI API 参考

jtUI v1.0.0 完整 API 参考。公开符号位于 `hui::` namespace，并通过总入口头文件 `#include "jtui/jtui.hpp"` 以 `jtui::` 别名暴露。

学习框架请先看 [`快速上手`](getting-started.zh-CN.md)。内部机制见 [`架构文档`](architecture.zh-CN.md)。

[English](api-reference.md)

---

## 目录

1. [Foundation（基础设施）](#1-foundation) — `Color`、`i18n`、`log`、codicon
2. [对象模型](#2-object-model) — `Node`、`Element`、`Widget`、`DirtyFlags`
3. [Property 与 Signal](#3-property--signals) — `Signal<T>`、`Property<T>`、`RealtimeSource<T>`
4. [Theme（主题）](#4-theme) — `Theme`、`SemanticColor`、elevation / spacing / typography token
5. [Event（事件）](#5-event) — `EventDispatcher`、`MouseEvent`、`KeyEvent`、`TextInputEvent`
6. [Render（渲染）](#6-render) — `PaintContext`、`DrawCommand`
7. [Application（应用层）](#7-application) — `Application`、`Window`、`WindowOptions`、`WindowFrame`、`TitleBar`
8. [widgets/basic（基础 widget）](#8-widgetsbasic) — `Panel`、`Text`、`CodiconIcon`、`ScrollView`、`ListView`、`FlexBox`
9. [widgets/common（通用 widget）](#9-widgetscommon) — `Button`、`TextInput`、`Switch`、`Checkbox`、`Tabs`、`Dialog`、`Popover`、`Tooltip`、`Slider`、`Toolbar`、`Dropdown`、`SearchInput`、`ProgressBar`、`RadialGauge`、`SemiGauge`、`Gauge`、`Chip`、`Badge`、`Card`、`Avatar`、`FolderCard`、`MetricCard`、`SidebarNav`、`AboutCard`
10. [widgets/media（媒体 widget）](#10-widgetsmedia) — `VideoPlayer`、`AudioPlayer`、`WaveformView`、`Timeline`、`LevelMeter`
11. [媒体抽象](#11-media-abstractions) — `IVideoDecoder`、`IAudioDecoder`、`TextureHandle`、`PixelBuffer`
12. [几何类型](#12-geometry-types) — `RectF`、`PointF`、`SizeF`
13. [公开 C API（FFI）](#13-public-c-api-ffi)

---

## 1. Foundation

### `Color`

RGBA 颜色，每个通道 32-bit float，alpha 预乘（pre-multiplied）。

```cpp
namespace hui {
struct Color {
    float r{0.0F}, g{0.0F}, b{0.0F}, a{1.0F};

    [[nodiscard]] static Color from_hex(std::string_view hex);
    // "#RRGGBB" 或 "#RRGGBBAA"。解析失败返回 {0,0,0,1}。

    [[nodiscard]] static Color from_rgba8(uint8_t r, uint8_t g, uint8_t b,
                                          uint8_t a = 255);
};
}
```

注意：`Color{}` 是**不透明的黑色**（a = 1.0），不是透明。透明请用 `Color{0,0,0,0}`。

```cpp
auto accent  = jtui::Color::from_hex("#FB923C");
auto warning = jtui::Color::from_rgba8(255, 180, 84);  // alpha 默认 255
auto glass   = jtui::Color{0.0F, 0.0F, 0.0F, 0.55F};   // 半透明
```

### i18n

双语（英文 / 中文）字符串目录，一行切换语言。

```cpp
namespace hui::i18n {

enum class Locale : uint8_t { En, Zh };

struct Entry {
    std::string key;
    std::string en;
    std::string zh;
};

void register_entries(const std::vector<Entry>& entries);

[[nodiscard]] const std::string& tr(std::string_view key);

void set_locale(Locale locale) noexcept;
[[nodiscard]] Locale current_locale() noexcept;

// 语言切换时发射的 Signal（供绑定 key 的 widget 使用）。
[[nodiscard]] Signal<Locale>& on_changed() noexcept;

}
```

```cpp
jtui::i18n::register_entries({
    {"nav.home",   "Home",  "首页"},
    {"hero.title", "Build Native. Ship Fast.", "原生构建，极速出货。"},
});
jtui::i18n::set_locale(jtui::i18n::Locale::Zh);
auto label = jtui::i18n::tr("nav.home");   // → "首页"
```

`register_entries` 是幂等的 —— 用相同 `key` 再次调用会覆盖原值。从多个模块调用是安全的。

### 日志

```cpp
namespace hui::foundation {
enum class LogLevel : uint8_t { Debug, Info, Warn, Error };
void log_set_enabled(bool on) noexcept;
void log(LogLevel level, std::string_view tag, std::string_view msg);
}

#define HUI_LOG_D(tag, msg)  hui::foundation::log(LogLevel::Debug, tag, msg)
#define HUI_LOG_I(tag, msg)  hui::foundation::log(LogLevel::Info,  tag, msg)
#define HUI_LOG_W(tag, msg)  hui::foundation::log(LogLevel::Warn,  tag, msg)
#define HUI_LOG_E(tag, msg)  hui::foundation::log(LogLevel::Error, tag, msg)
```

日志输出到 stderr，在 Windows 上同时输出到 `OutputDebugStringA`。用 `tag` 来过滤（`"anim"`、`"backdrop"`、`"video"` 等）。

### Codicon 查找

`@vscode/codicons` 字形以 TTF 形式内嵌于二进制中。按名称查找即可得到 Unicode 码点：

```cpp
namespace hui::foundation {
[[nodiscard]] std::optional<uint32_t> codicon_codepoint_for(std::string_view name);
}
```

```cpp
const auto cp = hui::foundation::codicon_codepoint_for("globe");  // → 0xEB7C
```

大多数 widget 直接接受名称：

```cpp
button->set_leading_codicon("play");
icon->set_codicon_name("info");
```

完整名称表（649 项）见 `runtime/foundation/include/hui/foundation/codicon_codepoints.hpp`。

---

## 2. 对象模型

### `Node`（内部基类）

持有 `std::vector<std::unique_ptr<Node>>` 形式的子节点列表。提供 `append_child(std::unique_ptr<Node>)` 以及受保护的 `release_child(const Node*) → std::unique_ptr<Node>` 操作。应用代码很少直接接触 `Node` —— 一般通过 `Widget` 交互。

### `Element` 继承自 `Node`

新增 frame（位置 + 尺寸）以及 dirty flag 的传播能力。

```cpp
namespace hui {

enum class DirtyFlags : uint32_t {
    None      = 0,
    Structure = 1 << 0,   // 子节点添加/删除/重排
    Layout    = 1 << 1,   // 尺寸/位置变化，需要 measure+arrange
    Paint     = 1 << 2,   // 像素变化，需要重绘
    Style     = 1 << 3,   // 语义色 token 变化
};

class Element : public Node {
public:
    [[nodiscard]] RectF frame() const noexcept;
    void set_frame(RectF frame) noexcept;          // 标记 Layout+Paint dirty

    [[nodiscard]] bool visible() const noexcept;
    void set_visible(bool visible) noexcept;       // 标记 Paint dirty

    void invalidate(DirtyFlags flags = DirtyFlags::Paint) noexcept;

    // v1.20：跳过 dirty 传播的子树位置偏移。
    // 用于动画/滚动场景：在容器上一次性标记 dirty 即可。
    void translate_subtree(float dx, float dy) noexcept;

    void shift_subtree(float dx, float dy) noexcept;  // 同上但会标记 dirty
};

}
```

`translate_subtree` 与 `set_frame` 之间的取舍是性能上的关键决策：

- **`set_frame`** —— 用于一次性布局。会自动在当前 widget 上标记 Layout|Paint dirty；dirty rect 只覆盖新位置。
- **`translate_subtree`** —— 用于动画/滚动。直接改写 `frame_.x/y` 而不标记 dirty；**你必须在某个 `frame` 覆盖整个受影响区域的容器 widget 上标记 dirty**，否则会出现残影。详细说明见 [`快速上手 § 7`](getting-started.zh-CN.md#7-animation)。

### `Widget` 继承自 `Element`

所有可交互 UI 构件的基类。

```cpp
namespace hui {

class Widget : public Element {
public:
    [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;

    // ──── 状态查询 ────
    [[nodiscard]] bool enabled() const noexcept;
    void set_enabled(bool enabled) noexcept;

    [[nodiscard]] bool focused() const noexcept;
    void set_focused(bool focused) noexcept;

    [[nodiscard]] bool hovered() const noexcept;
    [[nodiscard]] bool pressed() const noexcept;

    // paint 是否读取 pressed()。默认 false。Button 类 widget
    // 需重写为 true 以便按下时重绘。
    [[nodiscard]] virtual bool paint_reacts_to_pressed() const noexcept;

    // ──── 命中测试 ────
    [[nodiscard]] virtual bool hit_test(PointF point) const noexcept;
    // 默认实现：visible() && frame().contains(point)

    [[nodiscard]] virtual bool accepts_focus() const noexcept;

    // 返回 true 则中断 hit_test 递归 —— 即使 children 也包含该点，
    // `this` widget 仍作为叶子节点返回。
    [[nodiscard]] virtual bool intercepts_hit() const noexcept;

    // ──── 光标 ────
    [[nodiscard]] virtual CursorKind cursor_kind() const noexcept;
    // CursorKind: Default, IBeam, Hand, ResizeH, ResizeV

    // ──── Paint ────
    virtual void paint(PaintContext&) const;
    virtual void paint_overlay(PaintContext&) const;   // 在 children 之后绘制
    [[nodiscard]] virtual bool clips_self() const noexcept;
    [[nodiscard]] virtual bool clips_children() const noexcept;

    // ──── Event（低层） ────
    virtual void on_mouse_move(PointF) {}
    virtual void on_mouse_leave() {}
    virtual void on_press_changed(PointF, bool) {}
    virtual void on_click(PointF) {}
    [[nodiscard]] virtual bool on_key_down(Key, bool shift, bool ctrl, bool alt);
    [[nodiscard]] virtual bool on_text_input(char);

    // 低层事件分发入口 —— 重写以拦截 capture/target/bubble 阶段。
    virtual bool on_event(Event& ev);

    // ──── 动画 ────
    [[nodiscard]] virtual bool tick(float delta);
    // 返回 true 保持动画 timer 继续运行。

    // ──── Layout（容器需重写） ────
    [[nodiscard]] virtual SizeF measure(SizeF available) const;
    virtual void arrange(RectF frame_in);

    // ──── 类型安全的 release_child ────
    template <typename T>
    [[nodiscard]] std::unique_ptr<T> release_child(const T* target) noexcept;
};

}
```

### z-order 约定

**默认 z-order 等同于 append 顺序 —— 最后 append 的 widget 在最上层**（更晚绘制 + 优先接收 hit_test）。没有显式的 `z-index`。要让 modal/popover 浮在其他 widget 之上，把它最后 append 到父节点即可。原因和注意事项见 [`widget.hpp:248` 注释](../runtime/object/include/hui/object/widget.hpp)。

### `clips_self` 的局限

返回 `true` 会让 runtime 在 `paint()` 之前自动 push 一个 `push_clip(self.frame)` 矩形裁剪。适用于在自定义 widget 上实现 "overflow: hidden" 语义（当绘制超出 frame 时）。

**重要**：裁剪区域是**矩形**的，不是圆角的。如果你的 widget 设了 `corner_radius` 并在圆角附近绘制几何体，圆角扇形内的几何体仍然会绘制到圆形之外。要么把几何顶点坐标收紧避开圆角，要么用更小的 `corner_radius`。一个解决案例见 `examples/jtui_cinema/thumbnail_art.hpp`。

---

## 3. Property 与 Signal

### `Signal<Args...>`

带类型擦除连接的回调列表。

```cpp
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;
    using ConnectionId = uint64_t;

    ConnectionId connect(Slot slot);     // 返回用于断开的 id
    void disconnect(ConnectionId id);
    void disconnect_all();
    void emit(Args... args);

    [[nodiscard]] std::size_t connection_count() const noexcept;
};
```

```cpp
jtui::Signal<int, bool> sig;
auto id = sig.connect([](int n, bool flag) {
    std::cout << n << " " << flag << "\n";
});
sig.emit(42, true);
sig.disconnect(id);
```

**生命周期注意**：没有自动 RAII 的 `ScopedConnection`。在长期存活的状态 Signal 上，lambda 捕获了 widget 裸指针，一旦 widget 销毁就会出现悬空指针。涉及 widget 状态交互时，更推荐 full-tree rebuild 模式 —— 讨论见 [`架构文档`](architecture.zh-CN.md)。

### `Property<T>`

`Signal<T>` 之上的轻量封装，附带一个当前值。

```cpp
template <typename T>
class Property {
public:
    Property() = default;
    explicit Property(T value);

    [[nodiscard]] const T& get() const noexcept;
    void set(T value);

    [[nodiscard]] Signal<T>& changed() noexcept;
};
```

```cpp
jtui::Property<bool> dark_mode{true};
dark_mode.changed().connect([](bool dark) { /* react */ });
dark_mode.set(false);  // 发射 changed
```

### `RealtimeSource<T>`

为高频数据（音频电平、视频位置、传感器流）提供的 lock-free 最新值生产者/消费者对。生产者发布；消费者轮询 `generation()` 来检测更新。

```cpp
template <typename T>
class RealtimeSource {
public:
    void publish(T value);                   // 生产者侧
    [[nodiscard]] T latest() const noexcept; // 消费者侧
    [[nodiscard]] uint64_t generation() const noexcept;
};

template <typename T>
class LatestValue { /* RealtimeSource<T> 的只读视图 */ };

template <typename T>
class RealtimeRingBuffer { /* lock-free SPSC ring */ };
```

`RealtimeCanvas::bind_source<T>(source)` 把自定义 paint widget 绑到 `RealtimeSource<T>` —— 它的 `tick()` 会轮询 `generation()`，值前进时标记 dirty。

---

## 4. Theme（主题）

### `Theme` 单例

全局 mode + 变更 Signal。所有 widget 通过 `theme::colors()` / `theme::elevation()` 等读取 token，所以一次 mode 切换 + 重绘就能完成整个 app 的换肤。

```cpp
namespace hui::theme {

enum class ThemeMode : uint8_t { Dark, Light };

class Theme {
public:
    [[nodiscard]] static ThemeMode mode() noexcept;
    static void set(ThemeMode mode) noexcept;

    [[nodiscard]] static Signal<ThemeMode>& on_changed() noexcept;
};

}
```

```cpp
jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
jtui::theme::Theme::on_changed().connect([](auto mode) {
    // app 级响应。一般只是调用 rebuild()。
});
```

### `SemanticColor` token

查找主题色。每种 mode（Dark/Light）把语义名映射到基础色板。

```cpp
namespace hui::theme {

struct SemanticColor {
    Color bg_base;         // 窗口背景
    Color bg_surface;      // card / panel 表面
    Color bg_surface_alt;  // 替代表面
    Color bg_raised;       // 凸起 card
    Color border;
    Color border_strong;
    Color text_strong;
    Color text_primary;
    Color text_secondary;
    Color text_muted;
    Color text_disabled;
    Color accent;
    Color accent_hover;
    Color accent_pressed;
    Color accent_soft;
    Color accent_fg;
    Color danger;
    Color warning;
    Color info;
    Color success;
};

[[nodiscard]] const SemanticColor& colors() noexcept;
[[nodiscard]] const SemanticColor& resolve(const TokenOverride* override) noexcept;

}
```

大多数品牌示例会定义自己的 `brand::Palette`，字段名相同再加品牌专属字段（`bg_card`、`bg_card_hover` 等）—— 标准范式见 `examples/jtui_cinema/brand_tokens.hpp`。

### Elevation token

5 级阴影，对应 CSS box-shadow 语义。Windows 上由 `CLSID_D2D1Shadow` 支持。

```cpp
namespace hui::theme {

struct ElevationLevel {
    PointF offset;   // 阴影偏移 (x, y)
    float  blur;     // 高斯 sigma * 3
    float  spread;
    Color  color;
};

struct Elevation {
    ElevationLevel level_0;  // 平面
    ElevationLevel level_1;  // 按钮 hover
    ElevationLevel level_2;  // card
    ElevationLevel level_3;  // popover
    ElevationLevel level_4;  // dialog
};

[[nodiscard]] const Elevation& elevation() noexcept;

}
```

```cpp
card->set_shadow(jtui::theme::elevation().level_2);
```

### Spacing 与 Typography token

```cpp
namespace hui::theme {

struct Spacing {
    float xs;   // 4
    float sm;   // 8
    float md;   // 12
    float lg;   // 16
    float xl;   // 24
    float xxl;  // 32
};

struct Typography {
    float size_xs;     // 11
    float size_sm;     // 12
    float size_body;   // 14
    float size_md;     // 16
    float size_lg;     // 20
    float size_xl;     // 28
    float size_xxl;    // 42
};

[[nodiscard]] const Spacing& spacing() noexcept;
[[nodiscard]] const Typography& typography() noexcept;

}
```

### Token override

针对单个 widget 覆盖某些语义色，不影响全局主题。

```cpp
namespace hui::theme {

class TokenOverride {
public:
    void set_text_strong(Color c);
    void set_accent(Color c);
    // ... 每个 SemanticColor 字段对应一个 setter ...

    // 取色：若已 override 则返回 override 值，否则回退到全局主题。
    [[nodiscard]] Color resolve_text_strong() const noexcept;
};

}
```

Widget 子类持有 `std::unique_ptr<TokenOverride>`，在 paint 时通过 `theme::resolve(override.get())` 取值。

### VSCode 色彩 token

`gallery` 示例里有一个 `vscode_palette` 章节，演示了 VSCode token 目录（semantic + 基础色板）。API 见 `runtime/theme/include/hui/theme/vscode_tokens.hpp`。当你想为代码编辑器风格的 widget 做到与 VS Code 完全配色一致时使用它们。

---

## 5. Event（事件）

### Event 类型

```cpp
namespace hui {

enum class MouseButton : uint8_t { Left, Right, Middle };

struct MouseEvent {
    PointF position;
    MouseButton button{MouseButton::Left};
    bool pressed{false};
    bool double_click{false};
};

struct WheelEvent {
    PointF position;
    float  delta_x{0.0F};
    float  delta_y{0.0F};
};

enum class Key : uint16_t {
    Unknown, Tab, Enter, Escape, Backspace, Delete, Space,
    Left, Right, Up, Down, Home, End, PageUp, PageDown,
    A, B, C, ..., Z, Num0, Num1, ..., Num9,
    F1, ..., F12,
};

struct KeyEvent {
    Key  key{Key::Unknown};
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool down{true};       // 抬起时为 false
    bool autorepeat{false};
};

struct TextInputEvent {
    uint32_t code_point{0};  // Unicode (UTF-32)
};

// 用于低层 on_event 的 tagged-union，涵盖所有 event 类别。
using EventVariant = std::variant<MouseEvent, WheelEvent, KeyEvent, TextInputEvent>;

struct Event {
    EventVariant payload;
    enum class Phase : uint8_t { Capture, Target, Bubble };
    Phase phase{Phase::Target};
    bool handled{false};
};

}
```

### `EventDispatcher`

`Application` 把 Win32 消息路由给 widget 时使用的静态外观（facade）。

```cpp
namespace hui {

struct EventState {
    Widget* hovered{nullptr};
    Widget* pressed{nullptr};
    Widget* focused{nullptr};
};

class EventDispatcher {
public:
    static Widget* dispatch_pointer_down(
        Widget* root, PointF point, EventState& state,
        MouseButton button = MouseButton::Left);

    static void dispatch_pointer_up(
        Widget* root, PointF point, EventState& state,
        Widget*& activation);

    static void dispatch_pointer_move(
        Widget* root, PointF point, EventState& state);

    static bool dispatch_key_down(
        Widget* root, EventState& state, Key key,
        bool shift, bool ctrl, bool alt);

    static bool dispatch_text_input(
        Widget* root, EventState& state, uint32_t code_point);

    static bool set_focus(EventState& state, Widget* target);
};

}
```

分发按 **capture → target → bubble** 三阶段管线运行。多数用户代码不直接调用 `EventDispatcher` —— 由 `Application` 调用。要在任意阶段拦截，可重写 `Widget::on_event(Event&)`，或者使用高层虚钩子（`on_click`、`on_mouse_move` 等）。

---

## 6. Render（渲染）

### `PaintContext`

绘图表面，背后是一份内存中的 `DrawCommand` 列表，每 frame 重放到 D2D。

```cpp
namespace hui {

class PaintContext {
public:
    // ──── 矩形与椭圆 ────
    void fill_rect(RectF bounds, Color color);
    void stroke_rect(RectF bounds, Color color, float thickness = 1.0F);

    void fill_rounded_rect(RectF bounds, Color color, float radius);
    void stroke_rounded_rect(RectF bounds, Color color, float radius,
                              float thickness = 1.0F);

    void fill_ellipse(RectF bounds, Color color);
    void stroke_ellipse(RectF bounds, Color color, float thickness = 1.0F);

    // ──── 路径 ────
    void line(PointF start, PointF end, Color color, float thickness = 1.0F);

    void draw_bezier(PointF p0, PointF p1, PointF p2, PointF p3,
                      Color color, float thickness = 1.0F);
    // 三次贝塞尔。p1/p2 为控制点。

    void fill_polygon(std::vector<PointF> points, Color color);
    // 自动闭合（最后一点 → 第一点）。点数 < 3：noop。> 200：后端可能裁剪。

    // ──── 渐变 ────
    void fill_gradient_rect(RectF bounds, Color color_top, Color color_bottom,
                             float radius = 0.0F);
    // 垂直线性渐变。radius=0 表示非圆角矩形。

    // ──── 特效（D2D 1.1） ────
    void fill_shadow(RectF bounds, float corner_radius,
                      PointF offset, float blur, float spread, Color color);
    // CSS box-shadow 语义。在 D2D 1.0 上回退为 noop。

    void fill_backdrop_blur(RectF bounds, float corner_radius,
                             float blur, Color tint);
    // 毛玻璃效果：CopyFromRenderTarget → GaussianBlur → DrawImage + tint。
    // 在 D2D 1.0 上回退为纯色 tint。

    // ──── 文本 ────
    void draw_text(RectF bounds, std::string text, Color color,
                    TextAlignment alignment = TextAlignment::Leading,
                    float font_size = 14.0F, bool bold = false);

    void draw_text_with_font(RectF bounds, std::string text, Color color,
                              std::string font_family, TextAlignment alignment,
                              float font_size, bool bold);

    // ──── 纹理 ────
    void draw_texture(RectF bounds, PixelBuffer buffer,
                       AlphaMode alpha_mode = AlphaMode::Premultiplied);

    // ──── 裁剪 ────
    void push_clip(RectF bounds);
    void pop_clip();
};

}
```

### `TextAlignment`

```cpp
enum class TextAlignment : uint8_t { Leading, Center, Trailing };
```

### `DrawCommand`（内部）

每次 paint() 调用都会向其中发出指令的 IR。应用代码不直接构造 —— 如需检查或重放命令，schema 见 `runtime/render/include/hui/render/paint_context.hpp`。

---

## 7. Application（应用层）

### `Application`

持有消息循环和所有 `Window` 实例。

```cpp
namespace hui {

class Application {
public:
    Application();
    ~Application();

    [[nodiscard]] Window& create_window(const WindowOptions& options);

    int run();
    // Windows 上会阻塞。在非 Windows 平台上立即返回 0。
};

}
```

```cpp
int main() {
    hui::Application app;
    auto& window = app.create_window({...});
    window.set_content(build_root());
    return app.run();
}
```

### `WindowOptions`

```cpp
struct WindowOptions {
    std::string title;
    SizeF       size;
    bool        frameless{false};    // true → 通过 WindowFrame 实现自定义标题栏
    bool        resizable{true};
    bool        maximizable{true};
    bool        minimizable{true};
};
```

### `Window`

```cpp
class Window {
public:
    void set_content(std::unique_ptr<Widget> root);
    [[nodiscard]] Widget* content() const noexcept;

    [[nodiscard]] SizeF size() const noexcept;
    void set_size(SizeF size) noexcept;

    void close();
    void minimize();
    void maximize();
    void restore();

    void invalidate(DirtyFlags flags = DirtyFlags::Paint) noexcept;

    [[nodiscard]] Signal<>& on_close() noexcept;
    [[nodiscard]] Signal<SizeF>& on_resized() noexcept;
};
```

`set_content` 把旧的 root 退役进 `pending_destroy_` 队列，再装入新 root。旧树会在当前 event handler 返回后才释放 —— 这正是 "在点击回调里 rebuild" 之所以安全的原因（不会出现触发该点击的 widget 被 use-after-free）。

### `WindowFrame`

自定义标题栏外壳（当 `WindowOptions::frameless = true` 时启用）。支持拖动移动 + 双击最大化 + Win32 边缘调整大小。

```cpp
class WindowFrame : public Widget {
public:
    void set_frameless(bool frameless) noexcept;
};
```

### `TitleBar`

只包含视觉上的标题栏文本 + 关闭/最小化/最大化按钮。配合 `WindowFrame` 使用。

```cpp
class TitleBar : public Widget {
public:
    explicit TitleBar(std::string title);

    void set_title(std::string title);
    void set_show_minimize(bool show);
    void set_show_maximize(bool show);
};
```

---

## 8. widgets/basic（基础 widget）

### `Panel`

带可选背景、边框、阴影、毛玻璃和圆角的矩形容器。最常用的 widget。

```cpp
namespace hui {

enum class PanelRole : uint8_t {
    Base,         // 窗口背景
    Surface,      // card 表面
    SurfaceAlt,   // 替代表面
    Raised,       // 凸起表面
};

class Panel : public Widget {
public:
    void set_role(PanelRole role) noexcept;
    void set_background_color(Color color) noexcept;
    void clear_background_color() noexcept;      // 恢复主题 token

    void set_corner_radius(float radius) noexcept;

    void set_border(Color color, float thickness = 1.0F) noexcept;
    void clear_border() noexcept;

    void set_shadow(theme::ElevationLevel level) noexcept;
    void clear_shadow() noexcept;

    void set_backdrop_blur(float blur, Color tint) noexcept;
    void clear_backdrop_blur() noexcept;
};

}
```

```cpp
auto card = std::make_unique<jtui::Panel>();
card->set_role(jtui::PanelRole::Surface);
card->set_corner_radius(12.0F);
card->set_shadow(jtui::theme::elevation().level_2);
card->set_frame({40, 40, 320, 200});
```

**毛玻璃陷阱**：启用 `set_backdrop_blur(...)` 后，请同时调用 `set_background_color({0,0,0,0})` 把 panel 变成透明 —— 否则不透明的背景会盖住模糊层。

### `Text`

渲染一行文本（或通过 `\n` 多行）。通过 `set_runs` 支持行内着色片段。

```cpp
struct TextRun {
    std::string text;
    Color       color{};       // Color{} → "不覆盖"，使用 widget 的 set_color
    bool        color_explicit{false};

    TextRun() = default;
    explicit TextRun(std::string t);
    TextRun(std::string t, Color c);  // 标记 color_explicit
};

class Text : public Widget {
public:
    Text() = default;
    explicit Text(std::string content);

    void set_content(std::string content);

    // 行内多色：按顺序水平绘制每个 TextRun。
    void set_runs(std::vector<TextRun> runs);

    void set_font_size(float size) noexcept;
    void set_bold(bool bold) noexcept;
    void set_color(Color color) noexcept;
    void set_alignment(TextAlignment alignment) noexcept;

    enum class Role : uint8_t { Primary, Secondary, Muted };
    void set_role(Role role) noexcept;

    enum class StylePreset : uint8_t {
        Title, SubTitle, Body, Caption, Mono,
    };
    void set_preset(StylePreset preset) noexcept;
};
```

```cpp
auto title = std::make_unique<jtui::Text>();
title->set_font_size(42.0F);
title->set_bold(true);
title->set_color(p.text_strong);
title->set_runs({
    jtui::TextRun{"Build "},
    jtui::TextRun{"native", p.accent},   // 着色片段
    jtui::TextRun{". Ship fast."},
});
title->set_frame({40, 200, 600, 60});
```

### `CodiconIcon`

单个 VSCode codicon 字形作为 widget。

```cpp
class CodiconIcon : public Widget {
public:
    CodiconIcon() = default;
    explicit CodiconIcon(std::string name);

    void set_codicon_name(std::string name);
    void set_color(Color color) noexcept;
    void set_size_px(float size) noexcept;
};
```

```cpp
auto chk = std::make_unique<jtui::CodiconIcon>("check");
chk->set_color(p.success);
chk->set_size_px(16.0F);
chk->set_frame({x, y, 16, 16});
```

### `ScrollView`

垂直滚动容器。通过 `set_content(unique_ptr<Widget>)` 设置内容；内容可以高于视口。

```cpp
class ScrollView : public Widget {
public:
    void set_content(std::unique_ptr<Widget> content);
    [[nodiscard]] Widget* content() const noexcept;

    void set_scroll_offset(PointF offset);
    [[nodiscard]] PointF scroll_offset() const noexcept;

    void set_smooth_scroll(bool smooth) noexcept;

    void arrange(RectF frame_in) override;
    // append 到父节点后必须显式调用一次 —— 见下方陷阱。
};
```

**陷阱**：`Window` 只对 root 调用 `arrange()`。`Panel::arrange` 并不递归，所以嵌套在 `Panel` 里的 `ScrollView` 不会被 arrange → `content_size_` 一直是 0 → 滚动被夹到 0。append 后手动 arrange 一次：

```cpp
auto scroll = std::make_unique<jtui::ScrollView>();
scroll->set_frame({0, kTitleBarH, kWidth, kHeight - kTitleBarH});
scroll->set_content(std::move(content));
jtui::ScrollView* raw = scroll.get();
root->append_child(std::move(scroll));
raw->arrange(raw->frame());   // <-- 这一行是必需的
```

ScrollView 目前只支持垂直方向。水平模式在路线图上。

### `ListView`

```cpp
class ListView : public Widget {
public:
    template <typename ItemT, typename BuilderFn>
    void set_items(std::vector<ItemT> items, BuilderFn build);
    // build(const ItemT&) -> unique_ptr<Widget>

    void set_item_height(float height);
    void set_gap(float gap);
};
```

### `FlexBox`

```cpp
enum class FlexDirection : uint8_t { Row, Column };
enum class JustifyContent : uint8_t { Start, Center, End, SpaceBetween, SpaceAround };
enum class AlignItems : uint8_t { Start, Center, End, Stretch };

class FlexBox : public Widget {
public:
    void set_direction(FlexDirection dir);
    void set_gap(float gap);
    void set_padding(float pad);
    void set_justify(JustifyContent justify);
    void set_align(AlignItems align);
};
```

```cpp
auto row = std::make_unique<jtui::FlexBox>();
row->set_direction(jtui::FlexDirection::Row);
row->set_gap(12);
row->set_justify(jtui::JustifyContent::SpaceBetween);
row->set_align(jtui::AlignItems::Center);
row->set_frame({0, 0, 800, 60});
row->append_child(std::move(logo));
row->append_child(std::move(nav_links));
row->append_child(std::move(cta));
```

---

## 9. widgets/common（通用 widget）

精致可交互 widget 的目录。全部继承 `Widget` 并遵循主题。

### `Button`

```cpp
enum class ButtonShape : uint8_t {
    Default,   // 按尺寸级别决定圆角
    Pill,      // corner = height/2
    Circle,    // 仅图标按钮，宽高相等
    Square,    // 按尺寸级别决定圆角
};

class Button : public Widget {
public:
    Button() = default;
    explicit Button(std::string text);

    void set_text(std::string text);
    void set_shape(ButtonShape shape);
    void set_corner_radius(float radius);  // 覆盖 shape 默认值

    // idle / hover / pressed / 文字 颜色
    void set_colors(Color idle, Color hover, Color pressed, Color text);

    void set_border(Color color, float thickness = 1.0F);
    void clear_border();

    void set_font_size(float size, bool bold = false);

    void set_leading_codicon(std::string name);
    void set_trailing_codicon(std::string name);
    void clear_leading_codicon();
    void clear_trailing_codicon();

    [[nodiscard]] Signal<>& on_clicked();
    [[nodiscard]] Signal<bool>& on_hover_changed();
};
```

```cpp
auto cta = std::make_unique<jtui::Button>("Get Started");
cta->set_shape(jtui::ButtonShape::Pill);
cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
cta->set_font_size(14.0F, /*bold=*/true);
cta->set_leading_codicon("play");
cta->set_frame({40, 200, 160, 44});
cta->on_clicked().connect([]() { /* ... */ });
```

### `TextInput`

```cpp
class TextInput : public Widget {
public:
    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const noexcept;

    void set_placeholder(std::string placeholder);
    void set_password(bool is_password);  // 字符显示为 *

    void set_max_length(int max);

    [[nodiscard]] Signal<std::string_view>& on_text_changed();
    [[nodiscard]] Signal<>& on_submit();          // 回车键
};
```

### `Switch`

```cpp
class Switch : public Widget {
public:
    void set_checked(bool checked);
    [[nodiscard]] bool checked() const noexcept;

    void set_label(std::string label);

    [[nodiscard]] Signal<bool>& on_changed();
};
```

### `Checkbox`

```cpp
class Checkbox : public Widget {
public:
    void set_checked(bool checked);
    [[nodiscard]] bool checked() const noexcept;
    void set_label(std::string label);
    [[nodiscard]] Signal<bool>& on_changed();
};
```

### `Tabs`

```cpp
class Tabs : public Widget {
public:
    void set_items(std::vector<std::string> labels);
    void set_active_index(int index);
    [[nodiscard]] int active_index() const noexcept;

    [[nodiscard]] Signal<int>& on_index_changed();
};
```

```cpp
auto tabs = std::make_unique<jtui::Tabs>();
tabs->set_items({"Buttons", "Inputs", "Gauges", "Video", "Audio"});
tabs->set_active_index(0);
tabs->on_index_changed().connect([](int idx) {
    // 显示 section[idx]，通过 set_visible 隐藏其他
});
```

### `Dialog`

简单的标题 + 消息 + 确认按钮 modal。如需更丰富的内容，使用 `Panel` + 业务 widget + scrim Button 组合（参见 [`快速上手`](getting-started.zh-CN.md) 中的 `Modal` 范式），或使用 `hui::install_about_card`（见下文 AboutCard 一节）。

```cpp
class Dialog : public Widget {
public:
    Dialog(std::string title, std::string message);

    void set_title(std::string title);
    void set_message(std::string message);
    void set_confirm_text(std::string text);
    void set_open(bool open);
    [[nodiscard]] bool open() const noexcept;
    void set_modal(bool modal);
};
```

### `Popover`

挂靠在锚点附近打开的浮动 panel。非模态、无 scrim —— 关闭由调用方负责。

```cpp
class Popover : public Widget {
public:
    void set_content(std::unique_ptr<Widget> content);
    void set_open(bool open);
    [[nodiscard]] bool open() const noexcept;
    [[nodiscard]] Signal<bool>& on_open_changed();
};
```

### `Tooltip`

hover 驱动的浮动气泡。调用方需要把锚点的 `on_hover_changed` 连到 tooltip。

```cpp
class Tooltip : public Widget {
public:
    void set_text(std::string text);
    void set_anchor_rect(RectF rect);
    void set_show_delay_ms(int ms);
    void set_hovered(bool hovered);   // 从锚点的 on_hover_changed 调用
};
```

### `Slider`

```cpp
class Slider : public Widget {
public:
    void set_range(float min, float max);
    void set_step(float step);
    void set_value(float value);
    [[nodiscard]] float value() const noexcept;

    [[nodiscard]] Signal<float>& on_value_changed();
};
```

### `ProgressBar`

```cpp
class ProgressBar : public Widget {
public:
    void set_progress(float p);  // 0..1
    void set_indeterminate(bool indeterminate);
    void set_height(float height);
};
```

### Gauge 系列

`Gauge`（线性）、`RadialGauge`（整圆）、`SemiGauge`（半圆）。通用 API：

```cpp
class Gauge : public Widget {
public:
    void set_value(float v);          // 0..1，或用 set_range
    void set_range(float min, float max);
    void set_tone(theme::Tone tone);  // Accent, Success, Warning, Danger
    void set_label(std::string label);
};
```

### `Toolbar` / `Dropdown` / `SearchInput`

```cpp
class Toolbar : public Widget {
public:
    void set_orientation(Orientation o);   // Horizontal / Vertical
    void add_item(std::unique_ptr<Widget> item);
};

class Dropdown : public Widget {
public:
    void set_items(std::vector<std::string> options);
    void set_selected_index(int index);
    [[nodiscard]] Signal<int>& on_changed();
};

class SearchInput : public Widget {
public:
    void set_placeholder(std::string placeholder);
    [[nodiscard]] Signal<std::string_view>& on_query_changed();
};
```

### 装饰类 widget

`Chip`、`Badge`、`Card`、`Avatar`、`FolderCard`、`MetricCard` —— 具体 API 见 `widgets/common/include/hui/widgets/common/` 下各自的头文件。

### `SidebarNav`

```cpp
struct NavItem {
    std::string label;
    std::string codicon;
};

class SidebarNav : public Widget {
public:
    void set_user(std::string name);
    void set_user_status(AvatarStatus status);
    void set_main_items(std::vector<NavItem> items);
    void set_secondary_items(std::vector<NavItem> items);
    void set_active_index(int index);
    [[nodiscard]] Signal<int>& on_index_changed();
};
```

### `AboutCard`（框架级）

开箱即用的 About modal，包含富内容（jtUI logo + 9 个示例列表 + 技术栈 chip + meta + 愿景 + 关闭按钮）。见 `widgets/common/include/hui/widgets/common/about_card.hpp`。

```cpp
namespace hui {

struct AboutColors {
    Color scrim, bg_panel, border, border_strong;
    Color text_strong, text_primary, text_secondary, text_muted;
    Color accent, accent_hover, accent_pressed, accent_soft, accent_fg;
};

template <typename PaletteT>
[[nodiscard]] AboutColors palette_to_about(const PaletteT& p);
// 将字段名匹配的任意品牌 Palette struct 映射为 AboutColors。

void register_about_i18n();
// 注入 26 条 i18n 条目（about.* + nav.about）。幂等；可在 app 启动时
// 安全地调用一次。

[[nodiscard]] Panel* install_about_card(
    Panel& root,
    bool open,
    const AboutColors& colors,
    std::function<void()> on_close,
    float window_w, float window_h);
// 返回容器 Panel 的裸指针。通过 container.set_visible(true/false)
// 显示/隐藏。关闭按钮 & scrim 点击会先内部 set_visible(false)，
// 再调用 on_close。

}
```

```cpp
// 在品牌示例中的用法（rebuild 模式）：
hui::Panel* about = hui::install_about_card(
    *root, s_about_open,
    hui::palette_to_about(brand::active()),
    [rebuild]() { s_about_open = false; rebuild(); },
    kWindowWidth, kWindowHeight);
```

---

## 10. widgets/media（媒体 widget）

### `VideoPlayer`

通过 Windows Media Foundation 播放 H.264 / AAC，GPU-decoded 帧写入 D2D bitmap atlas。

```cpp
class VideoPlayer : public Widget {
public:
    enum class PlayState { Stopped, Playing, Paused };

    VideoPlayer();
    ~VideoPlayer();

    bool set_source(const std::string& path);

    void play();
    void pause();
    void stop();
    void seek(double seconds);

    [[nodiscard]] PlayState state() const noexcept;
    [[nodiscard]] double position() const noexcept;
    [[nodiscard]] double duration() const noexcept;

    [[nodiscard]] const TextureHandle& texture() const noexcept;
    [[nodiscard]] Signal<PlayState>& on_state_changed();

    void on_click(PointF point) override;  // 内置 play/pause 切换
};
```

```cpp
auto video = std::make_unique<jtui::VideoPlayer>();
video->set_source("intro.mp4");
video->play();
video->set_frame({40, 80, 720, 405});
```

**跨 rebuild 持久化**：VideoPlayer 持有 decoder + WASAPI 输出状态。rebuild widget 树会销毁/重建该 widget，从而中断播放。请用 `release_child<VideoPlayer>` 在多次 rebuild 之间保留它 —— 范式见 `examples/jtui_studio/main.cpp` 里的 `PersistentWidgets`。

### `AudioPlayer`

通过 WASAPI 共享模式播放 MP3 / AAC。

```cpp
class AudioPlayer : public Widget {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool set_source(const std::string& path);

    void play();
    void pause();
    void stop();
    void seek(double seconds);

    [[nodiscard]] double position() const noexcept;
    [[nodiscard]] double duration() const noexcept;
    [[nodiscard]] const std::vector<float>& peaks() const noexcept;  // 给 WaveformView 用

    [[nodiscard]] Signal<double>& on_position_changed();
};
```

### `WaveformView`

根据预计算的 peaks 数组绘制音频波形。

```cpp
class WaveformView : public Widget {
public:
    void set_peaks(const std::vector<float>& peaks);
    void set_progress(float progress);     // 0..1
    void set_tone(theme::Tone tone);
};
```

### `Timeline`

```cpp
class Timeline : public Widget {
public:
    void set_duration(double seconds);
    void set_playhead(double seconds);

    template <typename T>
    void bind_position_source(RealtimeSource<T>& source);

    void add_marker(double seconds, std::string label);

    [[nodiscard]] Signal<double>& on_seek();
};
```

### `LevelMeter`

单声道 [0,1] 电平表，带 peak hold + 衰减。绿 / 黄 / 红 三档。

```cpp
class LevelMeter : public Widget {
public:
    void set_value(float v);              // 0..1
    void set_orientation(Orientation o);  // Horizontal / Vertical
    void set_peak_hold_ms(int ms);

    template <typename T>
    void bind_source(RealtimeSource<T>& source);
};
```

---

## 11. 媒体抽象

支撑 `VideoPlayer` / `AudioPlayer` 的底层接口。实现它们即可加入新的封装/编解码支持。

```cpp
namespace hui {

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    [[nodiscard]] virtual bool open(const std::string& path) = 0;
    [[nodiscard]] virtual VideoFrame next_frame() = 0;
    [[nodiscard]] virtual double duration() const noexcept = 0;
    virtual void seek(double seconds) = 0;
};

class IAudioDecoder { /* 同理 */ };

struct VideoFrame {
    std::vector<uint8_t> pixels;   // BGRA8
    int width{0}, height{0};
    double pts{0.0};
};

class TextureHandle {
public:
    [[nodiscard]] uint64_t id() const noexcept;  // 跨 frame 稳定
    [[nodiscard]] PixelBuffer current() const noexcept;
};

struct PixelBuffer {
    std::shared_ptr<std::vector<uint8_t>> pixels;
    int width{0}, height{0};
    int stride{0};
};

}
```

---

## 12. 几何类型

```cpp
namespace hui {

struct PointF {
    float x{0.0F}, y{0.0F};
};

struct SizeF {
    float width{0.0F}, height{0.0F};
};

struct RectF {
    float x{0.0F}, y{0.0F}, width{0.0F}, height{0.0F};

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool contains(PointF point) const noexcept;
    [[nodiscard]] PointF center() const noexcept;
};

}
```

---

## 13. 公开 C API（FFI）

供其他语言绑定使用。位于 `ffi/c_api/include/hui/c_api.h`。

```c
hui_application_t* hui_application_create(void);
void hui_application_destroy(hui_application_t* app);
int  hui_application_run(hui_application_t* app);

hui_window_t* hui_application_create_window(
    hui_application_t* app, const char* title, float w, float h);

void hui_window_set_visible(hui_window_t* window, int visible);
void hui_window_invalidate(hui_window_t* window);
int  hui_application_window_count(hui_application_t* app);

/* ... 完整 C 接口见 hui/c_api.h ... */
```

C API 只是 C++ runtime 之上的一层薄壳 —— 大多数 jtUI 用户应直接使用 C++ 的 `hui::` / `jtui::` namespace。

---

## 参见

- [快速上手](getting-started.zh-CN.md) —— 分步教程
- [架构文档](architecture.zh-CN.md) —— 内部机制深入
- [README](../README.md) —— 项目概览

源码位于 `runtime/`（平台中立的核心）、`platform/win32/`（Windows 后端）、`widgets/{basic,common,media}/`（widget 目录）以及 `api/include/jtui/jtui.hpp`（总入口头文件）。
