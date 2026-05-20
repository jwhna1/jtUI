<div align="center">

# jtUI

**A C++ desktop UI framework with Vue-like simplicity.**

Pure C++20. Real frames. No Electron. No JS bridge.

[![License: MIT](https://img.shields.io/badge/License-MIT-FB923C.svg?style=flat-square)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?style=flat-square&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%2010%2B-0078D4.svg?style=flat-square&logo=windows&logoColor=white)](#build)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org)
[![Build: MSVC | MinGW | Clang](https://img.shields.io/badge/Build-MSVC%20%7C%20MinGW%20%7C%20Clang-success.svg?style=flat-square)](#build)
[![Status: pre-release](https://img.shields.io/badge/Status-pre--release-FB923C.svg?style=flat-square)](#project-status)
[![Version: v1.0.0](https://img.shields.io/badge/Release-v1.0.0-1F2937.svg?style=flat-square)](#)

[Website](https://jtai.cc) · [简体中文](README.zh-CN.md) · [Getting Started](docs/getting-started.md) · [API Reference](docs/api-reference.md) · [Architecture](docs/architecture.md) · [Porting](platform/PORTING.md)

---

<img src="docs/images/jtUI.gif" alt="jtUI showcase" width="900"/>

</div>

---

## Why jtUI

jtUI is what you get when you ask: _what if a native desktop UI toolkit shipped with the developer ergonomics of Vue.js, but without paying the Electron tax?_

- **Pure C++20** — runtime ~150K lines, no Node, no JS engine, no webview. Your app is a single `.exe`.
- **Real native frames** — Direct2D 1.1 rendering on Windows, with backdrop blur, true gaussian box shadow, bezier/polygon primitives, HiDPI-aware geometry, GPU-decoded video frames.
- **Vue-inspired developer flow** — declarative widget trees, `Signal<T>` / `Property<T>` reactivity, full-tree rebuild as the default state model. Mental model fits in one afternoon.
- **i18n + dual theme out of the box** — one-line `Theme::set(Dark/Light)` + `i18n::set_locale(En/Zh)` switch the entire app. No CSS-in-JS gymnastics.
- **Media-native widgets** — `VideoPlayer` (MF + Direct2D), `AudioPlayer` (WASAPI shared mode), `WaveformView`, `Timeline`, `LevelMeter` — built into the framework, not bolted on.
- **40+ widgets ready** across `widgets/{basic,common,media}` — Buttons, Inputs, Tabs, Dialog, Popover, Tooltip, Gauges, Avatars, SidebarNav, AboutCard, and many more.

## Quick Start

```cpp
#include "jtui/jtui.hpp"

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "Hello jtUI";
    options.frameless = true;
    options.size      = {800.0F, 600.0F};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_frame({0.0F, 0.0F, 800.0F, 600.0F});

    auto title = std::make_unique<jtui::Text>("Hello, jtUI");
    title->set_font_size(32.0F);
    title->set_bold(true);
    title->set_color(jtui::Color::from_hex("#FB923C"));
    title->set_alignment(jtui::TextAlignment::Center);
    title->set_frame({0.0F, 240.0F, 800.0F, 50.0F});
    root->append_child(std::move(title));

    auto cta = std::make_unique<jtui::Button>("Get Started");
    cta->set_shape(jtui::ButtonShape::Pill);
    cta->set_colors(
        jtui::Color::from_hex("#FB923C"),
        jtui::Color::from_hex("#FDA45F"),
        jtui::Color::from_hex("#EA8224"),
        jtui::Color::from_hex("#1A0F05"));
    cta->set_frame({340.0F, 320.0F, 120.0F, 44.0F});
    cta->on_clicked().connect([]() {
        // your callback here
    });
    root->append_child(std::move(cta));

    window.set_content(std::move(root));
    return app.run();
}

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) { return run_app(); }
#else
int main() { return run_app(); }
#endif
```

Build with CMake + Ninja (full instructions below). On Windows you get a hardware-accelerated native window. On Linux the runtime stubs out (no Direct2D), useful for cross-compile and CI.

For a deeper walkthrough see [`docs/getting-started.md`](docs/getting-started.md).

## Highlights

### Theme + i18n in two lines

```cpp
jtui::i18n::register_entries({
    {"hero.title", "Build Native. Ship Fast.", "原生构建，极速出货。"},
});
auto title = std::make_unique<jtui::Text>(jtui::i18n::tr("hero.title"));
title->set_color(jtui::theme::colors().text_strong);
```

Switching the whole app between Dark/Light or English/Chinese is one line — `jtui::theme::Theme::set(...)` / `jtui::i18n::set_locale(...)`. Combine with a full-tree `window.set_content(build_root())` rebuild and you are done.

### Backdrop blur (D2D 1.1 gaussian)

```cpp
auto glass_nav = std::make_unique<jtui::Panel>();
glass_nav->set_frame({0, 0, kWidth, kNavBarH});
glass_nav->set_background_color({0, 0, 0, 0});  // must be transparent
glass_nav->set_backdrop_blur(64.0F, brand.glass_tint);
root.append_child(std::move(glass_nav));
```

True hardware-accelerated frosted-glass nav bars — the same effect macOS Safari ships, implemented via `CopyFromRenderTarget` → `CLSID_D2D1GaussianBlur` → `DrawImage` + tint overlay. Degrades cleanly on systems without D2D 1.1.

### Drop shadows via elevation tokens

```cpp
card->set_corner_radius(jtui::radius_lg);
card->set_shadow(jtui::theme::elevation().level_2);  // CSS-like box-shadow
```

5 elevation levels (`level_0..4`) mapped to true gaussian shadows via `CLSID_D2D1Shadow`. No more "draw a darker rectangle behind the card" hacks.

### Real video + audio playback

```cpp
auto video = std::make_unique<jtui::VideoPlayer>();
video->set_source("intro.mp4");
video->play();
video->set_frame({40, 80, 720, 405});
root.append_child(std::move(video));
```

`VideoPlayer` decodes via Windows Media Foundation, GPU textures from D2D bitmap atlas. `AudioPlayer` plays via WASAPI shared mode with sub-50ms latency. `WaveformView` + `Timeline` + `LevelMeter` round out a media-native widget set.

### Custom widgets — paint any geometry you want

```cpp
class ThumbnailArt : public hui::Widget {
public:
    void paint(hui::PaintContext& ctx) const override {
        const auto f = frame();
        ctx.fill_gradient_rect(f, top_color, bottom_color, /*radius=*/10.0F);
        ctx.fill_polygon({/* mountain silhouette */}, mountain_color);
        ctx.draw_bezier(p0, c1, c2, p3, leaf_color, /*thickness=*/4.0F);
        ctx.fill_ellipse({/* sun */}, sun_color);
    }
    [[nodiscard]] bool clips_self() const noexcept override { return true; }
};
```

`PaintContext` gives you fill_rect / stroke_rect / fill_rounded_rect / fill_ellipse / line / draw_bezier / fill_polygon / fill_gradient_rect / fill_shadow / fill_backdrop_blur / draw_text / draw_texture / push_clip / pop_clip. See `examples/jtui_cinema/thumbnail_art.hpp` for a full geometry-based illustration widget.

### Animation via `Widget::tick`

```cpp
class CountUpText : public hui::Widget {
    bool tick(float delta) override {
        if (progress_ >= 1.0F) return false;
        progress_ += delta / duration_;
        mark_dirty(hui::DirtyFlags::Paint);
        return progress_ < 1.0F;
    }
    void paint(hui::PaintContext& ctx) const override {
        const float eased = ease_out_cubic(progress_);
        ctx.draw_text(frame(), format(target_ * eased), color_, ...);
    }
};
```

Built-in 60 fps tick loop (`SetTimer` on Windows). Return `true` from `tick` to keep animating, `false` to stop. See `examples/jtui_invest/animated_widgets.hpp` for count-up / line-chart / bar-chart animations, and `examples/jtui_cinema/carousel_animator.hpp` for ease-in-out carousel slide.

## Architecture

Three layers, each in its own subtree:

```
runtime/        Platform-neutral core
  foundation/   Color · i18n · log · codicon lookup
  object/       Node / Element / Widget · DirtyFlags · z-order
  property/     Signal<T> · Property<T> · RealtimeSource<T>
  theme/        SemanticColor tokens · elevation · spacing · typography
  layout/       FlexBox · two-pass measure/arrange
  event/        EventDispatcher · capture-target-bubble
  render/       PaintContext · DrawCommand IR
  media/        IVideoDecoder · IAudioDecoder · TextureHandle
  app/          Application · Window · WindowFrame · TitleBar

platform/
  win32/        Direct2D 1.1 renderer · MF decoder · WASAPI · codicon font loader

widgets/
  basic/        Panel · Text · CodiconIcon · ScrollView · ListView · FlexBox
  common/       Button · TextInput · Switch · Checkbox · Tabs · Dialog · Popover ·
                Tooltip · Slider · Toolbar · Dropdown · SearchInput · ProgressBar ·
                RadialGauge · SemiGauge · Gauge · Chip · Badge · Card · Avatar ·
                FolderCard · MetricCard · SidebarNav · AboutCard
  media/        VideoPlayer · AudioPlayer · WaveformView · Timeline · LevelMeter

api/            jtui:: namespace facade (umbrella header)
ffi/c_api/      C ABI (for FFI from other languages)
examples/       10 brand demos (gallery + 9 product hero pages)
docs/           Architecture · API reference · Getting started
```

Detailed architecture in [`docs/architecture.md`](docs/architecture.md). Porting notes to non-Win32 in [`platform/PORTING.md`](platform/PORTING.md).

## Build

### Windows native (recommended: VS Developer PowerShell)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
.\build\examples\gallery\gallery.exe
```

### Linux → Windows cross compile (MinGW-w64)

```bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw
# Copy build-mingw/examples/gallery/gallery.exe to a Windows machine to run.
```

### Linux native (CI / platform-neutral unit tests only)

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

`Application::run()` returns `0` immediately on Linux — there is no GTK / X11 backend yet. Use Linux only for cross-compile + unit tests today.

### CMake options

| Option | Default | Effect |
|---|---|---|
| `HUI_BUILD_EXAMPLES` | ON | Build the 10 brand examples |
| `HUI_BUILD_TESTS` | ON | Build `tests/hui_unit_tests` |
| `HUI_WARNINGS_AS_ERRORS` | OFF | Promote warnings to errors |

## Examples

Each example is a complete brand hero page assembled from `brand_tokens.hpp` + `i18n_catalog.hpp` + `main.cpp`, exercising a different slice of the framework. Every example ships with **Dark/Light theme + English/Chinese i18n** toggles (round icon buttons in the title bar) and an About modal.

<!--
Screenshot placeholders below. Drop two PNGs per example into docs/images/:
  docs/images/<name>-dark.png   (recommended ~1280x800, dark theme)
  docs/images/<name>-light.png  (same size, light theme)
GitHub renders the relative paths once the repo is pushed.
-->

<table>
  <tr>
    <td width="50%" align="center">
      <b>gallery</b><br/>
      <sub>Component library: all widgets in tabbed sections + VSCode palette/codicons</sub><br/><br/>
      <img src="docs/images/gallery-dark.png" alt="gallery (dark)" width="100%"/>
      <img src="docs/images/gallery-light.png" alt="gallery (light)" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_hero</b><br/>
      <sub>Brand hero page, double-line title with inline <code>TextRun</code> accent highlights</sub><br/><br/>
      <img src="docs/images/jtui_hero-dark.png" alt="jtui_hero (dark)" width="100%"/>
      <img src="docs/images/jtui_hero-light.png" alt="jtui_hero (light)" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_cinema</b><br/>
      <sub>Video carousel: 5 geometric thumbnails + ease-in-out slide + real <code>VideoPlayer</code></sub><br/><br/>
      <img src="docs/images/jtui_cinema-dark.png" alt="jtui_cinema (dark)" width="100%"/>
      <img src="docs/images/jtui_cinema-light.png" alt="jtui_cinema (light)" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_studio</b><br/>
      <sub>Media studio: <code>VideoPlayer</code> + <code>AudioPlayer</code> + <code>WaveformView</code> cross-rebuild persistence</sub><br/><br/>
      <img src="docs/images/jtui_studio-dark.png" alt="jtui_studio (dark)" width="100%"/>
      <img src="docs/images/jtui_studio-light.png" alt="jtui_studio (light)" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_invest</b><br/>
      <sub>Financial landing: backdrop blur nav + count-up animation + ScrollView</sub><br/><br/>
      <img src="docs/images/jtui_invest-dark.png" alt="jtui_invest (dark)" width="100%"/>
      <img src="docs/images/jtui_invest-light.png" alt="jtui_invest (light)" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_atlas</b><br/>
      <sub>KPI sparklines + chart cards + grid background</sub><br/><br/>
      <img src="docs/images/jtui_atlas-dark.png" alt="jtui_atlas (dark)" width="100%"/>
      <img src="docs/images/jtui_atlas-light.png" alt="jtui_atlas (light)" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_live</b><br/>
      <sub>Code editor mock-up with hot-reload status card</sub><br/><br/>
      <img src="docs/images/jtui_live-dark.png" alt="jtui_live (dark)" width="100%"/>
      <img src="docs/images/jtui_live-light.png" alt="jtui_live (light)" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_pro</b><br/>
      <sub>Enterprise SSO landing: bezier stream lines + trust badges</sub><br/><br/>
      <img src="docs/images/jtui_pro-dark.png" alt="jtui_pro (dark)" width="100%"/>
      <img src="docs/images/jtui_pro-light.png" alt="jtui_pro (light)" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_pulse</b><br/>
      <sub>Live chart background with custom <code>clips_self()</code> widget</sub><br/><br/>
      <img src="docs/images/jtui_pulse-dark.png" alt="jtui_pulse (dark)" width="100%"/>
      <img src="docs/images/jtui_pulse-light.png" alt="jtui_pulse (light)" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>folders_app</b><br/>
      <sub>File manager: sidebar nav + folder grid + search filter</sub><br/><br/>
      <img src="docs/images/folders_app-dark.png" alt="folders_app (dark)" width="100%"/>
      <img src="docs/images/folders_app-light.png" alt="folders_app (light)" width="100%"/>
    </td>
  </tr>
</table>

## Project status

jtUI is on its way to a `1.0` release. Today's status:

- **Stable**: rendering pipeline (D2D 1.1), widget catalog, theme + i18n, animation tick, full-tree rebuild as state model, build (Windows native + MinGW cross)
- **In progress**: GPU texture registry, ScrollView horizontal mode, AnimationState abstraction
- **Roadmap**: macOS via SDL3 + Skia backend, Linux native via GTK4 or SDL3

## License

Released under the [MIT License](LICENSE). Commercial use is welcome — fork freely, build products, no royalty needed.

## Credits

- **jtai team**, led by Zeng Nenghun (`jwhna1@gmail.com`). The project started March 2025 with the explicit goal of bringing Vue-like ergonomics to native C++ desktop development.
- **Icons**: [`@vscode/codicons`](https://github.com/microsoft/vscode-codicons) (MIT) — 649 named glyphs embedded into the binary, loaded via `IDWriteFactory5` custom font collection.

## Contributing

The project is in pre-release. We are not yet accepting external PRs while the API surface is stabilizing for `1.0`. Issue reports and design discussions are welcome — please open an issue with your use case before sending code. See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).

---

<div align="center">
<sub>Built with C++20 · No Electron · No JS bridge · One <code>.exe</code> to ship.</sub>
</div>
