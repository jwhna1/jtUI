# jtUI 平台移植指南

本文档说明 jtUI v1 的平台抽象现状,以及把 jtUI 移植到 Win32 之外的平台
(macOS / Linux X11 / Linux Wayland)需要做什么。

## 现状(v1)

jtUI 已经把"渲染端"和"应用层"做了语义分层:

| 层 | 文件 | 平台无关性 |
|---|---|---|
| `runtime/foundation` | geometry / color / pixel_buffer | 100% 平台无关 |
| `runtime/object` | Node / Element / Widget / DirtyFlags | 100% 平台无关 |
| `runtime/property` | Property / Signal / Command | 100% 平台无关 |
| `runtime/event` | Capture/Target/Bubble dispatch + path resolve | 100% 平台无关 |
| `runtime/layout` | FlexBox / Grid / Length / Edges | 100% 平台无关 |
| `runtime/render` | PaintContext + DrawCommand | 100% 平台无关(命令 IR) |
| `runtime/theme` | SemanticColor + ThemeMode | 100% 平台无关 |
| `runtime/media` | TextureHandle / FrameTypes / Decoder 接口 + null fallback | 接口无关,后端 Win32 |
| `widgets/*` | Panel / Text / Button / TextInput / ListView / RealtimeCanvas | 100% 平台无关 |
| `runtime/app` | Application / Window / D2D 渲染 + WndProc | **Win32-only(主循环 / 渲染 replay)** |
| `platform/win32` | MF / WASAPI 媒体后端 | Windows 特定 |

**已经验证 Linux 原生 cmake 可以编译整个 jtUI**(`runtime/app` 中 `#if !defined(_WIN32)`
分支让 `Application::run()` return 0,window 创建是 noop)。所以 unit tests 和
不依赖窗口的逻辑测试在 Linux 可跑。

## 需要做什么才能让 jtUI 在新平台运行 GUI

`runtime/app/src/application.cpp` 1900+ 行,所有平台特定代码集中在这里
(8 处 `#if defined(_WIN32)` 块)。具体职责:

1. **窗口创建 + lifecycle**(WindowOptions → 平台原生窗口 handle)
2. **主消息循环**(GetMessage/TranslateMessage/DispatchMessage 等价物)
3. **WndProc 等效物**:WM_PAINT / WM_SIZE / WM_KEYDOWN / WM_LBUTTONDOWN /
   WM_MOUSEMOVE / WM_MOUSEWHEEL / WM_CHAR / WM_NCHITTEST / WM_GETMINMAXINFO 等
4. **PaintContext IR 回放**(把 `runtime/render` 输出的 DrawCommand 列表画到
   原生 surface)— 当前 Win32 用 Direct2D + DirectWrite
5. **Application::post 唤醒**(跨线程往 UI 投递 lambda;Win32 用 PostMessageW
   + 自定义 WM_APP 消息)
6. **frameless 窗口的 NC 区域命中测试**(WM_NCHITTEST 把 caption 区识别为
   HTCAPTION,边框为 HTLEFT/HTRIGHT/... 让系统接管 drag/resize)
7. **多媒体后端**(`platform/win32/src/media/` 的 MF + WASAPI;Linux 端有
   `runtime/media/src/null_decoder.cpp` fallback)

## 推荐的移植拓扑

把 `runtime/app/src/application.cpp` 拆成两层:

```
runtime/app/include/hui/app/application.hpp   ← 公共 API(无变化)
runtime/app/src/application_core.cpp           ← 平台无关:tick / dispatch / dirty 管理
runtime/app/include/hui/app/native_backend.hpp ← 接口:INativeBackend
platform/win32/src/native_backend_win32.cpp    ← Win32 实现
platform/cocoa/src/native_backend_cocoa.mm     ← (新)macOS 实现
platform/x11/src/native_backend_x11.cpp        ← (新)Linux X11 实现
```

`INativeBackend` 接口至少需要:

```cpp
struct INativeBackend {
    virtual ~INativeBackend() = default;

    // Window lifecycle
    virtual NativeWindowHandle create_window(const WindowOptions& opts) = 0;
    virtual void destroy_window(NativeWindowHandle h) = 0;
    virtual SizeF window_size(NativeWindowHandle h) = 0;
    virtual void invalidate(NativeWindowHandle h) = 0;

    // Main loop
    virtual int run_message_loop(Application& app) = 0;
    virtual void post_wakeup() = 0;  // 跨线程唤醒

    // Render replay
    virtual void paint_window(NativeWindowHandle h,
                              const std::vector<DrawCommand>& commands) = 0;
};
```

平台特定的 WindowFrame 视觉(frameless 边框 / 圆角 / shadow)目前在 widgets
层用 RealtimeCanvas 自画,可继续保持平台无关(只要后端能渲染 fill_rect /
fill_rounded_rect 就行)。

## 各平台路径估算

| 平台 | 主要工作 | 预估工时 |
|---|---|---|
| **macOS** | Cocoa NSWindow + CALayer + Core Graphics replay + NSEvent → key/mouse | 2-3 周 |
| **Linux X11** | xcb/Xlib window + Cairo / pixman 软件 raster + XInput2 | 2-3 周 |
| **Linux Wayland** | wl_surface + EGL + Skia 或 Cairo + libinput | 3-4 周(协议复杂) |

D2D PathGeometry / LinearGradientBrush 这些原语在 Cairo 都有对应,Core
Graphics 也有(CGContextAddCurveToPoint / CGGradient)。最难的部分通常是
DirectWrite 的中文/CJK 排版换成平台等效物(macOS = CoreText,Linux = HarfBuzz
+ Pango 或 FreeType)。

## 当前 Linux build 用法

```bash
cmake -S /home/jz/jtUI -B build-linux
cmake --build build-linux -j
./build-linux/tests/hui_unit_tests       # 通过
./build-linux/examples/gallery/gallery   # 跑起来,但 Application::run() 立刻 return 0
```

测试用的 unit tests 在 Linux 跑全绿,验证 `runtime/object` / `runtime/event` /
`runtime/property` / `runtime/layout` 都不依赖 Win32。
