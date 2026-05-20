# jtUI API Reference

Complete API surface of jtUI v1.0.0. Public symbols live under the `hui::` namespace, aliased through `jtui::` via the umbrella header `#include "jtui/jtui.hpp"`.

For learning the framework start with [`getting-started.md`](getting-started.md). For internals see [`architecture.md`](architecture.md).

[中文版](api-reference.zh-CN.md)

---

## Table of contents

1. [Foundation](#1-foundation) — `Color`, `i18n`, `log`, codicon
2. [Object model](#2-object-model) — `Node`, `Element`, `Widget`, `DirtyFlags`
3. [Property & signals](#3-property--signals) — `Signal<T>`, `Property<T>`, `RealtimeSource<T>`
4. [Theme](#4-theme) — `Theme`, `SemanticColor`, elevation / spacing / typography tokens
5. [Event](#5-event) — `EventDispatcher`, `MouseEvent`, `KeyEvent`, `TextInputEvent`
6. [Render](#6-render) — `PaintContext`, `DrawCommand`
7. [Application](#7-application) — `Application`, `Window`, `WindowOptions`, `WindowFrame`, `TitleBar`
8. [widgets/basic](#8-widgetsbasic) — `Panel`, `Text`, `CodiconIcon`, `ScrollView`, `ListView`, `FlexBox`
9. [widgets/common](#9-widgetscommon) — `Button`, `TextInput`, `Switch`, `Checkbox`, `Tabs`, `Dialog`, `Popover`, `Tooltip`, `Slider`, `Toolbar`, `Dropdown`, `SearchInput`, `ProgressBar`, `RadialGauge`, `SemiGauge`, `Gauge`, `Chip`, `Badge`, `Card`, `Avatar`, `FolderCard`, `MetricCard`, `SidebarNav`, `AboutCard`
10. [widgets/media](#10-widgetsmedia) — `VideoPlayer`, `AudioPlayer`, `WaveformView`, `Timeline`, `LevelMeter`
11. [Media abstractions](#11-media-abstractions) — `IVideoDecoder`, `IAudioDecoder`, `TextureHandle`, `PixelBuffer`
12. [Geometry types](#12-geometry-types) — `RectF`, `PointF`, `SizeF`
13. [Public C API (FFI)](#13-public-c-api-ffi)

---

## 1. Foundation

### `Color`

RGBA color, 32-bit float per channel, alpha pre-multiplied.

```cpp
namespace hui {
struct Color {
    float r{0.0F}, g{0.0F}, b{0.0F}, a{1.0F};

    [[nodiscard]] static Color from_hex(std::string_view hex);
    // "#RRGGBB" or "#RRGGBBAA". Returns {0,0,0,1} on parse failure.

    [[nodiscard]] static Color from_rgba8(uint8_t r, uint8_t g, uint8_t b,
                                          uint8_t a = 255);
};
}
```

Note: `Color{}` is **opaque black** (a = 1.0), not transparent. Use `Color{0,0,0,0}` for transparent.

```cpp
auto accent  = jtui::Color::from_hex("#FB923C");
auto warning = jtui::Color::from_rgba8(255, 180, 84);  // alpha 255 default
auto glass   = jtui::Color{0.0F, 0.0F, 0.0F, 0.55F};   // semi-transparent
```

### i18n

Two-locale (English / Chinese) string catalog with one-line locale switch.

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

// Signal emitted when locale changes (for widgets that bind to a key).
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

`register_entries` is idempotent — calling with the same `key` overwrites. Safe to call from multiple modules.

### Logging

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

Logs go to stderr and (on Windows) `OutputDebugStringA`. Use `tag` to filter (`"anim"`, `"backdrop"`, `"video"`, etc).

### Codicon lookup

`@vscode/codicons` glyphs are embedded as a TTF in the binary. Lookup by name to get a Unicode code point:

```cpp
namespace hui::foundation {
[[nodiscard]] std::optional<uint32_t> codicon_codepoint_for(std::string_view name);
}
```

```cpp
const auto cp = hui::foundation::codicon_codepoint_for("globe");  // → 0xEB7C
```

Most widgets accept the name directly:

```cpp
button->set_leading_codicon("play");
icon->set_codicon_name("info");
```

See `runtime/foundation/include/hui/foundation/codicon_codepoints.hpp` for the full name table (649 names).

---

## 2. Object model

### `Node` (internal base)

Owns a `std::vector<std::unique_ptr<Node>>` of children. Provides `append_child(std::unique_ptr<Node>)` and the protected `release_child(const Node*) → std::unique_ptr<Node>` operations. Application code rarely deals with `Node` directly — interact via `Widget`.

### `Element` extends `Node`

Adds frame (position + size) and dirty-flag propagation.

```cpp
namespace hui {

enum class DirtyFlags : uint32_t {
    None      = 0,
    Structure = 1 << 0,   // child added/removed/reordered
    Layout    = 1 << 1,   // size/position changed; needs measure+arrange
    Paint     = 1 << 2,   // pixels changed; needs repaint
    Style     = 1 << 3,   // semantic color tokens changed
};

class Element : public Node {
public:
    [[nodiscard]] RectF frame() const noexcept;
    void set_frame(RectF frame) noexcept;          // dirties Layout+Paint

    [[nodiscard]] bool visible() const noexcept;
    void set_visible(bool visible) noexcept;       // dirties Paint

    void invalidate(DirtyFlags flags = DirtyFlags::Paint) noexcept;

    // v1.20: subtree position offset that skips dirty propagation.
    // Use for animation/scroll where you mark dirty once at the container.
    void translate_subtree(float dx, float dy) noexcept;

    void shift_subtree(float dx, float dy) noexcept;  // same but DOES mark dirty
};

}
```

`translate_subtree` vs `set_frame` is a critical performance decision:

- **`set_frame`** — Use for one-shot layout. Auto-marks Layout|Paint dirty on this widget; dirty rect = new position only.
- **`translate_subtree`** — Use for animation/scroll. Mutates `frame_.x/y` without dirty marks; **you must mark dirty on a container widget whose `frame` covers the entire affected region** to avoid ghosting. See [`getting-started.md` § 7](getting-started.md#7-animation) for details.

### `Widget` extends `Element`

The base of every interactive UI building block.

```cpp
namespace hui {

class Widget : public Element {
public:
    [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;

    // ──── State queries ────
    [[nodiscard]] bool enabled() const noexcept;
    void set_enabled(bool enabled) noexcept;

    [[nodiscard]] bool focused() const noexcept;
    void set_focused(bool focused) noexcept;

    [[nodiscard]] bool hovered() const noexcept;
    [[nodiscard]] bool pressed() const noexcept;

    // Whether paint reads pressed(). Default false. Override to true in
    // Button-like widgets to repaint on press.
    [[nodiscard]] virtual bool paint_reacts_to_pressed() const noexcept;

    // ──── Hit testing ────
    [[nodiscard]] virtual bool hit_test(PointF point) const noexcept;
    // Default: visible() && frame().contains(point)

    [[nodiscard]] virtual bool accepts_focus() const noexcept;

    // True intercepts hit_test recursion — `this` widget becomes the leaf
    // even if children also contain the point.
    [[nodiscard]] virtual bool intercepts_hit() const noexcept;

    // ──── Cursor ────
    [[nodiscard]] virtual CursorKind cursor_kind() const noexcept;
    // CursorKind: Default, IBeam, Hand, ResizeH, ResizeV

    // ──── Paint ────
    virtual void paint(PaintContext&) const;
    virtual void paint_overlay(PaintContext&) const;   // after children
    [[nodiscard]] virtual bool clips_self() const noexcept;
    [[nodiscard]] virtual bool clips_children() const noexcept;

    // ──── Events (low-level) ────
    virtual void on_mouse_move(PointF) {}
    virtual void on_mouse_leave() {}
    virtual void on_press_changed(PointF, bool) {}
    virtual void on_click(PointF) {}
    [[nodiscard]] virtual bool on_key_down(Key, bool shift, bool ctrl, bool alt);
    [[nodiscard]] virtual bool on_text_input(char);

    // Low-level event dispatch entry — override for capture/target/bubble.
    virtual bool on_event(Event& ev);

    // ──── Animation ────
    [[nodiscard]] virtual bool tick(float delta);
    // Return true to keep the animation timer running.

    // ──── Layout (override in containers) ────
    [[nodiscard]] virtual SizeF measure(SizeF available) const;
    virtual void arrange(RectF frame_in);

    // ──── Type-safe release_child ────
    template <typename T>
    [[nodiscard]] std::unique_ptr<T> release_child(const T* target) noexcept;
};

}
```

### z-order conventions

**Default z-order is append order — last-appended widget is on top** (paints later + receives hit_test first). There is no explicit `z-index`. To overlay a modal/popover above other widgets, append it last to its parent. See [`widget.hpp:248` comments](../runtime/object/include/hui/object/widget.hpp) for the rationale and gotchas.

### `clips_self` limitations

Returns `true` to ask the runtime to auto-push a `push_clip(self.frame)` rectangle before `paint()`. Useful for "overflow: hidden" semantics on custom widgets that draw beyond their frame.

**Important**: the clip is **rectangular**, not rounded. If your widget has a `corner_radius` and paints geometry near the corners, the geometry inside the rounded corner sector will still render outside the visual circle. Either tighten geometry vertex coordinates to stay away from corners, or use a smaller corner radius. See `examples/jtui_cinema/thumbnail_art.hpp` for a worked-around case.

---

## 3. Property & signals

### `Signal<Args...>`

Callback list with type-erased connections.

```cpp
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;
    using ConnectionId = uint64_t;

    ConnectionId connect(Slot slot);     // returns id for disconnect
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

**Lifetime caveat**: There is no automatic RAII `ScopedConnection`. Lambdas capturing widget raw pointers on long-lived state Signals will dangle if the widget is destroyed. For widget-state interaction prefer the full-tree rebuild pattern — see [`architecture.md`](architecture.md) for the discussion.

### `Property<T>`

A thin wrapper over `Signal<T>` + a current value.

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
dark_mode.set(false);  // emits changed
```

### `RealtimeSource<T>`

A lock-free latest-value producer/consumer pair for high-frequency data (audio levels, video positions, sensor streams). Producer publishes; consumer polls `generation()` to detect updates.

```cpp
template <typename T>
class RealtimeSource {
public:
    void publish(T value);                   // producer side
    [[nodiscard]] T latest() const noexcept; // consumer side
    [[nodiscard]] uint64_t generation() const noexcept;
};

template <typename T>
class LatestValue { /* read-only view of a RealtimeSource<T> */ };

template <typename T>
class RealtimeRingBuffer { /* lock-free SPSC ring */ };
```

`RealtimeCanvas::bind_source<T>(source)` ties a custom paint widget to a `RealtimeSource<T>` — its `tick()` polls `generation()` and marks dirty when the value advances.

---

## 4. Theme

### `Theme` singleton

Global mode + change signal. All widgets read tokens through `theme::colors()` / `theme::elevation()` etc., so a mode switch + repaint re-themes the entire app.

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
    // App-level reaction. Typically just calls rebuild().
});
```

### `SemanticColor` tokens

Looks up themed colors. Each mode (Dark/Light) maps semantic names to base palette swatches.

```cpp
namespace hui::theme {

struct SemanticColor {
    Color bg_base;         // window background
    Color bg_surface;      // card / panel surface
    Color bg_surface_alt;  // alternate surface
    Color bg_raised;       // raised card
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

Most brand examples define their own `brand::Palette` with the same field names plus brand-specific extras (`bg_card`, `bg_card_hover`, etc) — see `examples/jtui_cinema/brand_tokens.hpp` for the canonical pattern.

### Elevation tokens

5 shadow levels matching CSS box-shadow semantics. Backed by `CLSID_D2D1Shadow` on Windows.

```cpp
namespace hui::theme {

struct ElevationLevel {
    PointF offset;   // shadow offset (x, y)
    float  blur;     // gaussian sigma * 3
    float  spread;
    Color  color;
};

struct Elevation {
    ElevationLevel level_0;  // flat
    ElevationLevel level_1;  // button hover
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

### Spacing & typography tokens

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

Per-widget overrides of selected semantic colors, without changing global theme.

```cpp
namespace hui::theme {

class TokenOverride {
public:
    void set_text_strong(Color c);
    void set_accent(Color c);
    // ... one setter per SemanticColor field ...

    // Resolved color: returns override if set, else falls back to global theme.
    [[nodiscard]] Color resolve_text_strong() const noexcept;
};

}
```

Widget subclasses hold `std::unique_ptr<TokenOverride>` and paint via `theme::resolve(override.get())`.

### VSCode color tokens

The `gallery` example includes a `vscode_palette` section that demonstrates the VSCode token catalog (semantic + base swatches). See `runtime/theme/include/hui/theme/vscode_tokens.hpp` for the API. Use these when you want exact VS Code color parity for code-editor style widgets.

---

## 5. Event

### Event types

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
    bool down{true};       // false on key up
    bool autorepeat{false};
};

struct TextInputEvent {
    uint32_t code_point{0};  // Unicode (UTF-32)
};

// Tagged-union over all event kinds for low-level on_event.
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

The static facade through which `Application` routes Win32 messages to widgets.

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

Dispatch runs the **Capture → Target → Bubble** three-phase pipeline. Most user code does not call `EventDispatcher` directly — `Application` does. Override `Widget::on_event(Event&)` to intercept at any phase, or use the high-level virtual hooks (`on_click`, `on_mouse_move`, etc).

---

## 6. Render

### `PaintContext`

The drawing surface backed by an in-memory `DrawCommand` list that gets replayed to D2D each frame.

```cpp
namespace hui {

class PaintContext {
public:
    // ──── Rectangles & ellipses ────
    void fill_rect(RectF bounds, Color color);
    void stroke_rect(RectF bounds, Color color, float thickness = 1.0F);

    void fill_rounded_rect(RectF bounds, Color color, float radius);
    void stroke_rounded_rect(RectF bounds, Color color, float radius,
                              float thickness = 1.0F);

    void fill_ellipse(RectF bounds, Color color);
    void stroke_ellipse(RectF bounds, Color color, float thickness = 1.0F);

    // ──── Paths ────
    void line(PointF start, PointF end, Color color, float thickness = 1.0F);

    void draw_bezier(PointF p0, PointF p1, PointF p2, PointF p3,
                      Color color, float thickness = 1.0F);
    // Cubic bezier. p1/p2 are control points.

    void fill_polygon(std::vector<PointF> points, Color color);
    // Auto-closes (last → first). Points < 3: noop. > 200: backend may clip.

    // ──── Gradients ────
    void fill_gradient_rect(RectF bounds, Color color_top, Color color_bottom,
                             float radius = 0.0F);
    // Vertical linear gradient. radius=0 means non-rounded rectangle.

    // ──── Effects (D2D 1.1) ────
    void fill_shadow(RectF bounds, float corner_radius,
                      PointF offset, float blur, float spread, Color color);
    // CSS box-shadow semantics. Falls back to noop on D2D 1.0.

    void fill_backdrop_blur(RectF bounds, float corner_radius,
                             float blur, Color tint);
    // Frosted-glass effect: CopyFromRenderTarget → GaussianBlur → DrawImage + tint.
    // Falls back to flat tint on D2D 1.0.

    // ──── Text ────
    void draw_text(RectF bounds, std::string text, Color color,
                    TextAlignment alignment = TextAlignment::Leading,
                    float font_size = 14.0F, bool bold = false);

    void draw_text_with_font(RectF bounds, std::string text, Color color,
                              std::string font_family, TextAlignment alignment,
                              float font_size, bool bold);

    // ──── Textures ────
    void draw_texture(RectF bounds, PixelBuffer buffer,
                       AlphaMode alpha_mode = AlphaMode::Premultiplied);

    // ──── Clip ────
    void push_clip(RectF bounds);
    void pop_clip();
};

}
```

### `TextAlignment`

```cpp
enum class TextAlignment : uint8_t { Leading, Center, Trailing };
```

### `DrawCommand` (internal)

The IR every paint() call emits into. Application code does not construct these directly — see `runtime/render/include/hui/render/paint_context.hpp` for the schema if you need to inspect or replay commands.

---

## 7. Application

### `Application`

Owns the message loop and all `Window` instances.

```cpp
namespace hui {

class Application {
public:
    Application();
    ~Application();

    [[nodiscard]] Window& create_window(const WindowOptions& options);

    int run();
    // Blocks on Windows. Returns 0 immediately on non-Windows platforms.
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
    bool        frameless{false};    // true → custom title bar via WindowFrame
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

`set_content` retires the old root into a `pending_destroy_` queue and installs the new one. The old tree is freed after the current event handler unwinds — this is what makes "rebuild from inside a click callback" safe (no use-after-free on the widget that triggered the click).

### `WindowFrame`

Custom title bar chrome (when `WindowOptions::frameless = true`). Drag-to-move + double-click-to-maximize + Win32 resize edges.

```cpp
class WindowFrame : public Widget {
public:
    void set_frameless(bool frameless) noexcept;
};
```

### `TitleBar`

Just the visual title bar text + close/minimize/maximize buttons. Pair with `WindowFrame`.

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

## 8. widgets/basic

### `Panel`

Rectangular container with optional background, border, shadow, backdrop blur, rounded corners. The most-used widget.

```cpp
namespace hui {

enum class PanelRole : uint8_t {
    Base,         // window background
    Surface,      // card surface
    SurfaceAlt,   // alternate surface
    Raised,       // raised surface
};

class Panel : public Widget {
public:
    void set_role(PanelRole role) noexcept;
    void set_background_color(Color color) noexcept;
    void clear_background_color() noexcept;      // revert to theme token

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

**Backdrop blur gotcha**: when `set_backdrop_blur(...)` is enabled, also call `set_background_color({0,0,0,0})` to make the panel transparent — otherwise the opaque background covers the blurred layer.

### `Text`

Renders a single line (or multi-line via `\n`) of text. Supports inline colored runs via `set_runs`.

```cpp
struct TextRun {
    std::string text;
    Color       color{};       // Color{} → "no override", uses widget set_color
    bool        color_explicit{false};

    TextRun() = default;
    explicit TextRun(std::string t);
    TextRun(std::string t, Color c);  // marks color_explicit
};

class Text : public Widget {
public:
    Text() = default;
    explicit Text(std::string content);

    void set_content(std::string content);

    // Inline multi-color: paint each TextRun in sequence horizontally.
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
    jtui::TextRun{"native", p.accent},   // colored run
    jtui::TextRun{". Ship fast."},
});
title->set_frame({40, 200, 600, 60});
```

### `CodiconIcon`

Single VSCode codicon glyph as a widget.

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

Vertical scrollable container. Set content via `set_content(unique_ptr<Widget>)`; the content can be taller than the viewport.

```cpp
class ScrollView : public Widget {
public:
    void set_content(std::unique_ptr<Widget> content);
    [[nodiscard]] Widget* content() const noexcept;

    void set_scroll_offset(PointF offset);
    [[nodiscard]] PointF scroll_offset() const noexcept;

    void set_smooth_scroll(bool smooth) noexcept;

    void arrange(RectF frame_in) override;
    // CALL EXPLICITLY ONCE after appending to parent — see gotcha below.
};
```

**Gotcha**: `Window` only calls `arrange()` on the root. `Panel::arrange` does NOT recurse, so `ScrollView` nested inside a `Panel` never gets arranged → `content_size_` stays 0 → scroll clamps to 0. Manually arrange once after appending:

```cpp
auto scroll = std::make_unique<jtui::ScrollView>();
scroll->set_frame({0, kTitleBarH, kWidth, kHeight - kTitleBarH});
scroll->set_content(std::move(content));
jtui::ScrollView* raw = scroll.get();
root->append_child(std::move(scroll));
raw->arrange(raw->frame());   // <-- this is necessary
```

ScrollView is vertical-only. Horizontal mode is on the roadmap.

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

## 9. widgets/common

The catalog of polished interactive widgets. All inherit `Widget` and respect the theme.

### `Button`

```cpp
enum class ButtonShape : uint8_t {
    Default,   // rounded by size step
    Pill,      // corner = height/2
    Circle,    // for icon-only buttons, set equal w/h
    Square,    // rounded by size step
};

class Button : public Widget {
public:
    Button() = default;
    explicit Button(std::string text);

    void set_text(std::string text);
    void set_shape(ButtonShape shape);
    void set_corner_radius(float radius);  // overrides shape default

    // Idle / hover / pressed / text colors
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
    void set_password(bool is_password);  // displays * for chars

    void set_max_length(int max);

    [[nodiscard]] Signal<std::string_view>& on_text_changed();
    [[nodiscard]] Signal<>& on_submit();          // Enter key
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
    // show section[idx], hide others via set_visible
});
```

### `Dialog`

Simple title + message + confirm button modal. For richer content use a `Panel` + business widgets + scrim Button pattern (see [`getting-started.md`](getting-started.md) for the `Modal` pattern) or `hui::install_about_card` (§ AboutCard below).

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

A floating panel that opens near an anchor. Non-modal, no scrim — caller handles dismiss.

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

Hover-driven floating bubble. Caller wires anchor's `on_hover_changed` to the tooltip.

```cpp
class Tooltip : public Widget {
public:
    void set_text(std::string text);
    void set_anchor_rect(RectF rect);
    void set_show_delay_ms(int ms);
    void set_hovered(bool hovered);   // call from anchor's on_hover_changed
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

### Gauges

`Gauge` (linear), `RadialGauge` (full circle), `SemiGauge` (half circle). Common API:

```cpp
class Gauge : public Widget {
public:
    void set_value(float v);          // 0..1 or use set_range
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

### Decorative widgets

`Chip`, `Badge`, `Card`, `Avatar`, `FolderCard`, `MetricCard` — see each header in `widgets/common/include/hui/widgets/common/` for the specific API.

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

### `AboutCard` (framework-level)

Drop-in About modal with rich content (jtUI logo + 9 examples list + tech-stack chips + meta + vision + close). See `widgets/common/include/hui/widgets/common/about_card.hpp`.

```cpp
namespace hui {

struct AboutColors {
    Color scrim, bg_panel, border, border_strong;
    Color text_strong, text_primary, text_secondary, text_muted;
    Color accent, accent_hover, accent_pressed, accent_soft, accent_fg;
};

template <typename PaletteT>
[[nodiscard]] AboutColors palette_to_about(const PaletteT& p);
// Maps any brand Palette struct (with matching field names) to AboutColors.

void register_about_i18n();
// Inject 26 i18n entries (about.* + nav.about). Idempotent; safe to call once
// per app startup.

[[nodiscard]] Panel* install_about_card(
    Panel& root,
    bool open,
    const AboutColors& colors,
    std::function<void()> on_close,
    float window_w, float window_h);
// Returns container Panel raw ptr. Set container.set_visible(true/false) to
// show/hide. close button & scrim click internally set_visible(false) then
// invoke on_close.

}
```

```cpp
// Usage in a brand example (rebuild mode):
hui::Panel* about = hui::install_about_card(
    *root, s_about_open,
    hui::palette_to_about(brand::active()),
    [rebuild]() { s_about_open = false; rebuild(); },
    kWindowWidth, kWindowHeight);
```

---

## 10. widgets/media

### `VideoPlayer`

H.264 / AAC playback via Windows Media Foundation, GPU-decoded frames to D2D bitmap atlas.

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

    void on_click(PointF point) override;  // built-in play/pause toggle
};
```

```cpp
auto video = std::make_unique<jtui::VideoPlayer>();
video->set_source("intro.mp4");
video->play();
video->set_frame({40, 80, 720, 405});
```

**Cross-rebuild persistence**: VideoPlayer holds decoder + WASAPI output state. Rebuilding the tree destroys/recreates the widget, which interrupts playback. Use `release_child<VideoPlayer>` to preserve it across rebuilds — see `examples/jtui_studio/main.cpp` for the `PersistentWidgets` pattern.

### `AudioPlayer`

MP3 / AAC playback via WASAPI shared mode.

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
    [[nodiscard]] const std::vector<float>& peaks() const noexcept;  // for WaveformView

    [[nodiscard]] Signal<double>& on_position_changed();
};
```

### `WaveformView`

Displays an audio waveform from a precomputed peaks vector.

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

Single-channel [0,1] level meter with peak hold + decay. Green / yellow / red zones.

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

## 11. Media abstractions

Low-level interfaces backing `VideoPlayer` / `AudioPlayer`. Implement these to add new container/codec support.

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

class IAudioDecoder { /* analogous */ };

struct VideoFrame {
    std::vector<uint8_t> pixels;   // BGRA8
    int width{0}, height{0};
    double pts{0.0};
};

class TextureHandle {
public:
    [[nodiscard]] uint64_t id() const noexcept;  // stable across frames
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

## 12. Geometry types

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

## 13. Public C API (FFI)

For binding from other languages. Located in `ffi/c_api/include/hui/c_api.h`.

```c
hui_application_t* hui_application_create(void);
void hui_application_destroy(hui_application_t* app);
int  hui_application_run(hui_application_t* app);

hui_window_t* hui_application_create_window(
    hui_application_t* app, const char* title, float w, float h);

void hui_window_set_visible(hui_window_t* window, int visible);
void hui_window_invalidate(hui_window_t* window);
int  hui_application_window_count(hui_application_t* app);

/* ... see hui/c_api.h for the complete C surface ... */
```

The C API is a thin shell over the C++ runtime — most jtUI users should prefer the C++ `hui::` / `jtui::` namespace directly.

---

## See also

- [Getting Started](getting-started.md) — Step-by-step tutorial
- [Architecture](architecture.md) — Internals deep dive
- [README](../README.md) — Project overview

Source code lives under `runtime/` (platform-neutral core), `platform/win32/` (Windows backend), `widgets/{basic,common,media}/` (widget catalog), and `api/include/jtui/jtui.hpp` (umbrella header).
