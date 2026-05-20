# Getting Started with jtUI

A 30-minute tour from "Hello World" to a complete branded hero page. By the end of this tutorial you will know how to:

1. Build and run a jtUI app on Windows (native) or Linux (cross-compile to Windows).
2. Compose UIs from `Panel`, `Text`, `Button` and other built-in widgets.
3. Switch themes (Dark/Light) and locales (En/Zh) with one line.
4. Handle clicks, hover, and keyboard events.
5. Write your own custom widget that paints geometry through `PaintContext`.
6. Animate widgets using the built-in `tick(delta)` loop.

If you are looking for a complete API reference, jump to [`api-reference.md`](api-reference.md). For internals (rendering pipeline, dirty propagation, event dispatch) see [`architecture.md`](architecture.md).

---

## 0. Prerequisites

You need:

- **CMake 3.25+**
- **A C++20 compiler**:
  - On Windows: `clang++ 18+` (recommended) or MSVC v143+
  - On Linux for cross-compile: `mingw-w64` with POSIX threads (`x86_64-w64-mingw32-g++-posix`)
- **Ninja** (the project uses Ninja generator by default)

The Linux side is supported only for cross-compile and unit tests today — `Application::run()` returns 0 immediately on Linux because there is no GTK/X11 backend yet (see [Roadmap](#9-next-steps)).

### Clone & first build

```bash
git clone <jtUI-repo-url> jtUI
cd jtUI

# Windows (PowerShell):
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
.\build\examples\gallery\gallery.exe

# Linux → Windows cross compile:
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw
# Copy build-mingw/examples/gallery/gallery.exe to a Windows machine.
```

You should see the `gallery` window — the official component showcase. Click through the tabs to see every widget jtUI ships.

---

## 1. Hello World

Create a new directory `examples/hello/` with two files:

### `examples/hello/main.cpp`

```cpp
#include "jtui/jtui.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

constexpr float kWidth  = 800.0F;
constexpr float kHeight = 600.0F;

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "Hello jtUI";
    options.frameless = true;             // Custom title bar
    options.size      = {kWidth, kHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(jtui::Color::from_hex("#050505"));
    root->set_corner_radius(0.0F);
    root->set_frame({0.0F, 0.0F, kWidth, kHeight});

    // Custom title bar (drag-to-move + minimize/maximize/close handled by jtUI)
    auto window_frame = std::make_unique<jtui::WindowFrame>();
    window_frame->set_frameless(true);
    window_frame->set_frame({0.0F, 0.0F, kWidth, kHeight});
    root->append_child(std::move(window_frame));

    auto title_bar = std::make_unique<jtui::TitleBar>("Hello jtUI");
    title_bar->set_frame({0.0F, 0.0F, kWidth, 36.0F});
    root->append_child(std::move(title_bar));

    // Big centered greeting
    auto greet = std::make_unique<jtui::Text>("Hello, jtUI!");
    greet->set_font_size(48.0F);
    greet->set_bold(true);
    greet->set_color(jtui::Color::from_hex("#FB923C"));
    greet->set_alignment(jtui::TextAlignment::Center);
    greet->set_frame({0.0F, 220.0F, kWidth, 60.0F});
    root->append_child(std::move(greet));

    // Get Started button
    auto cta = std::make_unique<jtui::Button>("Get Started");
    cta->set_shape(jtui::ButtonShape::Pill);
    cta->set_colors(
        jtui::Color::from_hex("#FB923C"),   // idle
        jtui::Color::from_hex("#FDA45F"),   // hover
        jtui::Color::from_hex("#EA8224"),   // pressed
        jtui::Color::from_hex("#1A0F05"));  // text
    cta->set_font_size(14.0F, /*bold=*/true);
    cta->set_frame({340.0F, 320.0F, 120.0F, 44.0F});
    cta->on_clicked().connect([]() {
        // Your callback here. Try popping a Dialog, navigating to a page, etc.
    });
    root->append_child(std::move(cta));

    window.set_content(std::move(root));
    return app.run();
}

}  // namespace

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) { return run_app(); }
#else
int main() { return run_app(); }
#endif
```

### `examples/hello/CMakeLists.txt`

```cmake
set(HELLO_SOURCES main.cpp)

if(WIN32)
    add_executable(hello WIN32 ${HELLO_SOURCES})
    if(MINGW)
        target_compile_options(hello PRIVATE -municode)
        target_link_options(hello PRIVATE -municode)
    endif()
else()
    add_executable(hello ${HELLO_SOURCES})
endif()

target_include_directories(hello PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(hello PRIVATE jtui::api)

hui_configure_target(hello)
```

Wire it into the top-level `CMakeLists.txt`:

```cmake
if(HUI_BUILD_EXAMPLES)
    add_subdirectory(examples/gallery)
    # ...other examples...
    add_subdirectory(examples/hello)   # add this line
endif()
```

Build it:

```bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw --target hello
# Run build-mingw/examples/hello/hello.exe on Windows.
```

### What you just learned

- **`jtui::Application`** owns the message loop. **`Window`** is the OS window handle.
- **Widget tree** is built bottom-up with `std::make_unique<T>()` then `parent->append_child(std::move(child))`. Ownership flows up — the parent owns all descendants.
- **All coordinates are absolute** (logical pixels, DPI-aware). `set_frame({x, y, w, h})` places a widget anywhere inside its window. Parents do not automatically translate children — this is intentional and keeps the model simple.
- **Colors** are `jtui::Color` (float RGBA 0..1) or built from `from_hex("#RRGGBB")`.
- **Click handlers** are `Signal<>` connections; `on_clicked().connect(lambda)` is the canonical pattern.

---

## 2. State and Rebuild

UIs need state. jtUI's default state model is **full-tree rebuild**: keep your state in a plain struct, write a `build_root()` function that builds the entire widget tree from current state, and call it on every state change.

This sounds slow but is actually fast — rebuilding a typical screen is ~1 ms because Panel/Text/Button are cheap, and the dirty-rect repaint only redraws what changed.

```cpp
struct AppState {
    int counter = 0;
    bool dark_mode = true;
};

using RebuildFn = std::function<void()>;

std::unique_ptr<jtui::Panel> build_root(AppState& state, const RebuildFn& rebuild) {
    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(state.dark_mode
        ? jtui::Color::from_hex("#050505")
        : jtui::Color::from_hex("#FAFAFA"));
    root->set_frame({0, 0, 800, 600});

    auto counter_text = std::make_unique<jtui::Text>(
        "Count: " + std::to_string(state.counter));
    counter_text->set_font_size(32.0F);
    counter_text->set_color(state.dark_mode
        ? jtui::Color::from_hex("#FFFFFF")
        : jtui::Color::from_hex("#0A0A0A"));
    counter_text->set_alignment(jtui::TextAlignment::Center);
    counter_text->set_frame({0, 200, 800, 50});
    root->append_child(std::move(counter_text));

    auto inc_btn = std::make_unique<jtui::Button>("+1");
    inc_btn->set_shape(jtui::ButtonShape::Pill);
    inc_btn->set_frame({340, 280, 120, 44});
    inc_btn->on_clicked().connect([&state, rebuild]() {
        state.counter += 1;
        rebuild();
    });
    root->append_child(std::move(inc_btn));

    auto toggle_btn = std::make_unique<jtui::Button>(
        state.dark_mode ? "Light mode" : "Dark mode");
    toggle_btn->set_shape(jtui::ButtonShape::Pill);
    toggle_btn->set_frame({340, 340, 120, 36});
    toggle_btn->on_clicked().connect([&state, rebuild]() {
        state.dark_mode = !state.dark_mode;
        rebuild();
    });
    root->append_child(std::move(toggle_btn));

    return root;
}

int run_app() {
    jtui::Application app;
    jtui::WindowOptions options{};
    options.title = "Counter";
    options.size = {800, 600};
    jtui::Window& window = app.create_window(options);

    auto state = std::make_shared<AppState>();

    // Self-referencing rebuild closure. Wrap in shared_ptr so the lambda can
    // reference its own handle.
    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, state, rebuild_holder]() {
        window.set_content(build_root(*state, *rebuild_holder));
    };

    window.set_content(build_root(*state, *rebuild_holder));
    return app.run();
}
```

### Key concepts

- **`AppState` is plain data** — no widgets, no signals. Clean and easy to reason about.
- **`build_root(state, rebuild)`** is pure: same state → same tree.
- **Click callbacks mutate state + call `rebuild()`**. Rebuild calls `window.set_content(new_root)` which replaces the entire widget tree.
- **The old widget tree is not destroyed immediately** — it goes into a `pending_destroy_` queue and is freed after the current event finishes unwinding. This is what makes "modify state inside a click handler then rebuild" safe (no use-after-free even when the click came from a widget that's about to be destroyed). See `Window::set_content` internals.

### When NOT to rebuild

Full-tree rebuild is the default but not the only option:

- **Animation widgets** (custom `tick()`) repaint themselves without rebuilding the tree (see Section 7).
- **`ScrollView`** scrolls via `translate_subtree`, not rebuild.
- **Heavy widgets** (`VideoPlayer`, `AudioPlayer`) can be persisted across rebuilds using `release_child<T>` to preserve decoder state. See `examples/jtui_studio/main.cpp` for the `PersistentWidgets` pattern.

---

## 3. Layout

jtUI uses **absolute frame coordinates** by default. Each widget is placed at `{x, y, w, h}` in window space. This is simple and predictable but means you compute positions manually.

For more complex layouts there are three options:

### Option A: Absolute math (default, used in all `examples/`)

```cpp
constexpr float kNavBarH = 60.0F;
constexpr float kSidePad = 80.0F;
const float right_x = kWindowWidth - kSidePad;
const float btn_y   = kTitleBarH + (kNavBarH - 38.0F) * 0.5F;

cta->set_frame({right_x - 130.0F, btn_y, 130.0F, 38.0F});
```

Verbose but explicit. Great when you have a designed layout (specific paddings and gutters).

### Option B: FlexBox

```cpp
auto row = std::make_unique<jtui::FlexBox>();
row->set_direction(jtui::FlexDirection::Row);
row->set_gap(12.0F);
row->set_padding(16.0F);
row->set_justify(jtui::JustifyContent::SpaceBetween);
row->set_align(jtui::AlignItems::Center);
row->set_frame({0, 0, 800, 60});

row->append_child(std::move(logo));
row->append_child(std::move(nav_links));
row->append_child(std::move(cta));
```

CSS-flex-like behavior. Use for navigation bars, button groups, settings rows.

### Option C: ScrollView

```cpp
auto scroll = std::make_unique<jtui::ScrollView>();
scroll->set_frame({0, kTitleBarH, kWidth, kHeight - kTitleBarH});

auto content = std::make_unique<jtui::Panel>();
content->set_frame({0, 0, kWidth, kPageH});  // taller than viewport
// ...add children to content...

scroll->set_content(std::move(content));
scroll->arrange(scroll->frame());  // important: kick layout once
root->append_child(std::move(scroll));
```

Manual `arrange()` call is needed because the window only arranges the root once. ScrollView is currently vertical-only — horizontal mode is on the roadmap.

---

## 4. Theming and i18n

jtUI ships a token-based theme system. Switching dark/light or English/Chinese is one line; combine with a tree rebuild and the entire app re-themes.

### Brand tokens (per-example pattern)

Each example defines its own `brand_tokens.hpp`:

```cpp
namespace my_app::brand {

struct Palette {
    jtui::Color bg_window;
    jtui::Color bg_panel;
    jtui::Color bg_card;
    jtui::Color text_strong;
    jtui::Color text_primary;
    jtui::Color text_secondary;
    jtui::Color text_muted;
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;
    // ...add more as needed...
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#050505"),
    .accent        = jtui::Color::from_hex("#FB923C"),
    // ...
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#FAFAFA"),
    .accent        = jtui::Color::from_hex("#EA580C"),  // darker on light
    // ...
};

[[nodiscard]] inline const Palette& active() noexcept {
    return jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark
         ? dark : light;
}

}  // namespace my_app::brand
```

In your `build_root` start with `const auto& p = my_app::brand::active();` and reference `p.accent`, `p.bg_panel`, etc. Switching themes only requires re-calling `build_root` because `active()` reads `Theme::mode()`.

### Switching themes

```cpp
auto theme_btn = std::make_unique<jtui::Button>("");
theme_btn->set_shape(jtui::ButtonShape::Circle);
theme_btn->set_leading_codicon("color-mode");
theme_btn->on_clicked().connect([rebuild]() {
    const auto cur = jtui::theme::Theme::mode();
    jtui::theme::Theme::set(cur == jtui::theme::ThemeMode::Dark
        ? jtui::theme::ThemeMode::Light
        : jtui::theme::ThemeMode::Dark);
    rebuild();
});
```

### i18n in two steps

**Step 1**: register your strings once at startup:

```cpp
void register_strings() {
    jtui::i18n::register_entries({
        {"nav.home",      "Home",         "首页"},
        {"nav.docs",      "Docs",         "文档"},
        {"hero.cta",      "Get Started",  "立即开始"},
        {"hero.tagline",  "Build native. Ship fast.",
                          "原生构建，极速出货。"},
    });
}
```

**Step 2**: look up at build time:

```cpp
using jtui::i18n::tr;
auto tagline = std::make_unique<jtui::Text>(tr("hero.tagline"));
```

Toggle locale:

```cpp
lang_btn->on_clicked().connect([rebuild]() {
    const auto cur = jtui::i18n::current_locale();
    jtui::i18n::set_locale(cur == jtui::i18n::Locale::En
        ? jtui::i18n::Locale::Zh
        : jtui::i18n::Locale::En);
    rebuild();
});
```

---

## 5. Events: click, hover, keyboard

Most widgets expose `Signal<...>` for their semantic events:

```cpp
button->on_clicked().connect([](){ /*...*/ });
button->on_hover_changed().connect([](bool hovered){ /*...*/ });

switch_widget->on_changed().connect([](bool checked){ /*...*/ });

text_input->on_text_changed().connect([](std::string_view text){ /*...*/ });
text_input->on_submit().connect([](){ /*...*/ });

tabs->on_index_changed().connect([](int idx){ /*...*/ });
```

For custom widgets you override the virtual hooks:

```cpp
class HoverCard : public hui::Widget {
    void on_mouse_move(hui::PointF point) override { /*...*/ }
    void on_mouse_leave() override { /*...*/ }
    void on_press_changed(hui::PointF point, bool pressed) override { /*...*/ }
    void on_click(hui::PointF point) override { /*...*/ }
    bool on_key_down(hui::Key k, bool shift, bool ctrl, bool alt) override {
        if (k == hui::Key::Escape) { /*...*/ return true; }
        return false;
    }
};
```

Event dispatch is `Capture → Target → Bubble`. You can override the `on_event(Event&)` low-level entry to intercept at any phase — see Dialog's modal-scrim trick in `widgets/common/src/dialog.cpp`.

---

## 6. Custom widget: a glowing Counter

Time to write your first custom widget. We'll make a `GlowCounter` that paints a number with a soft accent halo behind it.

```cpp
#pragma once

#include "jtui/jtui.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

class GlowCounter : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "GlowCounter";
    }

    void set_count(int n) noexcept {
        if (n != count_) {
            count_ = n;
            mark_dirty(hui::DirtyFlags::Paint);
        }
    }

    void set_accent(jtui::Color c) noexcept { accent_ = c; }

    void paint(hui::PaintContext& ctx) const override {
        const auto f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // Soft halo: a translucent ellipse behind the number.
        const float cx = f.x + f.width * 0.5F;
        const float cy = f.y + f.height * 0.5F;
        const float halo_r = std::min(f.width, f.height) * 0.45F;
        const jtui::Color halo{accent_.r, accent_.g, accent_.b, 0.18F};
        ctx.fill_ellipse(
            {cx - halo_r, cy - halo_r, halo_r * 2.0F, halo_r * 2.0F},
            halo);

        // Number on top.
        ctx.draw_text(
            f, std::to_string(count_), jtui::Color::from_hex("#FFFFFF"),
            jtui::TextAlignment::Center,
            /*font_size=*/56.0F, /*bold=*/true);
    }

private:
    int count_ {0};
    jtui::Color accent_ {jtui::Color::from_hex("#FB923C")};
};
```

Use it:

```cpp
auto counter = std::make_unique<GlowCounter>();
counter->set_count(state.counter);
counter->set_accent(p.accent);
counter->set_frame({340, 220, 120, 120});
root->append_child(std::move(counter));
```

### `PaintContext` cheatsheet

| Call | Effect |
|---|---|
| `fill_rect(bounds, color)` | Solid rectangle |
| `stroke_rect(bounds, color, thickness)` | Outline rectangle |
| `fill_rounded_rect(bounds, color, radius)` | Rounded fill |
| `stroke_rounded_rect(bounds, color, radius, thickness)` | Rounded outline |
| `fill_ellipse(bounds, color)` | Solid ellipse / circle |
| `stroke_ellipse(bounds, color, thickness)` | Outline ellipse |
| `line(p0, p1, color, thickness)` | Single straight line |
| `draw_bezier(p0, c1, c2, p3, color, thickness)` | Cubic bezier curve |
| `fill_polygon({pts}, color)` | Filled polygon, auto-closed |
| `fill_gradient_rect(bounds, top, bottom, radius)` | Linear gradient top→bottom |
| `fill_shadow(bounds, radius, offset, blur, spread, color)` | CSS-like box shadow |
| `fill_backdrop_blur(bounds, radius, blur, tint)` | Frosted glass |
| `draw_text(bounds, text, color, align, size, bold)` | Render text |
| `draw_texture(bounds, pixel_buffer, alpha_mode)` | Draw RGBA pixels |
| `push_clip(bounds)` / `pop_clip()` | Manual rect clip |

For complex example geometries see `examples/jtui_cinema/thumbnail_art.hpp` (5 region-themed illustrations) and `examples/jtui_invest/animated_widgets.hpp` (animated charts).

---

## 7. Animation

Override `tick(float delta)` and return `true` while you want more frames:

```cpp
class FadeIn : public hui::Widget {
public:
    void set_color(jtui::Color c) noexcept { target_color_ = c; }
    void set_duration(float seconds) noexcept { duration_ = seconds; }

    bool tick(float delta) override {
        if (progress_ >= 1.0F) return false;
        progress_ += delta / duration_;
        progress_ = std::min(progress_, 1.0F);
        mark_dirty(hui::DirtyFlags::Paint);
        return progress_ < 1.0F;
    }

    void paint(hui::PaintContext& ctx) const override {
        const float eased = ease_out_cubic(progress_);
        const auto f = frame();
        ctx.fill_rect(f, jtui::Color{
            target_color_.r, target_color_.g, target_color_.b,
            target_color_.a * eased});
    }

private:
    static float ease_out_cubic(float t) noexcept {
        const float inv = 1.0F - t;
        return 1.0F - inv * inv * inv;
    }
    jtui::Color target_color_ {};
    float progress_ {0.0F};
    float duration_ {0.4F};
};
```

`Application` keeps a `WM_TIMER`-driven 60 fps tick loop running while any widget in the tree returns `true` from `tick()`. The loop stops automatically when nothing is animating, so you don't pay for idle frames.

### Easing functions

Common easings (paste into your widget):

```cpp
static float ease_out_cubic(float t) { float i = 1 - t; return 1 - i*i*i; }
static float ease_in_out_cubic(float t) {
    if (t < 0.5F) return 4 * t * t * t;
    const float p = -2 * t + 2;
    return 1 - p * p * p * 0.5F;
}
```

For carousel-style "slide from A to B" animation, prefer `ease_in_out_cubic` — see `examples/jtui_cinema/carousel_animator.hpp` for a complete implementation including a key detail: when jtUI synchronously ticks one frame on `WM_LBUTTONUP` to eliminate button feedback latency, spring-style animations jump on frame 1 by `diff × k` pixels (which can be 50+px and look like "teleport then slide"). `progress + ease-in-out` produces ~0.2 px on frame 1 — buttery smooth.

### Repaint without residual frames

A common animation pitfall: calling `set_frame` repeatedly to move a widget will trigger dirty-rect repaints, but the dirty rect only covers the **new** position. The **old** position's pixels stay on screen → "ghosting" artifacts.

The fix is to use `translate_subtree(dx, dy)` (which does not mark dirty) plus a single `mark_dirty(Paint)` on a container widget whose `frame` covers the entire animated area:

```cpp
class CarouselAnimator : public hui::Widget {
public:
    void add_target(hui::Element* w) { targets_.push_back(w); }

    bool tick(float dt) override {
        const float dx = compute_dx_from_progress(dt);
        for (auto* w : targets_) w->translate_subtree(dx, 0);
        mark_dirty(hui::DirtyFlags::Paint);  // animator.frame covers whole area
        return progress_ < 1.0F;
    }
};
```

Set the animator's `frame` to cover the whole reel/scrollable region. The `mark_dirty(Paint)` will cause the entire region to repaint and old pixels are cleanly erased.

---

## 8. Putting it all together: a brand hero

Combine theming + i18n + custom widget + animation into a full brand example. Pattern:

```
examples/my_brand/
├── brand_tokens.hpp     // Palette + active()
├── i18n_catalog.hpp     // register_strings()
├── main.cpp             // build_root + run_app
└── CMakeLists.txt
```

A minimal `build_root`:

```cpp
std::unique_ptr<jtui::Panel> build_root(AppState& state, const RebuildFn& rebuild) {
    const auto& p = my_brand::brand::active();
    using jtui::i18n::tr;

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(p.bg_window);
    root->set_frame({0, 0, kWindowWidth, kWindowHeight});

    // Standard window chrome (frameless + custom title bar)
    auto wf = std::make_unique<jtui::WindowFrame>();
    wf->set_frameless(true);
    wf->set_frame({0, 0, kWindowWidth, kWindowHeight});
    root->append_child(std::move(wf));

    auto tb = std::make_unique<jtui::TitleBar>(tr("window.title"));
    tb->set_frame({0, 0, kWindowWidth, kTitleBarH});
    root->append_child(std::move(tb));

    build_navbar(*root, state, rebuild);   // theme + lang switches
    build_hero(*root, state);              // big title + CTA
    build_features(*root, state);          // 3-column feature grid
    build_footer(*root);

    return root;
}
```

For a complete reference look at any of the 9 brand examples in `examples/`:

- **`jtui_hero`** — minimal hero page with title + 3 features + dual CTA
- **`jtui_cinema`** — carousel + real video playback + custom geometry widget
- **`jtui_invest`** — scrollable dashboard with backdrop blur nav + animations
- **`folders_app`** — sidebar nav + folder grid + search filter
- **`jtui_studio`** — media app with `VideoPlayer` + `AudioPlayer` + `WaveformView`

All ten ship the same `About` modal (via `hui::install_about_card`) — copy that pattern into your app to give users a quick "about this project" panel.

---

## 9. Next steps

Now that you can build a jtUI app, here's where to go next:

- **[API Reference](api-reference.md)** — Every public type and function, with signatures and examples.
- **[Architecture](architecture.md)** — How the rendering pipeline, dirty propagation, event dispatch, and theme tokens work under the hood. Read this when you want to write framework-level widgets or contribute to the runtime.

### Common patterns to copy

| Pattern | Example file |
|---|---|
| Brand palette + dark/light | `examples/jtui_hero/brand_tokens.hpp` |
| i18n catalog with `register_entries` | `examples/jtui_cinema/i18n_catalog.hpp` |
| `PersistentWidgets` for `VideoPlayer` cross-rebuild | `examples/jtui_studio/main.cpp` |
| Ease-in-out carousel animator | `examples/jtui_cinema/carousel_animator.hpp` |
| Custom geometry widget with `clips_self()` | `examples/jtui_cinema/thumbnail_art.hpp` |
| Backdrop blur nav | `examples/jtui_invest/main.cpp` |
| Count-up / draw-in / grow-in animations | `examples/jtui_invest/animated_widgets.hpp` |
| About modal install (framework-level) | `widgets/common/include/hui/widgets/common/about_card.hpp` |

### Roadmap

What's coming in upcoming releases:

- ScrollView horizontal mode (`set_orientation(Horizontal)`)
- AnimationState abstraction (cross-rebuild animation progress preservation)
- `clips_self_rounded(corner_radius)` for true rounded clipping
- macOS backend via SDL3 + Skia
- Linux native backend via GTK4 or SDL3

Future enhancements live in our project backlog.

---

If you build something with jtUI, we'd love to see it. The project lives at the jtai team (`曾能混 <jwhna1@gmail.com>`) — issue reports, design discussions, and showcases are welcome.

Happy hacking.
