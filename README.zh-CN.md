<div align="center">

# jtUI

**追求 Vue 般简单的 C++ 桌面 UI 框架。**

纯 C++20。原生帧渲染。无 Electron。无 JS 桥。

[![License: MIT](https://img.shields.io/badge/License-MIT-FB923C.svg?style=flat-square)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?style=flat-square&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%2010%2B-0078D4.svg?style=flat-square&logo=windows&logoColor=white)](#构建)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org)
[![Build: MSVC | MinGW | Clang](https://img.shields.io/badge/Build-MSVC%20%7C%20MinGW%20%7C%20Clang-success.svg?style=flat-square)](#构建)
[![Status: pre-release](https://img.shields.io/badge/Status-pre--release-FB923C.svg?style=flat-square)](#项目状态)
[![Version: v1.0.0](https://img.shields.io/badge/Release-v1.0.0-1F2937.svg?style=flat-square)](#)

[官网](https://jtai.cc) · [English](README.md) · [快速上手](docs/getting-started.zh-CN.md) · [API 参考](docs/api-reference.zh-CN.md) · [架构说明](docs/architecture.zh-CN.md) · [平台移植](platform/PORTING.md)

---

<img src="docs/images/jtUI.gif" alt="jtUI 演示" width="900"/>

</div>

---

## 为什么是 jtUI

jtUI 来自一个问题：**如果原生桌面 UI 工具集能像 Vue.js 那样好写，但不用付 Electron 的税，会是什么样？**

- **纯 C++20** — runtime 约 15 万行代码，没有 Node，没有 JS 引擎，没有 webview。你的 app 是一个单独的 `.exe`。
- **真原生帧** — Windows 走 Direct2D 1.1 渲染，自带毛玻璃（backdrop blur）、真高斯模糊阴影、贝塞尔/多边形原语、HiDPI 几何感知、GPU 解码视频帧。
- **Vue 风格的开发体感** — 声明式 widget 树、`Signal<T>` / `Property<T>` 响应式、整树 rebuild 作为默认状态模型。心智模型一个下午就能上手。
- **i18n + 双主题开箱即用** — 一行 `Theme::set(Dark/Light)` + `i18n::set_locale(En/Zh)` 全应用切换。不需要 CSS-in-JS 那套体操。
- **媒体级原生 widget** — `VideoPlayer`（MF + Direct2D）、`AudioPlayer`（WASAPI 共享模式）、`WaveformView`、`Timeline`、`LevelMeter` 都是框架内置，不是外挂。
- **40+ widget 即用** —— `widgets/{basic,common,media}` 三大目录覆盖 Button / Input / Tabs / Dialog / Popover / Tooltip / Gauge / Avatar / SidebarNav / AboutCard 等常用形态。

## 快速上手

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

    auto title = std::make_unique<jtui::Text>("你好，jtUI");
    title->set_font_size(32.0F);
    title->set_bold(true);
    title->set_color(jtui::Color::from_hex("#FB923C"));
    title->set_alignment(jtui::TextAlignment::Center);
    title->set_frame({0.0F, 240.0F, 800.0F, 50.0F});
    root->append_child(std::move(title));

    auto cta = std::make_unique<jtui::Button>("开始使用");
    cta->set_shape(jtui::ButtonShape::Pill);
    cta->set_colors(
        jtui::Color::from_hex("#FB923C"),
        jtui::Color::from_hex("#FDA45F"),
        jtui::Color::from_hex("#EA8224"),
        jtui::Color::from_hex("#1A0F05"));
    cta->set_frame({340.0F, 320.0F, 120.0F, 44.0F});
    cta->on_clicked().connect([]() {
        // 业务回调写这里
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

CMake + Ninja 构建（完整说明见下）。Windows 上跑出来是一扇硬件加速的原生窗口；Linux 上 runtime 自动 stub（没有 Direct2D 后端），适合做交叉编译和 CI。

详细教程见 [`docs/getting-started.zh-CN.md`](docs/getting-started.zh-CN.md)。

## 亮点能力

### 两行搞定主题 + i18n

```cpp
jtui::i18n::register_entries({
    {"hero.title", "Build Native. Ship Fast.", "原生构建，极速出货。"},
});
auto title = std::make_unique<jtui::Text>(jtui::i18n::tr("hero.title"));
title->set_color(jtui::theme::colors().text_strong);
```

整应用在深浅色 / 中英文之间切换都是一行 —— `jtui::theme::Theme::set(...)` / `jtui::i18n::set_locale(...)`。配合整树 `window.set_content(build_root())` rebuild 即可生效。

### 毛玻璃（D2D 1.1 高斯模糊）

```cpp
auto glass_nav = std::make_unique<jtui::Panel>();
glass_nav->set_frame({0, 0, kWidth, kNavBarH});
glass_nav->set_background_color({0, 0, 0, 0});  // 必须透明，否则盖掉模糊层
glass_nav->set_backdrop_blur(64.0F, brand.glass_tint);
root.append_child(std::move(glass_nav));
```

真正的硬件加速磨砂玻璃顶栏 —— 和 macOS Safari 一样的视觉，底层走 `CopyFromRenderTarget` → `CLSID_D2D1GaussianBlur` → `DrawImage` + tint。D2D 1.1 不可用时优雅降级。

### Elevation token 投影

```cpp
card->set_corner_radius(jtui::radius_lg);
card->set_shadow(jtui::theme::elevation().level_2);  // CSS box-shadow 语义
```

5 档阴影（`level_0..4`），底层走 `CLSID_D2D1Shadow` 真高斯模糊。不再用"画一圈深色矩形"的硬 hack 充阴影。

### 真视频 + 音频播放

```cpp
auto video = std::make_unique<jtui::VideoPlayer>();
video->set_source("intro.mp4");
video->play();
video->set_frame({40, 80, 720, 405});
root.append_child(std::move(video));
```

`VideoPlayer` 通过 Windows Media Foundation 解码，GPU texture 由 D2D bitmap atlas 承载。`AudioPlayer` 走 WASAPI 共享模式，延迟 < 50 ms。`WaveformView` + `Timeline` + `LevelMeter` 合起来是一套媒体原生 widget 套件。

### 自定义 widget —— 任意几何都能画

```cpp
class ThumbnailArt : public hui::Widget {
public:
    void paint(hui::PaintContext& ctx) const override {
        const auto f = frame();
        ctx.fill_gradient_rect(f, top_color, bottom_color, /*radius=*/10.0F);
        ctx.fill_polygon({/* 山脉剪影顶点 */}, mountain_color);
        ctx.draw_bezier(p0, c1, c2, p3, leaf_color, /*thickness=*/4.0F);
        ctx.fill_ellipse({/* 太阳 */}, sun_color);
    }
    [[nodiscard]] bool clips_self() const noexcept override { return true; }
};
```

`PaintContext` 提供 fill_rect / stroke_rect / fill_rounded_rect / fill_ellipse / line / draw_bezier / fill_polygon / fill_gradient_rect / fill_shadow / fill_backdrop_blur / draw_text / draw_texture / push_clip / pop_clip。完整范例见 `examples/jtui_cinema/thumbnail_art.hpp`。

### 通过 `Widget::tick` 做动画

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

内置 60 fps tick 循环（Windows 下用 `SetTimer`）。`tick` 返回 `true` 继续，`false` 停。范例：`examples/jtui_invest/animated_widgets.hpp`（数字 count-up / 折线 draw-in / 柱状 grow-in）+ `examples/jtui_cinema/carousel_animator.hpp`（ease-in-out 轮播滑动）。

## 架构概览

三层结构，每层一个目录：

```
runtime/        平台中立核心
  foundation/   Color · i18n · log · codicon 查表
  object/       Node / Element / Widget · DirtyFlags · z-order
  property/     Signal<T> · Property<T> · RealtimeSource<T>
  theme/        语义色 token · elevation · spacing · typography
  layout/       FlexBox · measure/arrange 两阶段
  event/        EventDispatcher · capture-target-bubble
  render/       PaintContext · DrawCommand IR
  media/        IVideoDecoder · IAudioDecoder · TextureHandle
  app/          Application · Window · WindowFrame · TitleBar

platform/
  win32/        Direct2D 1.1 renderer · MF decoder · WASAPI · codicon 字体加载

widgets/
  basic/        Panel · Text · CodiconIcon · ScrollView · ListView · FlexBox
  common/       Button · TextInput · Switch · Checkbox · Tabs · Dialog · Popover ·
                Tooltip · Slider · Toolbar · Dropdown · SearchInput · ProgressBar ·
                RadialGauge · SemiGauge · Gauge · Chip · Badge · Card · Avatar ·
                FolderCard · MetricCard · SidebarNav · AboutCard
  media/        VideoPlayer · AudioPlayer · WaveformView · Timeline · LevelMeter

api/            jtui:: 公共 namespace facade（伞 header）
ffi/c_api/      C ABI 入口（给其他语言 FFI）
examples/       10 个品牌演示（gallery + 9 个产品 hero 页）
docs/           架构 · API 参考 · 快速上手
```

详细架构见 [`docs/architecture.zh-CN.md`](docs/architecture.zh-CN.md)。非 Win32 平台移植说明见 [`platform/PORTING.md`](platform/PORTING.md)。

## 构建

### Windows 原生（推荐用 VS Developer PowerShell）

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
.\build\examples\gallery\gallery.exe
```

### Linux → Windows 交叉编译（MinGW-w64）

```bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw
# 把 build-mingw/examples/gallery/gallery.exe 拷到 Windows 上跑。
```

### Linux 原生（CI / 平台中立单测）

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Linux 上 `Application::run()` 直接 `return 0` —— 还没有 GTK / X11 后端。Linux 端今天只做交叉编译 + 单测。

### CMake 选项

| 选项 | 默认 | 作用 |
|---|---|---|
| `HUI_BUILD_EXAMPLES` | ON | 编译 10 个品牌 example |
| `HUI_BUILD_TESTS` | ON | 编译 `tests/hui_unit_tests` |
| `HUI_WARNINGS_AS_ERRORS` | OFF | 把 warning 当 error |

## Example 一览

每个 example 是一个完整品牌 hero 页，由 `brand_tokens.hpp` + `i18n_catalog.hpp` + `main.cpp` 构成，演示不同框架能力。所有 example 都内置**深浅双主题 + 中英双语切换**（标题栏圆形 icon 按钮）+ About 弹窗介绍项目。

<!--
example 截图占位。请把每个 example 的两张 PNG 放进 docs/images/：
  docs/images/<name>-dark.png   （建议 1280x800 左右，深色主题）
  docs/images/<name>-light.png  （同尺寸，浅色主题）
仓库 push 后 GitHub 会自动按相对路径渲染。
-->

<table>
  <tr>
    <td width="50%" align="center">
      <b>gallery</b><br/>
      <sub>组件库：tab 切换的所有 widget + VSCode palette/codicons</sub><br/><br/>
      <img src="docs/images/gallery-dark.png" alt="gallery（深色）" width="100%"/>
      <img src="docs/images/gallery-light.png" alt="gallery（浅色）" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_hero</b><br/>
      <sub>品牌 hero 页，双行大标题 + 内联 <code>TextRun</code> accent 高亮</sub><br/><br/>
      <img src="docs/images/jtui_hero-dark.png" alt="jtui_hero（深色）" width="100%"/>
      <img src="docs/images/jtui_hero-light.png" alt="jtui_hero（浅色）" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_cinema</b><br/>
      <sub>视频轮播：5 张几何缩略图 + ease-in-out 滑动 + 真 <code>VideoPlayer</code> 播放</sub><br/><br/>
      <img src="docs/images/jtui_cinema-dark.png" alt="jtui_cinema（深色）" width="100%"/>
      <img src="docs/images/jtui_cinema-light.png" alt="jtui_cinema（浅色）" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_studio</b><br/>
      <sub>媒体工作台：<code>VideoPlayer</code> + <code>AudioPlayer</code> + <code>WaveformView</code> 跨 rebuild 保活</sub><br/><br/>
      <img src="docs/images/jtui_studio-dark.png" alt="jtui_studio（深色）" width="100%"/>
      <img src="docs/images/jtui_studio-light.png" alt="jtui_studio（浅色）" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_invest</b><br/>
      <sub>金融落地页：毛玻璃 NavBar + count-up 动画 + ScrollView</sub><br/><br/>
      <img src="docs/images/jtui_invest-dark.png" alt="jtui_invest（深色）" width="100%"/>
      <img src="docs/images/jtui_invest-light.png" alt="jtui_invest（浅色）" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_atlas</b><br/>
      <sub>KPI sparkline + 图表卡片 + 网格背景</sub><br/><br/>
      <img src="docs/images/jtui_atlas-dark.png" alt="jtui_atlas（深色）" width="100%"/>
      <img src="docs/images/jtui_atlas-light.png" alt="jtui_atlas（浅色）" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_live</b><br/>
      <sub>代码编辑器 mock + hot-reload 状态卡</sub><br/><br/>
      <img src="docs/images/jtui_live-dark.png" alt="jtui_live（深色）" width="100%"/>
      <img src="docs/images/jtui_live-light.png" alt="jtui_live（浅色）" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>jtui_pro</b><br/>
      <sub>企业 SSO 落地页：贝塞尔流线 + trust 徽章</sub><br/><br/>
      <img src="docs/images/jtui_pro-dark.png" alt="jtui_pro（深色）" width="100%"/>
      <img src="docs/images/jtui_pro-light.png" alt="jtui_pro（浅色）" width="100%"/>
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <b>jtui_pulse</b><br/>
      <sub>动态图表背景，自定义 <code>clips_self()</code> widget</sub><br/><br/>
      <img src="docs/images/jtui_pulse-dark.png" alt="jtui_pulse（深色）" width="100%"/>
      <img src="docs/images/jtui_pulse-light.png" alt="jtui_pulse（浅色）" width="100%"/>
    </td>
    <td width="50%" align="center">
      <b>folders_app</b><br/>
      <sub>文件管理：sidebar nav + 文件夹网格 + search 筛选</sub><br/><br/>
      <img src="docs/images/folders_app-dark.png" alt="folders_app（深色）" width="100%"/>
      <img src="docs/images/folders_app-light.png" alt="folders_app（浅色）" width="100%"/>
    </td>
  </tr>
</table>

## 项目状态

jtUI 正在朝 `1.0` 推进。当前状态：

- **稳定**：渲染管线（D2D 1.1）、widget 目录、主题 + i18n、动画 tick、整树 rebuild 状态模型、构建（Windows 原生 + MinGW 交叉）
- **开发中**：GPU texture registry、ScrollView 横向滚动、AnimationState 抽象
- **路线图**：macOS（SDL3 + Skia 后端）、Linux 原生（GTK4 或 SDL3）

## License

[MIT 协议](LICENSE)发布。欢迎商业使用 —— 自由 fork、构建产品，无需版税。

## Credits

- **jtai 团队**，由曾能混（`jwhna1@gmail.com`）主导。项目 2025 年 3 月启动，目标是把 Vue 般的开发体感带到原生 C++ 桌面开发。
- **图标**：[`@vscode/codicons`](https://github.com/microsoft/vscode-codicons)（MIT）—— 649 个命名 glyph 内嵌到二进制，通过 `IDWriteFactory5` 自定义字体集合加载。

## 贡献

项目处于预发布阶段。API 还在 `1.0` 稳定化过程中，暂不接受外部 PR。欢迎在 Issue 区报 bug + 提需求场景 + 设计讨论，附上你的使用场景。详见 [`CONTRIBUTING.md`](CONTRIBUTING.md) 与 [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md)。

---

<div align="center">
<sub>Built with C++20 · No Electron · No JS bridge · One <code>.exe</code> to ship.</sub>
</div>
