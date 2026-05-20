# jtUI Architecture

How jtUI works under the hood. Read this when you want to write framework-level widgets, contribute to the runtime, or simply understand why a specific pattern (full-tree rebuild, `translate_subtree`, ...) is the canonical approach.

For application-level usage start with [`getting-started.md`](getting-started.md). For the public API surface see [`api-reference.md`](api-reference.md).

[中文版](architecture.zh-CN.md)

---

## Table of contents

1. [Module layout & dependency direction](#1-module-layout--dependency-direction)
2. [Rendering pipeline](#2-rendering-pipeline)
3. [Dirty propagation & partial repaint](#3-dirty-propagation--partial-repaint)
4. [Event dispatch (capture–target–bubble)](#4-event-dispatch-capture--target--bubble)
5. [Theme tokens & resolution](#5-theme-tokens--resolution)
6. [Animation timer & `tick()`](#6-animation-timer--tick)
7. [Full-tree rebuild as state model](#7-full-tree-rebuild-as-state-model)
8. [Z-order & hit testing](#8-z-order--hit-testing)
9. [Cross-platform model](#9-cross-platform-model)
10. [Performance notes](#10-performance-notes)
11. [Extension points](#11-extension-points)

---

## 1. Module layout & dependency direction

jtUI is split into three layers, each in its own directory tree. Higher layers depend on lower; nothing in `runtime/` depends on anything in `platform/` or `widgets/`.

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

Within `runtime/`, the order from bottom up is: `foundation` → `object` → `property` → `theme` → `layout` → `event` → `render` → `media` → `app`. Each module exports a CMake target (`hui::foundation`, `hui::object`, ...) with no cycles.

`api/include/jtui/jtui.hpp` is the umbrella header that brings every public type into the `jtui::` namespace via aliases. Application code includes just this one header. `widgets/common/include/hui/widgets/common/about_card.hpp` (added in v1.4) is the canonical example of a framework-level widget that lives outside the `api/` umbrella but is still consumed via `jtui::install_about_card`.

---

## 2. Rendering pipeline

jtUI separates "what to draw" (`PaintContext`) from "how to draw it" (the platform backend). This is what enables the Linux stub mode + future macOS/Skia backends without touching widget code.

### 2.1 PaintContext → DrawCommand IR

Each `Widget::paint(PaintContext&)` call appends `DrawCommand` records to a `std::vector<DrawCommand>` inside `PaintContext`. A `DrawCommand` is a tagged union:

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

Widgets never call D2D directly. They only emit commands. The backend replays the command list into the platform render target.

### 2.2 Backend replay (`platform/win32`)

`runtime/app/src/application.cpp` (line ~1438) holds `paint_widget_tree(const Widget& widget, PaintContext& context, RectF clip_rect)` which:

1. Returns early if the widget is `!visible()` or its `frame` falls outside `clip_rect`.
2. If `clips_self()` is true: emits `PushClip(intersect(parent_clip, self.frame))`.
3. Calls `widget.paint(context)`.
4. Recurses into children with the updated clip.
5. If `clips_self()` was true: emits `PopClip`.
6. Calls `widget.paint_overlay(context)` outside the clip (lets widgets draw "on top" of their children).

Once `paint_widget_tree` has filled the command list, `replay_paint_commands` walks the list and translates each `DrawCommand` to the corresponding D2D 1.1 / DirectWrite call.

The Win32 backend uses a double-buffered memory DC + D2D `ID2D1DCRenderTarget`. Big primitives like backdrop blur and box shadow go through `CLSID_D2D1GaussianBlur` / `CLSID_D2D1Shadow` Effects (a `ID2D1DeviceContext` queried from the render target on D2D 1.1+).

### 2.3 DPI awareness

All `DrawCommand` coordinates are in **logical pixels** (DPI-independent). At replay time the backend reads `window.dpi_scale` and multiplies geometry by it before issuing D2D calls. The `scaled_command` helper in `application.cpp` scales bounds, end_point, polygon.points, bezier_c1/c2, shadow_offset, font_size — anything carrying physical extent.

If you add a new `DrawCommandKind` that carries a `PointF` field, **you must update `scaled_command` to multiply it by the DPI scale**, otherwise HiDPI users will see misaligned geometry on that command but correct geometry elsewhere. (This was a real bug fixed in v1.22.1.)

---

## 3. Dirty propagation & partial repaint

jtUI does not redraw the whole window every frame. It tracks per-widget dirty flags and only repaints widgets that are actually invalidated, then unions their frames into the smallest possible dirty rectangle for D2D.

### 3.1 `DirtyFlags`

```cpp
enum class DirtyFlags : uint32_t {
    None      = 0,
    Structure = 1 << 0,   // children list mutated
    Layout    = 1 << 1,   // size/position changed
    Paint     = 1 << 2,   // pixels changed
    Style     = 1 << 3,   // theme tokens changed
};
```

Bitwise combinable. `set_frame` flips Layout|Paint. `set_visible` flips Paint. `theme::Theme::set(...)` flips Style on every widget.

### 3.2 The subtree-dirty summary

Naively, finding "all dirty widgets" requires walking the whole tree on every paint pass — O(N) per frame, expensive when N > 1000. jtUI keeps a per-widget **subtree-dirty summary** that is the bitwise OR of its own `dirty_flags_` and the summaries of all its children. When any leaf widget marks itself dirty, the bit propagates up to the root through `propagate_subtree_dirty(parent, flags)`. Repaint then descends only into branches whose summary is non-zero.

```cpp
void Element::mark_dirty(DirtyFlags flags) noexcept {
    if (flags == DirtyFlags::None) return;
    dirty_flags_ |= flags;
    propagate_subtree_dirty(parent(), flags);
}

// Recursive: ORs into parent's subtree_dirty_, stops early if parent already
// covers this flag (no redundant walk).
void Element::propagate_subtree_dirty(Node* node, DirtyFlags flags) noexcept {
    while (node != nullptr) {
        Element* e = dynamic_cast<Element*>(node);
        if (e == nullptr) return;
        if ((e->subtree_dirty_ & flags) == flags) return;   // early stop
        e->subtree_dirty_ |= flags;
        node = e->parent();
    }
}
```

This early-stop optimization keeps the propagate cost ~O(depth) instead of O(siblings).

### 3.3 Partial repaint

`invalidate_dirty_window` computes the AABB of every dirty widget's frame (skipping clean subtrees thanks to the summary) and only invalidates that rectangle in Win32:

```cpp
RectF dirty_bounds{};
if (dirty_bounds_widget_tree(*content, DirtyFlags::Paint, dirty_bounds)) {
    InvalidateRect(hwnd, &dirty_bounds, FALSE);
}
```

`WM_PAINT` then redraws only the dirty rect. For typical UIs this means hover/press transitions repaint a single button instead of the full window.

### 3.4 The `set_frame` vs `translate_subtree` tradeoff

This is the most important pattern to internalize when writing animations:

- **`set_frame(new_rect)`** triggers `mark_dirty(Layout|Paint)`. The dirty rect = new position. The old position's pixels are not in the dirty rect — D2D won't repaint them, so they stay on screen → ghosting.

- **`translate_subtree(dx, dy)`** mutates `frame_.x/y` directly without marking dirty. **You must mark dirty on a container widget whose `frame` covers the entire affected region** so the partial repaint redraws old + new together.

`ScrollView` and `CarouselAnimator` both use this pattern. See `widgets/basic/src/scroll_view.cpp:155` for ScrollView's scroll callback and `examples/jtui_cinema/carousel_animator.hpp` for the carousel slide.

---

## 4. Event dispatch (capture–target–bubble)

jtUI's event model mirrors DOM events: every pointer / key / text event traverses the widget tree in three phases.

### 4.1 The three phases

```
   root  ──→  panel  ──→  card  ──→  button     (Capture: top-down)
                                       ↓
                                  on_event()
                                       ↓
   root  ←──  panel  ←──  card  ←──  button     (Bubble: bottom-up)
```

1. **Capture**: from root to target, each widget sees `on_event(Event{phase=Capture})`. Container widgets can intercept here.
2. **Target**: only the leaf hit widget sees `on_event(Event{phase=Target})`. This is where the high-level virtual hooks (`on_click`, `on_press_changed`, etc.) are normally invoked.
3. **Bubble**: from target back to root, each ancestor sees `on_event(Event{phase=Bubble})`. Set `ev.handled = true` to stop propagation.

### 4.2 Hit testing

`hit_test(PointF)` decides which leaf is the target. Default:

```cpp
bool Widget::hit_test(PointF point) const noexcept {
    return visible() && frame().contains(point);
}
```

Walk recurses children **back-to-front** (last child checked first → last-painted is hit first → matches z-order convention). The first widget returning true becomes the target.

Set `intercepts_hit()` to `true` in a container to claim the hit even when a child would also accept it (used by `ScrollView` so the thumb gets press events instead of inner content).

### 4.3 `EventState`

Persistent state across messages:

```cpp
struct EventState {
    Widget* hovered{nullptr};
    Widget* pressed{nullptr};
    Widget* focused{nullptr};
};
```

`Application` maintains a per-window `EventState` and passes it into `EventDispatcher` calls. On every event, the dispatcher first **sanitizes** the state — if a `Widget*` is no longer in the tree (rebuild + free), it's nulled. This is what prevents UAF on the rebuild-from-click pattern.

### 4.4 Focus

`Widget::accepts_focus()` controls whether `Tab` / direct focus assignment lands on a widget. Default: `enabled()`. `Button` / `TextInput` / `Switch` / `Slider` etc. follow this default. Non-interactive widgets (`Text`, `Panel`, `CodiconIcon`) override to `false`.

`Tab` / `Shift+Tab` traversal walks the tree in document order, skipping non-focusable widgets.

---

## 5. Theme tokens & resolution

`Theme` is a global singleton. All widgets read tokens through `theme::colors()` / `theme::elevation()` / etc. — no per-widget palette state by default.

### 5.1 SemanticColor mapping

Each `ThemeMode` (Dark / Light) maps semantic names to base palette swatches. The mapping is defined in `runtime/theme/src/theme.cpp`:

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

Brand examples define their own `brand::Palette` and `brand::active()` for example-specific colors (`bg_card`, `bg_card_hover`, etc.), but the framework-level `SemanticColor` is what built-in widgets (`Button`, `Tabs`, `Dialog`) read.

### 5.2 `TokenOverride`

For per-widget customization without flipping global theme:

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

`theme::resolve(override)` returns `*override` if the field is set, else falls back to global `theme::colors()`. This is how brand `Button::set_colors(...)` works without leaking per-instance state into a global token.

### 5.3 Mode switching

`Theme::set(ThemeMode::Light)` emits `Theme::on_changed()`. Application code typically connects this to a full-tree rebuild:

```cpp
jtui::theme::Theme::on_changed().connect([rebuild](auto) {
    rebuild();
});
```

The full-tree rebuild re-reads every widget's colors from the new theme. Alternatively connect to a `root.invalidate(DirtyFlags::Paint)` for a paint-only refresh (works because semantic color tokens are read on every paint).

---

## 6. Animation timer & `tick()`

jtUI has one shared 60 fps `WM_TIMER` per window. When any widget in the tree returns `true` from `tick(delta)`, the timer runs; when none do, the timer is stopped to save CPU.

### 6.1 The tick loop

```cpp
// application.cpp ~ line 2211
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

`tick_widget_tree(widget, delta)` recurses, calls `widget.tick(delta)` on each, ORs the return values. If anything returned `true`, the timer survives this frame.

### 6.2 Synchronous tick on press/release

To eliminate the ~16 ms latency between `WM_LBUTTONDOWN` and the first `WM_TIMER` firing, `WM_LBUTTONDOWN` and `WM_LBUTTONUP` each run **one synchronous tick before the synchronous paint**:

```cpp
case WM_LBUTTONUP: {
    EventDispatcher::dispatch_pointer_up(content, point, state, activation);
    if (activation) activate_widget(window, activation, point);

    ensure_animation_timer(window);
    tick_widget_tree(*content, 1.0F / 60.0F);   // <-- synchronous first tick
    repaint_dirty_window_now(window, FALSE, false);
}
```

This is great for button feedback (the visual "pressed" state appears instantly) but has an unobvious consequence for animations:

- **Spring-style animations** (`step = diff * k`) push a huge step on frame 1 when `diff` is large — a 760 px carousel slide jumps 72 px on frame 1, looking like "teleport then slide".
- **Progress + easing animations** (`step = lerp(start, target, eased(progress))`) have a near-zero step on frame 1 because `eased(0.02) ≈ 0.0001` — they animate smoothly from rest.

For carousel-style "move from A to B" animations always prefer progress + ease-in-out. See `examples/jtui_cinema/carousel_animator.hpp` for the reference implementation.

### 6.3 Real wall-clock `delta`

`compute_tick_delta` uses `QueryPerformanceCounter`, not a hardcoded `1/60`. This matters for audio-locked animation (e.g. waveform progress) — if the message loop drops frames, the next tick advances by 2/60 instead of 1/60, keeping the visual in sync with the WASAPI clock.

---

## 7. Full-tree rebuild as state model

jtUI's recommended state model is "keep state in a struct, write `build_root(state) → unique_ptr<Panel>`, replace the entire tree on every state change". This sounds heavyweight but is fast in practice and avoids a class of bugs that finer-grained reactivity introduces.

### 7.1 Why it's not slow

- A typical screen has a few hundred widgets; constructing `Panel`/`Text`/`Button` is cheap (no D2D allocation, no GPU upload — just `std::make_unique` and `append_child`).
- The partial-repaint dirty-rect logic still applies: even though every widget is freshly constructed, the dirty union covers only the regions that visually changed (background color, text content), so D2D only redraws those regions.
- Profiling shows ~0.5–2 ms per rebuild for a moderately complex hero page on a modern Windows desktop.

### 7.2 Why it's safe even from click callbacks

A naive "rebuild from inside a click handler" would UAF: the click came from a button about to be destroyed by `window.set_content(new_root)`. jtUI handles this with **deferred destruction**:

```cpp
// Window::set_content
void Window::set_content(std::unique_ptr<Widget> content) {
    if (content_) pending_destroy_.push_back(std::move(content_));
    content_ = std::move(content);
    mark_dirty(DirtyFlags::Structure);
}

// Application drains pending_destroy_ at the bottom of each message loop iter:
void Application::drain_pending_destroy(Window& window) {
    window.pending_destroy_.clear();
}
```

The button's `on_clicked` lambda finishes unwinding the call stack before its parent widget is actually freed. This is what makes the `state.foo = ...; rebuild();` pattern safe.

### 7.3 Preserving heavyweight widgets

`VideoPlayer` / `AudioPlayer` / `WaveformView` hold decoder + WASAPI state. Destroying them on rebuild would interrupt playback. The pattern is `release_child<T>` + a `PersistentWidgets` struct held in the app scope:

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

Each rebuild "rescues" the persistent widget out of the old tree before `set_content` replaces it, then re-attaches it to the new tree. See `examples/jtui_studio/main.cpp` for the canonical pattern.

### 7.4 Preserving animation progress

Animation widgets are recreated on rebuild, which resets `progress_ = 0.0F`. To preserve animation state across rebuilds (e.g. carousel offset, scroll position), keep the state in your `AppState` struct and have the animator write back via a callback:

```cpp
animator->set_on_current_changed([&state](float new_current) {
    state.carousel_offset = new_current;
});
animator->set_offsets(state.carousel_offset, state.carousel_target);
```

The next rebuild's new animator reads `state.carousel_offset` and continues from there. See `examples/jtui_cinema/carousel_animator.hpp` for the full pattern.

---

## 8. Z-order & hit testing

**There is no explicit z-index API.** Z-order = append order, latest-appended-on-top. This is the most important convention to internalize when laying out widgets.

### 8.1 Implications

- Modal scrim + popover must be the **last** child appended to root.
- Background decorations (grid, watermark) must be the **first** children appended.
- An overlay button placed in the same X/Y region as another widget needs to be appended **after** the widget it should sit on top of — otherwise the underlying widget steals click via hit_test.

### 8.2 Common gotchas

- **`folders_app` shipped a buggy About button** because the navbar's right-side links (`Features` / `Pricing` Text widgets) were appended after the About button at overlapping X. Click on the About button hit the Text widget instead (which doesn't consume click but doesn't pass it through either by default). Fix: move the Text widgets out of the X overlap, or append them before the button.
- **`jtui_pro` shipped a buggy About button** because it already had a `bg_btn` (background-mode toggle) at the exact same X formula. Z-order placed `bg_btn` over `about_btn` completely. Fix: shift the About button X formula one slot left.



If you find yourself needing explicit z-index, file an issue — it's on the backlog but not implemented today.

---

## 9. Cross-platform model

jtUI today is **Windows-only at runtime**. Linux and macOS are supported for cross-compile + unit tests only.

### 9.1 Why Windows-only

The render and media backends are deeply tied to Microsoft APIs:

- **Direct2D 1.1** for hardware-accelerated 2D + Effects (`CLSID_D2D1Shadow`, `CLSID_D2D1GaussianBlur`)
- **DirectWrite** for text shaping + emoji color font + custom `IDWriteFactory5` font collection (codicons)
- **Media Foundation** for video decode (H.264 in MP4)
- **WASAPI shared mode** for audio output

There is no GTK / Cocoa / SDL fallback today.

### 9.2 Linux build

`Application::run()` returns 0 immediately on non-Windows. The render commands aren't replayed (no D2D target), but the widget tree still builds and pixel-free unit tests work.

`platform/win32/CMakeLists.txt` is gated `if(WIN32)`. Linux native builds skip it. Cross-compiling with MinGW-w64 enables `_WIN32` and pulls platform/win32 in.

### 9.3 Roadmap: SDL3 + Skia

A future `platform/skia/` will offer the same backend interface but implemented in Skia + SDL3, enabling Linux native + macOS. The widget catalog and runtime are designed to be backend-agnostic — `PaintContext` produces an IR, only the replay phase is platform-specific.

---

## 10. Performance notes

### 10.1 Frame budget

- Idle (no animation): no CPU. `WM_TIMER` is stopped.
- Animation running: ~0.1–1 ms per `tick_widget_tree` pass on a modern desktop for a typical hero page (~200 widgets).
- Full-tree rebuild: ~0.5–2 ms (mostly `std::make_unique<Widget>` calls).
- D2D replay: ~1–4 ms depending on the dirty rect and number of Effects (Shadow / GaussianBlur).

A 60 fps animation in a typical brand example uses ~15–25 % of one CPU core on a 2024-era laptop.

### 10.2 GPU effect caching

`CLSID_D2D1Shadow` and `CLSID_D2D1GaussianBlur` Effect objects are expensive to create (~10–30 ms cold). They are cached on `DirectTextSession` (one per render target) and reused across frames. Each Effect has its parameters set per command, then drawn.

The first Effect-using frame on a new render target has a cold-start penalty (visible as a stutter on theme switch); subsequent frames are smooth.

### 10.3 Avoid frequent `set_frame` in animation

As covered in § 3.4 — `set_frame` triggers `mark_dirty(Layout|Paint)` and parent propagation. Doing this 60 times per second for many widgets is wasteful. Prefer `translate_subtree` + a single container-level dirty mark.

### 10.4 Text rendering is the slowest primitive

DirectWrite layout is cached internally but issuing `DrawText` is still ~10× slower per command than `FillRect`. For widgets with many small text labels (e.g. axis ticks on a chart), consolidating into fewer `set_runs` calls (multi-segment in one Text widget) is faster than many separate `Text` widgets.

### 10.5 Animation throttling: WM_MOUSEWHEEL

`WM_MOUSEWHEEL` events can flood the queue during fast scrolling and starve `WM_TIMER`. v1.24 mitigates this by directly ticking inside the `WM_MOUSEWHEEL` handler when ScrollView is the wheel target — see `application.cpp` ~line 2152.

---

## 11. Extension points

### 11.1 Custom widgets

Inherit from `hui::Widget`, override `paint(PaintContext&)`, optionally `tick(float)`, `hit_test(PointF)`, the event hooks, and `type_name()`. See `examples/jtui_cinema/thumbnail_art.hpp` (paint-heavy) and `examples/jtui_invest/animated_widgets.hpp` (tick-heavy) for two complete examples.

Custom widgets compile against the public `hui::` / `jtui::` headers — no need to modify framework source.

### 11.2 Custom decoder

Implement `IVideoDecoder` or `IAudioDecoder` (see `runtime/media/include/hui/media/decoder.hpp`) and register it through a factory. The default Windows factory returns `MFVideoDecoder` / `WasapiAudioOutput`; a custom factory can substitute these. Use this to add codecs MF doesn't ship (VP9 in MKV, AV1) or to use a different decoder backend (libav, GStreamer).

### 11.3 Custom backend

The `runtime/render/PaintContext` produces an IR; `application.cpp::replay_paint_commands` is the only place that translates IR to D2D. Replacing this with a Skia or SDL_Renderer implementation is the path to macOS / Linux native support. The widget catalog and event dispatcher are backend-agnostic.

### 11.4 FFI

`ffi/c_api/include/hui/c_api.h` exposes a stable C ABI. Use this when binding from Rust, Go, Python, etc. The C API today covers `Application` / `Window` lifecycle + basic widget creation; more widget bindings are added on demand. See `ffi/c_api/include/hui/c_api.h` for the current surface.

---

## See also

- [Getting Started](getting-started.md) — Tutorial
- [API Reference](api-reference.md) — All public types
- [README](../README.md) — Project overview

For implementation-level reading: start at `runtime/object/include/hui/object/widget.hpp` (the central abstraction) and `runtime/app/src/application.cpp` (the message-loop + paint backend integration). Everything else flows from these two.
