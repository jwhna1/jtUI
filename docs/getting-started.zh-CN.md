# jtUI 快速上手

一份 30 分钟教程，从 "Hello World" 一路写到一个完整的品牌 hero 页。读完本教程你会知道如何：

1. 在 Windows（原生）或 Linux（交叉编译到 Windows）上 build 并运行一个 jtUI app。
2. 用 `Panel`、`Text`、`Button` 等内置 widget 组合 UI。
3. 一行切换主题（Dark/Light）和语言（En/Zh）。
4. 处理点击、hover 和键盘事件。
5. 写自己的自定义 widget，通过 `PaintContext` 绘制几何图形。
6. 用内置的 `tick(delta)` 循环驱动 widget 动画。

如果你要的是完整的 API 参考，直接跳到 [`api-reference.md`](api-reference.md)。想了解内部机制（渲染管线、dirty 传播、事件分发）见 [`architecture.md`](architecture.md)。

---

## 0. 准备工作

你需要：

- **CMake 3.25+**
- **一个 C++20 编译器**：
  - Windows 上：`clang++ 18+`（推荐）或 MSVC v143+
  - Linux 交叉编译：带 POSIX threads 的 `mingw-w64`（`x86_64-w64-mingw32-g++-posix`）
- **Ninja**（项目默认使用 Ninja generator）

Linux 端目前只支持交叉编译和单元测试 —— Linux 上 `Application::run()` 会立即返回 0，因为还没有 GTK/X11 后端（见 [路线图](#9-next-steps)）。

### Clone 并首次 build

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

你应该会看到 `gallery` 窗口 —— 官方组件展厅。点击各个 tab 可以看到 jtUI 自带的每一个 widget。

---

## 1. Hello World

新建一个目录 `examples/hello/`，里面放两个文件：

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
    options.frameless = true;             // 自定义标题栏
    options.size      = {kWidth, kHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(jtui::Color::from_hex("#050505"));
    root->set_corner_radius(0.0F);
    root->set_frame({0.0F, 0.0F, kWidth, kHeight});

    // 自定义标题栏（拖动移动 + 最小化/最大化/关闭由 jtUI 处理）
    auto window_frame = std::make_unique<jtui::WindowFrame>();
    window_frame->set_frameless(true);
    window_frame->set_frame({0.0F, 0.0F, kWidth, kHeight});
    root->append_child(std::move(window_frame));

    auto title_bar = std::make_unique<jtui::TitleBar>("Hello jtUI");
    title_bar->set_frame({0.0F, 0.0F, kWidth, 36.0F});
    root->append_child(std::move(title_bar));

    // 居中大号问候语
    auto greet = std::make_unique<jtui::Text>("Hello, jtUI!");
    greet->set_font_size(48.0F);
    greet->set_bold(true);
    greet->set_color(jtui::Color::from_hex("#FB923C"));
    greet->set_alignment(jtui::TextAlignment::Center);
    greet->set_frame({0.0F, 220.0F, kWidth, 60.0F});
    root->append_child(std::move(greet));

    // Get Started 按钮
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
        // 业务回调写这里。可以试试弹出 Dialog、跳转页面等等。
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

挂到顶层 `CMakeLists.txt`：

```cmake
if(HUI_BUILD_EXAMPLES)
    add_subdirectory(examples/gallery)
    # ...other examples...
    add_subdirectory(examples/hello)   # add this line
endif()
```

build 一下：

```bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw --target hello
# Run build-mingw/examples/hello/hello.exe on Windows.
```

### 你刚刚学到了什么

- **`jtui::Application`** 持有消息循环。**`Window`** 是 OS 窗口句柄。
- **Widget 树** 自下而上构建：`std::make_unique<T>()` 再 `parent->append_child(std::move(child))`。所有权向上流动 —— 父节点拥有所有后代。
- **所有坐标都是绝对坐标**（逻辑像素，DPI 感知）。`set_frame({x, y, w, h})` 把 widget 放到窗口内任意位置。父节点不会自动平移子节点 —— 这是有意为之，让模型保持简单。
- **颜色** 用 `jtui::Color`（float RGBA 0..1）或通过 `from_hex("#RRGGBB")` 构造。
- **点击处理器** 通过 `Signal<>` 连接；`on_clicked().connect(lambda)` 是惯用写法。

---

## 2. 状态与重建

UI 需要状态。jtUI 默认的状态模型是 **整树 rebuild**：把状态放在一个普通 struct 里，写一个 `build_root()` 函数从当前状态构建整棵 widget 树，每次状态变化都重新调用一次。

听起来很慢，但其实非常快 —— 重建一个典型界面只要约 1 ms，因为 Panel/Text/Button 都很轻量，而且 dirty-rect 重绘只会重画变化的部分。

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

    // 自引用的 rebuild 闭包。用 shared_ptr 包一层，让 lambda 能引用自己。
    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, state, rebuild_holder]() {
        window.set_content(build_root(*state, *rebuild_holder));
    };

    window.set_content(build_root(*state, *rebuild_holder));
    return app.run();
}
```

### 关键概念

- **`AppState` 是纯数据** —— 没有 widget，没有 signal。干净，易于推理。
- **`build_root(state, rebuild)`** 是纯函数：相同 state → 相同树。
- **点击回调修改 state + 调 `rebuild()`**。Rebuild 调用 `window.set_content(new_root)`，整棵 widget 树被替换。
- **旧的 widget 树不会立即销毁** —— 它会进入一个 `pending_destroy_` 队列，等当前事件处理栈完全退出之后再释放。这就是为什么"在点击处理器里改 state 再 rebuild"是安全的（即便点击事件来自一个马上要被销毁的 widget，也不会出现 use-after-free）。细节见 `Window::set_content` 内部实现。

### 什么时候 **不** 该 rebuild

整树 rebuild 是默认方案，但不是唯一选择：

- **动画 widget**（自定义 `tick()`）自己重绘，不重建树（见第 7 节）。
- **`ScrollView`** 通过 `translate_subtree` 滚动，不 rebuild。
- **重型 widget**（`VideoPlayer`、`AudioPlayer`）可以用 `release_child<T>` 跨 rebuild 保活，保留解码器状态。`PersistentWidgets` 模式见 `examples/jtui_studio/main.cpp`。

---

## 3. 布局

jtUI 默认使用 **绝对 frame 坐标**。每个 widget 都放在窗口空间内的 `{x, y, w, h}`。简单可预测，但意味着位置要手算。

更复杂的布局有三个选项：

### 选项 A：绝对计算（默认，所有 `examples/` 都在用）

```cpp
constexpr float kNavBarH = 60.0F;
constexpr float kSidePad = 80.0F;
const float right_x = kWindowWidth - kSidePad;
const float btn_y   = kTitleBarH + (kNavBarH - 38.0F) * 0.5F;

cta->set_frame({right_x - 130.0F, btn_y, 130.0F, 38.0F});
```

啰嗦但显式。在你拿到设计稿（带具体的 padding 和间距）的时候非常好用。

### 选项 B：FlexBox

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

类 CSS flex 的行为。适合导航栏、按钮组、设置项行。

### 选项 C：ScrollView

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

需要手动调用一次 `arrange()`，因为窗口只会对根节点 arrange 一次。ScrollView 目前只支持纵向 —— 横向模式在路线图里。

---

## 4. 主题与 i18n

jtUI 自带一套基于 token 的主题系统。深浅切换或者中英切换只要一行；配合整树 rebuild，整个 app 就重新换装。

### 品牌 token（每个 example 一份的模式）

每个 example 自己定义一个 `brand_tokens.hpp`：

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
    // ...按需添加更多...
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#050505"),
    .accent        = jtui::Color::from_hex("#FB923C"),
    // ...
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#FAFAFA"),
    .accent        = jtui::Color::from_hex("#EA580C"),  // 浅色背景上更深
    // ...
};

[[nodiscard]] inline const Palette& active() noexcept {
    return jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark
         ? dark : light;
}

}  // namespace my_app::brand
```

在你的 `build_root` 开头写 `const auto& p = my_app::brand::active();`，然后引用 `p.accent`、`p.bg_panel` 等。切主题只要重新调一次 `build_root` 就行，因为 `active()` 会读取 `Theme::mode()`。

### 切换主题

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

### i18n 两步走

**第一步**：启动时一次性注册字符串：

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

**第二步**：build 时查表：

```cpp
using jtui::i18n::tr;
auto tagline = std::make_unique<jtui::Text>(tr("hero.tagline"));
```

切换语言：

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

## 5. 事件：click、hover、键盘

大多数 widget 通过 `Signal<...>` 暴露语义事件：

```cpp
button->on_clicked().connect([](){ /*...*/ });
button->on_hover_changed().connect([](bool hovered){ /*...*/ });

switch_widget->on_changed().connect([](bool checked){ /*...*/ });

text_input->on_text_changed().connect([](std::string_view text){ /*...*/ });
text_input->on_submit().connect([](){ /*...*/ });

tabs->on_index_changed().connect([](int idx){ /*...*/ });
```

自定义 widget 则通过 override 虚 hook 接入：

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

事件分发走 `Capture → Target → Bubble`。你可以 override 底层入口 `on_event(Event&)` 在任意阶段拦截 —— 参考 `widgets/common/src/dialog.cpp` 里 Dialog 的模态遮罩技巧。

---

## 6. 自定义 widget：发光 Counter

到时候写你的第一个自定义 widget 了。我们做一个 `GlowCounter`，绘制一个数字，背后带一圈柔和的 accent 光晕。

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

        // 柔和光晕：数字背后一个半透明椭圆。
        const float cx = f.x + f.width * 0.5F;
        const float cy = f.y + f.height * 0.5F;
        const float halo_r = std::min(f.width, f.height) * 0.45F;
        const jtui::Color halo{accent_.r, accent_.g, accent_.b, 0.18F};
        ctx.fill_ellipse(
            {cx - halo_r, cy - halo_r, halo_r * 2.0F, halo_r * 2.0F},
            halo);

        // 数字画在上面。
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

用它：

```cpp
auto counter = std::make_unique<GlowCounter>();
counter->set_count(state.counter);
counter->set_accent(p.accent);
counter->set_frame({340, 220, 120, 120});
root->append_child(std::move(counter));
```

### `PaintContext` 速查表

| 调用 | 效果 |
|---|---|
| `fill_rect(bounds, color)` | 实心矩形 |
| `stroke_rect(bounds, color, thickness)` | 描边矩形 |
| `fill_rounded_rect(bounds, color, radius)` | 圆角填充 |
| `stroke_rounded_rect(bounds, color, radius, thickness)` | 圆角描边 |
| `fill_ellipse(bounds, color)` | 实心椭圆 / 圆 |
| `stroke_ellipse(bounds, color, thickness)` | 描边椭圆 |
| `line(p0, p1, color, thickness)` | 单条直线 |
| `draw_bezier(p0, c1, c2, p3, color, thickness)` | 三次贝塞尔曲线 |
| `fill_polygon({pts}, color)` | 填充多边形，自动闭合 |
| `fill_gradient_rect(bounds, top, bottom, radius)` | 线性渐变 top→bottom |
| `fill_shadow(bounds, radius, offset, blur, spread, color)` | 类 CSS box shadow |
| `fill_backdrop_blur(bounds, radius, blur, tint)` | 毛玻璃 |
| `draw_text(bounds, text, color, align, size, bold)` | 渲染文本 |
| `draw_texture(bounds, pixel_buffer, alpha_mode)` | 绘制 RGBA 像素 |
| `push_clip(bounds)` / `pop_clip()` | 手动矩形裁剪 |

复杂几何范例见 `examples/jtui_cinema/thumbnail_art.hpp`（5 个地域主题插画）和 `examples/jtui_invest/animated_widgets.hpp`（动画图表）。

---

## 7. 动画

override `tick(float delta)`，只要你想继续播放就返回 `true`：

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

只要树里有任何 widget 的 `tick()` 返回 `true`，`Application` 就会保持 `WM_TIMER` 驱动的 60 fps tick 循环。没有东西在动时循环自动停止，所以空闲帧不收费。

### 缓动函数

常用 easing（直接复制进你的 widget）：

```cpp
static float ease_out_cubic(float t) { float i = 1 - t; return 1 - i*i*i; }
static float ease_in_out_cubic(float t) {
    if (t < 0.5F) return 4 * t * t * t;
    const float p = -2 * t + 2;
    return 1 - p * p * p * 0.5F;
}
```

轮播式的 "从 A 滑到 B" 动画，优先用 `ease_in_out_cubic` —— 完整实现见 `examples/jtui_cinema/carousel_animator.hpp`，里面有一个关键细节：jtUI 在 `WM_LBUTTONUP` 上同步 tick 一帧消除按钮反馈延迟，弹簧式动画会在第 1 帧跳 `diff × k` 像素（可能 50+ 像素，看起来像"先瞬移再滑"）。`progress + ease-in-out` 在第 1 帧只走 ~0.2 px —— 顺滑如丝。

### 重绘不留残影

常见的动画陷阱：反复调用 `set_frame` 移动 widget 会触发 dirty-rect 重绘，但 dirty rect 只覆盖 **新** 位置。**旧** 位置的像素留在屏幕上 → "残影"。

修复办法是用 `translate_subtree(dx, dy)`（它不会 mark dirty）加上对某个 `frame` 覆盖整个动画区域的容器 widget 调一次 `mark_dirty(Paint)`：

```cpp
class CarouselAnimator : public hui::Widget {
public:
    void add_target(hui::Element* w) { targets_.push_back(w); }

    bool tick(float dt) override {
        const float dx = compute_dx_from_progress(dt);
        for (auto* w : targets_) w->translate_subtree(dx, 0);
        mark_dirty(hui::DirtyFlags::Paint);  // animator.frame 覆盖整个区域
        return progress_ < 1.0F;
    }
};
```

把 animator 的 `frame` 设到覆盖整个轮播 / 可滚动区域。`mark_dirty(Paint)` 会让整个区域重绘，旧像素也就被干净抹掉了。

---

## 8. 串起来：一个品牌 hero 页

把主题 + i18n + 自定义 widget + 动画组合成一个完整的品牌 example。模式：

```
examples/my_brand/
├── brand_tokens.hpp     // Palette + active()
├── i18n_catalog.hpp     // register_strings()
├── main.cpp             // build_root + run_app
└── CMakeLists.txt
```

一个最小 `build_root`：

```cpp
std::unique_ptr<jtui::Panel> build_root(AppState& state, const RebuildFn& rebuild) {
    const auto& p = my_brand::brand::active();
    using jtui::i18n::tr;

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(p.bg_window);
    root->set_frame({0, 0, kWindowWidth, kWindowHeight});

    // 标准窗口外壳（无边框 + 自定义标题栏）
    auto wf = std::make_unique<jtui::WindowFrame>();
    wf->set_frameless(true);
    wf->set_frame({0, 0, kWindowWidth, kWindowHeight});
    root->append_child(std::move(wf));

    auto tb = std::make_unique<jtui::TitleBar>(tr("window.title"));
    tb->set_frame({0, 0, kWindowWidth, kTitleBarH});
    root->append_child(std::move(tb));

    build_navbar(*root, state, rebuild);   // 主题 + 语言切换
    build_hero(*root, state);              // 大标题 + CTA
    build_features(*root, state);          // 三列特性网格
    build_footer(*root);

    return root;
}
```

完整范例可以参考 `examples/` 里 9 个品牌 example 中任意一个：

- **`jtui_hero`** —— 极简 hero 页：标题 + 3 个特性 + 双 CTA
- **`jtui_cinema`** —— 轮播 + 真视频播放 + 自定义几何 widget
- **`jtui_invest`** —— 可滚动 dashboard + 毛玻璃导航 + 动画
- **`folders_app`** —— 侧边导航 + 文件夹网格 + 搜索筛选
- **`jtui_studio`** —— 媒体 app：`VideoPlayer` + `AudioPlayer` + `WaveformView`

全部十个 example 都内置同一个 `About` 弹窗（通过 `hui::install_about_card`） —— 把这套模式抄进你的 app，就能给用户一个快速的"关于本项目"面板。

---

## 9. 下一步

现在你已经能 build jtUI app 了，接下来可以去这些地方：

- **[API 参考](api-reference.md)** —— 每个公开类型和函数，带签名和示例。
- **[架构说明](architecture.md)** —— 渲染管线、dirty 传播、事件分发、主题 token 的底层工作原理。如果你想写框架级 widget 或贡献 runtime，先读这一篇。

### 可以照抄的常见模式

| 模式 | 示例文件 |
|---|---|
| 品牌 palette + 深浅双主题 | `examples/jtui_hero/brand_tokens.hpp` |
| 用 `register_entries` 注册 i18n | `examples/jtui_cinema/i18n_catalog.hpp` |
| `PersistentWidgets` 跨 rebuild 保活 `VideoPlayer` | `examples/jtui_studio/main.cpp` |
| ease-in-out 轮播 animator | `examples/jtui_cinema/carousel_animator.hpp` |
| 带 `clips_self()` 的自定义几何 widget | `examples/jtui_cinema/thumbnail_art.hpp` |
| 毛玻璃导航 | `examples/jtui_invest/main.cpp` |
| count-up / draw-in / grow-in 动画 | `examples/jtui_invest/animated_widgets.hpp` |
| 安装 About 弹窗（框架级） | `widgets/common/include/hui/widgets/common/about_card.hpp` |

### 路线图

后续版本即将到来的内容：

- ScrollView 横向模式（`set_orientation(Horizontal)`）
- AnimationState 抽象（跨 rebuild 保存动画进度）
- `clips_self_rounded(corner_radius)` 真圆角裁剪
- macOS 后端：SDL3 + Skia
- Linux 原生后端：GTK4 或 SDL3

完整列表见项目 backlog。

---

如果你用 jtUI 做出了什么，我们很想看看。项目由 jtai 团队维护（`曾能混 <jwhna1@gmail.com>`）—— 欢迎报 issue、聊设计、晒作品。

Happy hacking。

