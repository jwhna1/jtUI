#include "hui/app/application.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hui/event/dispatcher.hpp"
#include "hui/foundation/log.hpp"
#include "hui/render/paint_context.hpp"

#if defined(_WIN32)
#include <d2d1.h>
#include <d2d1_1.h>            // D2D 1.1 / ID2D1Factory1 / ID2D1DeviceContext
#include <d2d1effects.h>       // CLSID_D2D1Shadow + D2D1_SHADOW_PROP_*
#include <dwmapi.h>
#include <dwrite.h>
#include <imm.h>
#include <mmsystem.h>
#include <objidl.h>
#include <shellapi.h>
#include <timeapi.h>
#include <windows.h>
#include <windowsx.h>

#include "hui/foundation/codicon_font.hpp"
#include "hui/platform/win32/codicon_font_loader.hpp"
#endif

namespace hui {

struct WindowRuntimeAccess {
    static void*& native_handle(Window& window) {
        return window.native_handle_;
    }

    static std::unique_ptr<Widget>& content(Window& window) {
        return window.content_;
    }

    static void*& back_buffer_dc(Window& window) {
        return window.back_buffer_dc_;
    }

    static void*& back_buffer_bitmap(Window& window) {
        return window.back_buffer_bitmap_;
    }

    static void*& back_buffer_old_bitmap(Window& window) {
        return window.back_buffer_old_bitmap_;
    }

    static int& back_buffer_width(Window& window) {
        return window.back_buffer_width_;
    }

    static int& back_buffer_height(Window& window) {
        return window.back_buffer_height_;
    }

    static Widget*& hovered_widget(Window& window) {
        return window.hovered_widget_;
    }

    static Widget*& pressed_widget(Window& window) {
        return window.pressed_widget_;
    }

    static Widget*& focused_widget(Window& window) {
        return window.focused_widget_;
    }

    static float& dpi_scale(Window& window) {
        return window.dpi_scale_;
    }

    static bool& animation_timer_active(Window& window) {
        return window.animation_timer_active_;
    }

    static bool& deferred_repaint_pending(Window& window) {
        return window.deferred_repaint_pending_;
    }

    static PaintContext& paint_context(Window& window) {
        return window.paint_context_;
    }

    static std::int64_t& last_tick_qpc(Window& window) {
        return window.last_tick_qpc_;
    }

    static float& arranged_width(Window& window) {
        return window.arranged_width_;
    }

    static float& arranged_height(Window& window) {
        return window.arranged_height_;
    }
};

#if defined(_WIN32)
namespace {

constexpr wchar_t kWindowClassName[] = L"jtUI.Window";
constexpr UINT kAnimationTimerId = 1U;
constexpr UINT kAnimationTimerIntervalMs = 16U;
constexpr UINT kDeferredRepaintMessage = WM_APP + 1U;
constexpr UINT kPendingCallbackMessage = WM_APP + 2U;

std::wstring utf8_to_wide(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), nullptr, 0);

    if (required <= 0) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        wide.data(), required);
    return wide;
}

// v1.22 (2026-05-04): wide -> utf8。
// 给 WM_DROPFILES 取出来的文件名 (Win32 widechar) 转 utf8 走 jtui 字符串通道。
std::string wide_to_utf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                              static_cast<int>(text.size()),
                                              nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        utf8.data(), required, nullptr, nullptr);
    return utf8;
}

const IID kIDWriteFactoryId{
    0xb859ee5a,
    0xd838,
    0x4b5b,
    {0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48},
};

class DirectTextSession {
    struct TextFormatEntry {
        int font_size_key;
        bool bold;
        TextAlignment alignment;
        std::string font_family;  // 空 = 默认 Segoe UI 路径; "codicon" 等走自定义字体
        IDWriteTextFormat* format;
    };

    struct BrushEntry {
        std::uint32_t color_key;
        ID2D1SolidColorBrush* brush;
    };

  public:
    DirectTextSession() {
        d2d_status_ = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory_);
        dwrite_status_ = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, kIDWriteFactoryId,
                                             reinterpret_cast<IUnknown**>(&dwrite_factory_));
        // 注册 Codicon 内嵌字体到 DirectWrite, 让 CreateTextFormat("codicon", ...)
        // 能用; 失败不影响主流程, Icon widget 会画占位 rect 让人眼可见。
        // 返回值刻意忽略 -- 失败由 codicon_collection() 返回 nullptr 体现。
        if (SUCCEEDED(dwrite_status_) && dwrite_factory_ != nullptr) {
            (void)hui::platform::win32::codicon_register(dwrite_factory_);
        }
    }

    ~DirectTextSession() {
        for (const BrushEntry& entry : brushes_) {
            if (entry.brush != nullptr) {
                entry.brush->Release();
            }
        }

        for (const TextFormatEntry& entry : text_formats_) {
            if (entry.format != nullptr) {
                entry.format->Release();
            }
        }

        // Effect 依赖 device context，先于它释放。
        if (shadow_effect_ != nullptr) {
            shadow_effect_->Release();
        }
        if (blur_effect_ != nullptr) {
            blur_effect_->Release();
        }

        if (device_context_ != nullptr) {
            device_context_->Release();
        }

        if (dc_render_target_ != nullptr) {
            dc_render_target_->Release();
        }

        if (dwrite_factory_ != nullptr) {
            dwrite_factory_->Release();
        }

        if (d2d_factory_ != nullptr) {
            d2d_factory_->Release();
        }
    }

    [[nodiscard]] bool ok() const noexcept {
        return SUCCEEDED(d2d_status_) && SUCCEEDED(dwrite_status_) && d2d_factory_ != nullptr &&
               dwrite_factory_ != nullptr;
    }

    [[nodiscard]] ID2D1Factory1* d2d_factory() const noexcept {
        return d2d_factory_;
    }

    // v0.4 (2026-05-15): D2D 1.1 DeviceContext 入口，用于 CreateEffect / DrawImage
    // 等 Effects 接口。dc_render_target() 首次调用后 QI 缓存；如果 D2D 1.1
    // 不可用（理论上 Win10 起 100% 可用），返回 nullptr，shadow 命令降级跳过。
    [[nodiscard]] ID2D1DeviceContext* device_context() const noexcept {
        return device_context_;
    }

    // v1.23.1 (2026-05-15): 懒创建 + 缓存的 Effect。每帧复用同一对象，
    // 只 SetInput / SetValue，省掉 CreateEffect / Release 的每帧开销。
    [[nodiscard]] ID2D1Effect* shadow_effect() {
        if (shadow_effect_ == nullptr && device_context_ != nullptr) {
            device_context_->CreateEffect(CLSID_D2D1Shadow, &shadow_effect_);
        }
        return shadow_effect_;
    }
    [[nodiscard]] ID2D1Effect* blur_effect() {
        if (blur_effect_ == nullptr && device_context_ != nullptr) {
            device_context_->CreateEffect(CLSID_D2D1GaussianBlur, &blur_effect_);
        }
        return blur_effect_;
    }

    [[nodiscard]] IDWriteFactory* dwrite_factory() const noexcept {
        return dwrite_factory_;
    }

    [[nodiscard]] IDWriteTextFormat* text_format(float font_size, bool bold,
                                                 TextAlignment alignment,
                                                 const std::string& font_family = {}) {
        const int font_size_key = static_cast<int>(font_size * 100.0F + 0.5F);
        for (const TextFormatEntry& entry : text_formats_) {
            if (entry.font_size_key == font_size_key && entry.bold == bold &&
                entry.alignment == alignment && entry.font_family == font_family) {
                return entry.format;
            }
        }

        // font_family 空 = 默认 "Segoe UI" (DirectWrite 自动按 fallback 链找中文 / 彩色 emoji 字形)
        // font_family 非空 = 调用方指定的自定义字体 (如 "codicon"), 需要 platform 层已注册
        std::wstring family_wide;
        const wchar_t* family_ptr = L"Segoe UI";
        if (!font_family.empty()) {
            family_wide.reserve(font_family.size());
            for (char ch : font_family) {
                family_wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
            }
            family_ptr = family_wide.c_str();
        }

        // 自定义字体 (如 codicon) 走专属 IDWriteFontCollection。系统字体 (默认 / Segoe UI)
        // 用 DirectWrite 系统集合 (nullptr 即默认)。
        IDWriteFontCollection* font_collection = nullptr;
        if (font_family == hui::foundation::kCodiconFontFamily) {
            font_collection = hui::platform::win32::codicon_collection();
        }

        IDWriteTextFormat* format = nullptr;
        const HRESULT status = dwrite_factory_->CreateTextFormat(
            family_ptr, font_collection,
            bold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            static_cast<float>(font_size_key) / 100.0F, L"zh-cn", &format);
        if (FAILED(status) || format == nullptr) {
            return nullptr;
        }

        DWRITE_TEXT_ALIGNMENT dwrite_align = DWRITE_TEXT_ALIGNMENT_LEADING;
        if (alignment == TextAlignment::Center) dwrite_align = DWRITE_TEXT_ALIGNMENT_CENTER;
        else if (alignment == TextAlignment::Trailing) dwrite_align = DWRITE_TEXT_ALIGNMENT_TRAILING;
        format->SetTextAlignment(dwrite_align);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        IDWriteInlineObject* trimming_sign = nullptr;
        DWRITE_TRIMMING trimming{
            DWRITE_TRIMMING_GRANULARITY_CHARACTER,
            0,
            0,
        };
        if (SUCCEEDED(dwrite_factory_->CreateEllipsisTrimmingSign(format, &trimming_sign))) {
            format->SetTrimming(&trimming, trimming_sign);
            trimming_sign->Release();
        }

        text_formats_.push_back(TextFormatEntry{font_size_key, bold, alignment, font_family, format});
        return format;
    }

    [[nodiscard]] ID2D1DCRenderTarget* dc_render_target(HDC hdc, const RECT& target_rect) {
        if (target_rect.right <= target_rect.left || target_rect.bottom <= target_rect.top) {
            return nullptr;
        }

        if (dc_render_target_ == nullptr) {
            D2D1_RENDER_TARGET_PROPERTIES target_properties = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0F, 96.0F,
                D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

            const HRESULT status =
                d2d_factory_->CreateDCRenderTarget(&target_properties, &dc_render_target_);
            if (FAILED(status) || dc_render_target_ == nullptr) {
                return nullptr;
            }

            // D2D 1.1 起所有 RenderTarget 都实现 ID2D1DeviceContext 接口。QI 失败
            // 视为运行环境不支持 1.1（Win10+ 理论 100% 可用），device_context_ 留
            // nullptr，shadow 命令降级跳过不画。
            dc_render_target_->QueryInterface(
                __uuidof(ID2D1DeviceContext),
                reinterpret_cast<void**>(&device_context_));
        }

        if (FAILED(dc_render_target_->BindDC(hdc, &target_rect))) {
            return nullptr;
        }

        return dc_render_target_;
    }

    [[nodiscard]] ID2D1SolidColorBrush* solid_brush(ID2D1DCRenderTarget* render_target,
                                                    Color color) {
        if (render_target == nullptr) {
            return nullptr;
        }

        const std::uint32_t key = color_key(color);
        for (const BrushEntry& entry : brushes_) {
            if (entry.color_key == key) {
                return entry.brush;
            }
        }

        ID2D1SolidColorBrush* brush = nullptr;
        const HRESULT status = render_target->CreateSolidColorBrush(
            D2D1::ColorF(color.r, color.g, color.b, color.a), &brush);
        if (FAILED(status) || brush == nullptr) {
            return nullptr;
        }

        brushes_.push_back(BrushEntry{key, brush});
        return brush;
    }

  private:
    [[nodiscard]] static std::uint8_t color_channel(float value) noexcept {
        if (value <= 0.0F) {
            return 0U;
        }

        if (value >= 1.0F) {
            return 255U;
        }

        return static_cast<std::uint8_t>(value * 255.0F + 0.5F);
    }

    [[nodiscard]] static std::uint32_t color_key(Color color) noexcept {
        return (static_cast<std::uint32_t>(color_channel(color.r)) << 24U) |
               (static_cast<std::uint32_t>(color_channel(color.g)) << 16U) |
               (static_cast<std::uint32_t>(color_channel(color.b)) << 8U) |
               static_cast<std::uint32_t>(color_channel(color.a));
    }

    HRESULT d2d_status_{E_FAIL};
    HRESULT dwrite_status_{E_FAIL};
    // v0.4 (2026-05-15): 升级到 D2D 1.1，工厂换成 ID2D1Factory1；DCRenderTarget
    // 通过 QueryInterface 拿出 ID2D1DeviceContext 接口供 Effects 调用（v1.0 的
    // ID2D1RenderTarget 没 CreateEffect/DrawImage，做不出真高斯模糊阴影）。
    // 现有 FillRectangle/CreateSolidColorBrush 等仍走 RenderTarget 接口不变。
    ID2D1Factory1* d2d_factory_{nullptr};
    IDWriteFactory* dwrite_factory_{nullptr};
    ID2D1DCRenderTarget* dc_render_target_{nullptr};
    ID2D1DeviceContext* device_context_{nullptr};
    // v1.23.1 (2026-05-15): Effect 对象缓存。CLSID_D2D1Shadow / GaussianBlur
    // 每帧 CreateEffect + Release 在滚动场景下是明显的性能热点（invest 毛玻璃
    // navbar 滚动卡顿）。Effect 对象本身可复用，每帧只 SetInput / SetValue。
    ID2D1Effect* shadow_effect_{nullptr};
    ID2D1Effect* blur_effect_{nullptr};
    std::vector<TextFormatEntry> text_formats_{};
    std::vector<BrushEntry> brushes_{};
};

DirectTextSession& direct_text_session() {
    static DirectTextSession session;
    return session;
}

int round_to_int(float value) {
    return static_cast<int>(value >= 0.0F ? value + 0.5F : value - 0.5F);
}

float dpi_to_scale(UINT dpi) {
    return static_cast<float>(dpi == 0U ? 96U : dpi) / 96.0F;
}

float normalize_scale(float scale) {
    return scale <= 0.0F ? 1.0F : scale;
}

LONG scale_to_long(float value, float scale) {
    return static_cast<LONG>(round_to_int(value * normalize_scale(scale)));
}

bool enable_process_dpi_awareness() {
    static const bool enabled = [] {
        if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE) {
            return true;
        }

        return GetLastError() == ERROR_ACCESS_DENIED;
    }();
    return enabled;
}

float system_dpi_scale() {
    return dpi_to_scale(GetDpiForSystem());
}

float window_dpi_scale(HWND hwnd) {
    return dpi_to_scale(GetDpiForWindow(hwnd));
}

PointF device_to_logical(PointF point, float scale) {
    const float normalized = normalize_scale(scale);
    return PointF{point.x / normalized, point.y / normalized};
}

RectF scaled_rect(RectF rect, float scale) {
    const float normalized = normalize_scale(scale);
    return RectF{
        rect.x * normalized,
        rect.y * normalized,
        rect.width * normalized,
        rect.height * normalized,
    };
}

PointF scaled_point(PointF point, float scale) {
    const float normalized = normalize_scale(scale);
    return PointF{point.x * normalized, point.y * normalized};
}

DrawCommand scaled_command(const DrawCommand& command, float scale) {
    const float normalized = normalize_scale(scale);
    DrawCommand scaled = command;
    scaled.bounds = scaled_rect(command.bounds, normalized);
    scaled.stroke_thickness = command.stroke_thickness * normalized;
    scaled.corner_radius = command.corner_radius * normalized;
    scaled.font_size = command.font_size * normalized;
    scaled.end_point = scaled_point(command.end_point, normalized);
    // v1.22 (2026-05-15): 阴影几何参数也走 DPI 缩放，否则 HiDPI 屏阴影会变细。
    scaled.shadow_offset = scaled_point(command.shadow_offset, normalized);
    scaled.shadow_blur = command.shadow_blur * normalized;
    scaled.shadow_spread = command.shadow_spread * normalized;
    // v1.22.1 (2026-05-15): Bezier 控制点 + Polygon 顶点也走 DPI 缩放，否则
    // HiDPI 屏下 bezier 曲线 / fill_polygon 面积图会跟 line / fill_rect 等
    // 命令错位（实战 bug：atlas Sparkline 面积图位置偏离数据线，原因正是
    // 这里漏处理 polygon.points）。
    scaled.bezier_c1 = scaled_point(command.bezier_c1, normalized);
    scaled.bezier_c2 = scaled_point(command.bezier_c2, normalized);
    for (auto& p : scaled.polygon.points) {
        p.x *= normalized;
        p.y *= normalized;
    }
    return scaled;
}

RectF offset_rect(RectF rect, PointF offset) {
    return RectF{rect.x - offset.x, rect.y - offset.y, rect.width, rect.height};
}

PointF offset_point(PointF point, PointF offset) {
    return PointF{point.x - offset.x, point.y - offset.y};
}

RectF inflated_rect(RectF rect, float amount) {
    return RectF{
        rect.x - amount,
        rect.y - amount,
        rect.width + amount * 2.0F,
        rect.height + amount * 2.0F,
    };
}

bool rects_intersect(RectF lhs, RectF rhs) {
    return !lhs.empty() && !rhs.empty() && lhs.x < rhs.x + rhs.width && lhs.x + lhs.width > rhs.x &&
           lhs.y < rhs.y + rhs.height && lhs.y + lhs.height > rhs.y;
}

// 判断 clip 是否会压到圆角矩形的 4 个圆角方块。
// 如果不压，`fill_rounded_rect(r, radius, color)` 对 clip 的视觉输出等价于
// 一个普通 `fill_rect(r, color)`，GDI 快路径就够，不用让 GDI+ 去 tessellate
// 一条 616x648 的完整 path（那是 3-8ms/次的 CPU 大坑）。
bool rounded_corners_overlap_clip(const RectF& rect, float radius, const RectF& clip) {
    if (radius <= 0.0F) {
        return false;
    }
    const float r = std::min(radius, std::min(rect.width, rect.height) * 0.5F);
    const RectF corners[4] = {
        RectF{rect.x, rect.y, r, r},
        RectF{rect.x + rect.width - r, rect.y, r, r},
        RectF{rect.x, rect.y + rect.height - r, r, r},
        RectF{rect.x + rect.width - r, rect.y + rect.height - r, r, r},
    };
    for (const RectF& corner : corners) {
        if (rects_intersect(corner, clip)) {
            return true;
        }
    }
    return false;
}

// 判断描边（无论矩形还是圆角矩形）对 clip 是否有像素贡献。
// 描边活跃带 = outer 膨胀 (thickness/2 + 1) - inner 收缩 (thickness/2 + 1)。
// clip 完全落在 inner 内部 → 描边跟 clip 不相关，整条命令可以 skip。
bool stroke_reaches_clip(const RectF& rect, float thickness, const RectF& clip) {
    const float thick = thickness <= 0.0F ? 1.0F : thickness;
    const float t = thick * 0.5F + 1.0F;
    const RectF outer{rect.x - t, rect.y - t, rect.width + 2.0F * t, rect.height + 2.0F * t};
    if (!rects_intersect(outer, clip)) {
        return false;
    }
    const float iw = std::max(0.0F, rect.width - 2.0F * t);
    const float ih = std::max(0.0F, rect.height - 2.0F * t);
    if (iw <= 0.0F || ih <= 0.0F) {
        return true;
    }
    const RectF inner{rect.x + t, rect.y + t, iw, ih};
    const bool clip_inside_inner = clip.x >= inner.x && clip.y >= inner.y &&
                                    clip.x + clip.width <= inner.x + inner.width &&
                                    clip.y + clip.height <= inner.y + inner.height;
    return !clip_inside_inner;
}

RectF intersect_rect(RectF lhs, RectF rhs) {
    const float left = std::max(lhs.x, rhs.x);
    const float top = std::max(lhs.y, rhs.y);
    const float right = std::min(lhs.x + lhs.width, rhs.x + rhs.width);
    const float bottom = std::min(lhs.y + lhs.height, rhs.y + rhs.height);
    if (right <= left || bottom <= top) {
        return RectF{};
    }
    return RectF{left, top, right - left, bottom - top};
}

RectF union_rect(RectF lhs, RectF rhs) {
    if (lhs.empty()) {
        return rhs;
    }

    if (rhs.empty()) {
        return lhs;
    }

    const float left = std::min(lhs.x, rhs.x);
    const float top = std::min(lhs.y, rhs.y);
    const float right = std::max(lhs.x + lhs.width, rhs.x + rhs.width);
    const float bottom = std::max(lhs.y + lhs.height, rhs.y + rhs.height);
    return RectF{left, top, right - left, bottom - top};
}

RectF command_bounds(const DrawCommand& command) {
    if (command.kind != DrawCommandKind::Line) {
        return inflated_rect(command.bounds, std::max(2.0F, command.stroke_thickness + 2.0F));
    }

    const float left = std::min(command.bounds.x, command.end_point.x);
    const float top = std::min(command.bounds.y, command.end_point.y);
    const float right = std::max(command.bounds.x, command.end_point.x);
    const float bottom = std::max(command.bounds.y, command.end_point.y);
    return inflated_rect(RectF{left, top, right - left, bottom - top},
                         command.stroke_thickness + 2.0F);
}

std::uint8_t color_channel_to_byte(float channel) {
    const float scaled = std::clamp(channel, 0.0F, 1.0F) * 255.0F;
    return static_cast<std::uint8_t>(scaled + 0.5F);
}

COLORREF to_colorref(const Color& color) {
    return RGB(color_channel_to_byte(color.r), color_channel_to_byte(color.g),
               color_channel_to_byte(color.b));
}

float stroke_thickness(float thickness) {
    return thickness <= 0.0F ? 1.0F : thickness;
}

RectF inset_for_stroke(const RectF& rect, float thickness) {
    const float inset = stroke_thickness(thickness) * 0.5F;
    return RectF{
        rect.x + inset,
        rect.y + inset,
        std::max(0.0F, rect.width - inset * 2.0F),
        std::max(0.0F, rect.height - inset * 2.0F),
    };
}

// 清 clip 区到指定颜色，用原生 GDI FillRect，适合 D2D 不可用时的兜底。
void fill_rect_gdi(HDC hdc, const RECT& rect, const Color& color) {
    HBRUSH brush = CreateSolidBrush(to_colorref(color));
    if (brush == nullptr) {
        return;
    }
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

ID2D1DCRenderTarget* create_text_render_target(HDC hdc, const RECT& target_rect) {
    DirectTextSession& session = direct_text_session();
    if (!session.ok()) {
        return nullptr;
    }

    return session.dc_render_target(hdc, target_rect);
}

void replay_paint_commands(HDC hdc, const PaintContext& context, float scale, RectF clip_rect,
                           PointF offset, const RECT& render_target_rect) {
    // 整帧统一走 Direct2D：一次 BeginDraw / 一次 EndDraw，所有 fill / stroke / rect
    // / rounded / ellipse / line / text / bitmap 全部用同一个 render target。
    //
    // 之前是 GDI+ + D2D 交错：text 用 D2D（硬件加速），其它用 GDI+（软件光栅化），
    // 每次切换都要 Flush GDI+ + BeginDraw/EndDraw D2D，一帧里这种切换十几次，光
    // state-sync 成本就吃掉几 ms。改成统一 D2D 之后：
    //   1. 零切换开销
    //   2. 所有图元硬件加速（圆角矩形 / 椭圆在 GPU 上画，比 GDI+ 的 CPU
    //      tessellation 快 5–10×）
    //   3. text 和 shape 共用同一个 brush cache，进一步省构造
    //
    // 按 AA 模式分两档：FillRect / StrokeRect 用 ALIASED（小色块像素精确锐利，
    // 这正是 palette swatch 发虚的根治方向）；rounded / ellipse / line 用
    // PER_PRIMITIVE（AA）。切换只在两档间跳，跳动次数少。
    ID2D1DCRenderTarget* target = create_text_render_target(hdc, render_target_rect);
    DirectTextSession& session = direct_text_session();
    if (target == nullptr) {
        // 没有 D2D 就什么都不画（Windows 10+ 都应该有）。
        // 继续用 GDI 把 clip 区域清成 bg，至少不会留上一帧残影。
        const RECT cr = {
            static_cast<int>(clip_rect.x),
            static_cast<int>(clip_rect.y),
            static_cast<int>(clip_rect.x + clip_rect.width),
            static_cast<int>(clip_rect.y + clip_rect.height),
        };
        fill_rect_gdi(hdc, cr, Color::from_rgba8(20U, 24U, 30U));
        return;
    }

    target->BeginDraw();

    const D2D1_RECT_F d2d_clip = D2D1::RectF(
        clip_rect.x, clip_rect.y,
        clip_rect.x + clip_rect.width, clip_rect.y + clip_rect.height);
    target->PushAxisAlignedClip(d2d_clip, D2D1_ANTIALIAS_MODE_ALIASED);
    // 默认底色兜底（root Panel 通常会覆盖掉，这里只是保险）
    target->Clear(D2D1::ColorF(0.04F, 0.04F, 0.04F, 1.0F));

    target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    D2D1_ANTIALIAS_MODE current_aa = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    target->SetAntialiasMode(current_aa);

    auto ensure_aa = [&](D2D1_ANTIALIAS_MODE m) {
        if (current_aa != m) {
            target->SetAntialiasMode(m);
            current_aa = m;
        }
    };

    for (const DrawCommand& source_command : context.commands()) {
        // v1.1 严格 clip: PushClip / PopClip 必须先于通用过滤路径, 不能被 brush
        // 检查 / clip 相交 early skip 跳过, 否则 push/pop 配对失败导致 D2D
        // PushAxisAlignedClip 栈错乱 (D2D 在 EndDraw 时会失败抛错)。
        if (source_command.kind == DrawCommandKind::PushClip) {
            const DrawCommand scaled = scaled_command(source_command, scale);
            const RectF clipped = offset_rect(scaled.bounds, offset);
            const D2D1_RECT_F d2d_clip = D2D1::RectF(
                clipped.x, clipped.y,
                clipped.x + clipped.width, clipped.y + clipped.height);
            target->PushAxisAlignedClip(d2d_clip, D2D1_ANTIALIAS_MODE_ALIASED);
            continue;
        }
        if (source_command.kind == DrawCommandKind::PopClip) {
            target->PopAxisAlignedClip();
            continue;
        }

        DrawCommand command = scaled_command(source_command, scale);
        if (!rects_intersect(command_bounds(command), clip_rect)) {
            continue;
        }

        command.bounds = offset_rect(command.bounds, offset);
        command.end_point = offset_point(command.end_point, offset);
        // v1.22.1 (2026-05-15): Bezier 控制点 + Polygon 顶点同样要做屏幕原点
        // offset 平移，否则 bounds 跟实际顶点错位（同 scaled_command 注释里的
        // atlas Sparkline 面积图位置 bug）。
        command.bezier_c1 = offset_point(command.bezier_c1, offset);
        command.bezier_c2 = offset_point(command.bezier_c2, offset);
        for (auto& p : command.polygon.points) {
            p = offset_point(p, offset);
        }

        const D2D1_RECT_F d2d_rect = D2D1::RectF(
            command.bounds.x, command.bounds.y,
            command.bounds.x + command.bounds.width,
            command.bounds.y + command.bounds.height);

        ID2D1SolidColorBrush* brush = session.solid_brush(target, command.color);
        if (brush == nullptr && command.kind != DrawCommandKind::DrawTexture) {
            continue;
        }

        switch (command.kind) {
        case DrawCommandKind::FillRect:
            ensure_aa(D2D1_ANTIALIAS_MODE_ALIASED);
            target->FillRectangle(d2d_rect, brush);
            break;
        case DrawCommandKind::StrokeRect: {
            if (!stroke_reaches_clip(command.bounds, command.stroke_thickness, clip_rect)) {
                break;
            }
            ensure_aa(D2D1_ANTIALIAS_MODE_ALIASED);
            const float t = stroke_thickness(command.stroke_thickness);
            const RectF r = inset_for_stroke(command.bounds, t);
            const D2D1_RECT_F sr = D2D1::RectF(r.x, r.y, r.x + r.width, r.y + r.height);
            target->DrawRectangle(sr, brush, t);
            break;
        }
        case DrawCommandKind::FillRoundedRect:
            // Clip 压不到任何圆角 → 退化为 FillRectangle + ALIASED，锐利又便宜。
            // 其它情况走 FillRoundedRectangle + PER_PRIMITIVE。
            if (command.color.a >= 0.999F &&
                !rounded_corners_overlap_clip(command.bounds, command.corner_radius, clip_rect)) {
                ensure_aa(D2D1_ANTIALIAS_MODE_ALIASED);
                target->FillRectangle(d2d_rect, brush);
            } else {
                ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                const D2D1_ROUNDED_RECT rr = {d2d_rect, command.corner_radius,
                                              command.corner_radius};
                target->FillRoundedRectangle(rr, brush);
            }
            break;
        case DrawCommandKind::StrokeRoundedRect: {
            if (!stroke_reaches_clip(command.bounds, command.stroke_thickness, clip_rect)) {
                break;
            }
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            const float t = stroke_thickness(command.stroke_thickness);
            const RectF r = inset_for_stroke(command.bounds, t);
            const D2D1_RECT_F sr = D2D1::RectF(r.x, r.y, r.x + r.width, r.y + r.height);
            const float radius_adj = std::max(0.0F, command.corner_radius - t * 0.5F);
            const D2D1_ROUNDED_RECT rr = {sr, radius_adj, radius_adj};
            target->DrawRoundedRectangle(rr, brush, t);
            break;
        }
        case DrawCommandKind::FillEllipse: {
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            const float cx = command.bounds.x + command.bounds.width * 0.5F;
            const float cy = command.bounds.y + command.bounds.height * 0.5F;
            const D2D1_ELLIPSE e = {D2D1::Point2F(cx, cy),
                                     command.bounds.width * 0.5F,
                                     command.bounds.height * 0.5F};
            target->FillEllipse(e, brush);
            break;
        }
        case DrawCommandKind::StrokeEllipse: {
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            const float cx = command.bounds.x + command.bounds.width * 0.5F;
            const float cy = command.bounds.y + command.bounds.height * 0.5F;
            const D2D1_ELLIPSE e = {D2D1::Point2F(cx, cy),
                                     command.bounds.width * 0.5F,
                                     command.bounds.height * 0.5F};
            target->DrawEllipse(e, brush, stroke_thickness(command.stroke_thickness));
            break;
        }
        case DrawCommandKind::Line:
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            target->DrawLine(
                D2D1::Point2F(command.bounds.x, command.bounds.y),
                D2D1::Point2F(command.end_point.x, command.end_point.y),
                brush, stroke_thickness(command.stroke_thickness));
            break;
        case DrawCommandKind::Text: {
            IDWriteTextFormat* fmt = session.text_format(
                command.font_size, command.bold, command.text_alignment, command.font_family);
            if (fmt == nullptr) {
                break;
            }
            const std::wstring wide_text = utf8_to_wide(command.text);
            // v1.2 (2026-04-28): ENABLE_COLOR_FONT 让 DirectWrite 走彩色字形 (COLR/
            // CPAL/SVG/CBDT) 渲染。Segoe UI Emoji 是 COLR 格式, 不开这个 flag 就
            // 退化到 outline 单色画 (用户截图里那种黑白线条 emoji 就是这个 fallback)。
            // Win 8.1 / D2D 1.2+ 引入, Win 10 系统默认可用。
            //
            // 字体回退: 基础 format 用 "Segoe UI" (无 emoji glyphs), DirectWrite
            // 内部对 BMP 外的 emoji codepoint 自动 fallback 到 Segoe UI Emoji,
            // 不需要手动设 fallback 链。后期媒体宿主 / 富文本场景跨脚本混排同样
            // 自动走系统 fallback。
            target->DrawText(
                wide_text.c_str(), static_cast<UINT32>(wide_text.size()), fmt, d2d_rect,
                brush,
                static_cast<D2D1_DRAW_TEXT_OPTIONS>(
                    D2D1_DRAW_TEXT_OPTIONS_CLIP | D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT));
            break;
        }
        case DrawCommandKind::DrawTexture: {
            // D2D Bitmap 从 CPU 内存创建。alpha mode 看 PixelBuffer 自带的 hint:
            //   - 视频帧 (MF_RGB32):alpha 字节常为 0,要 IGNORE 否则全透明;
            //   - PNG / 装饰资源:RGB 已与 alpha 预乘,要 PREMULTIPLIED 否则
            //     透明像素的 RGB=0 直接显示成黑色(典型表现:icon 周围一圈黑)。
            const auto& buf = command.pixel_buffer;
            if (buf.empty()) {
                ensure_aa(D2D1_ANTIALIAS_MODE_ALIASED);
                ID2D1SolidColorBrush* placeholder =
                    session.solid_brush(target, Color::from_rgba8(32U, 38U, 48U));
                if (placeholder != nullptr) {
                    target->FillRectangle(d2d_rect, placeholder);
                }
                break;
            }
            const D2D1_ALPHA_MODE alpha_mode =
                buf.alpha_mode == AlphaMode::Premultiplied
                    ? D2D1_ALPHA_MODE_PREMULTIPLIED
                    : D2D1_ALPHA_MODE_IGNORE;
            D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, alpha_mode));
            ID2D1Bitmap* bmp = nullptr;
            const HRESULT hr = target->CreateBitmap(
                D2D1::SizeU(static_cast<UINT32>(buf.width),
                            static_cast<UINT32>(buf.height)),
                buf.data->data(),
                static_cast<UINT32>(buf.stride),
                &props,
                &bmp);
            if (SUCCEEDED(hr) && bmp != nullptr) {
                target->DrawBitmap(bmp, d2d_rect, 1.0F,
                                   D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                bmp->Release();
            }
            break;
        }
        case DrawCommandKind::Bezier: {
            // 三次贝塞尔曲线 stroke。D2D PathGeometry — 起点 BeginFigure,
            // AddBezier(p1,p2,p3),EndFigure(open),DrawGeometry。
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            ID2D1PathGeometry* geom = nullptr;
            if (FAILED(session.d2d_factory()->CreatePathGeometry(&geom)) || geom == nullptr) break;
            ID2D1GeometrySink* sink = nullptr;
            if (FAILED(geom->Open(&sink)) || sink == nullptr) {
                geom->Release();
                break;
            }
            sink->BeginFigure(D2D1::Point2F(command.bounds.x, command.bounds.y),
                              D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(command.bezier_c1.x, command.bezier_c1.y),
                D2D1::Point2F(command.bezier_c2.x, command.bezier_c2.y),
                D2D1::Point2F(command.end_point.x, command.end_point.y)));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            target->DrawGeometry(geom, brush, stroke_thickness(command.stroke_thickness));
            sink->Release();
            geom->Release();
            break;
        }
        case DrawCommandKind::FillPolygon: {
            // 任意多边形 fill。PathGeometry + AddLines + EndFigure(closed) + FillGeometry。
            const auto& pts = command.polygon.points;
            if (pts.size() < 3) break;
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            ID2D1PathGeometry* geom = nullptr;
            if (FAILED(session.d2d_factory()->CreatePathGeometry(&geom)) || geom == nullptr) break;
            ID2D1GeometrySink* sink = nullptr;
            if (FAILED(geom->Open(&sink)) || sink == nullptr) {
                geom->Release();
                break;
            }
            sink->BeginFigure(D2D1::Point2F(pts[0].x, pts[0].y), D2D1_FIGURE_BEGIN_FILLED);
            std::vector<D2D1_POINT_2F> rest;
            rest.reserve(pts.size() - 1);
            for (std::size_t i = 1; i < pts.size(); ++i) {
                rest.push_back(D2D1::Point2F(pts[i].x, pts[i].y));
            }
            sink->AddLines(rest.data(), static_cast<UINT32>(rest.size()));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            target->FillGeometry(geom, brush);
            sink->Release();
            geom->Release();
            break;
        }
        case DrawCommandKind::FillGradientRect: {
            // 圆角矩形 + 纵向线性渐变(top → bottom)。LinearGradientBrush 两段:
            // 0 = color_top,1 = color_bottom。每帧 create + release 简化生命周期 —
            // session 缓存的话 key 是 (top,bottom) 数对,实际 unique 组合不多但
            // 复杂度上升,V1 暂不缓存。
            ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            const D2D1_GRADIENT_STOP stops[2] = {
                {0.0F, D2D1::ColorF(command.color.r, command.color.g, command.color.b, command.color.a)},
                {1.0F, D2D1::ColorF(command.gradient_end_color.r, command.gradient_end_color.g,
                                     command.gradient_end_color.b, command.gradient_end_color.a)},
            };
            ID2D1GradientStopCollection* stop_collection = nullptr;
            if (FAILED(target->CreateGradientStopCollection(stops, 2, &stop_collection)) ||
                stop_collection == nullptr) {
                break;
            }
            ID2D1LinearGradientBrush* gbrush = nullptr;
            const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES grad_props =
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(command.bounds.x, command.bounds.y),
                    D2D1::Point2F(command.bounds.x,
                                   command.bounds.y + command.bounds.height));
            if (FAILED(target->CreateLinearGradientBrush(grad_props, stop_collection, &gbrush)) ||
                gbrush == nullptr) {
                stop_collection->Release();
                break;
            }
            if (command.corner_radius > 0.0F) {
                const D2D1_ROUNDED_RECT rr = {d2d_rect, command.corner_radius,
                                               command.corner_radius};
                target->FillRoundedRectangle(rr, gbrush);
            } else {
                target->FillRectangle(d2d_rect, gbrush);
            }
            gbrush->Release();
            stop_collection->Release();
            break;
        }
        case DrawCommandKind::FillShadow: {
            // v1.22 (2026-05-15): 真高斯模糊阴影 = D2D 1.1 CLSID_D2D1Shadow Effect。
            // 流程：① CommandList 录一个"被投影的圆角矩形"（白色 alpha=1 作为
            // 形状蒙版，Shadow effect 会按 D2D1_SHADOW_PROP_COLOR 重新染色）；
            // ② SetTarget(shape_list) → 不嵌套 BeginDraw（当前帧已 BeginDraw,
            // 同一 BeginDraw 内可以多次 SetTarget）→ FillRoundedRectangle；
            // ③ SetTarget 回原 target，CreateEffect(CLSID_D2D1Shadow) 接 shape_list
            // 作输入，设 standard_deviation = blur/3 让 blur 语义跟 CSS 一致；
            // ④ DrawImage(shadow_fx, &offset) 把模糊后的阴影渲染到 main target。
            //
            // D2D 1.1 不可用（dc==nullptr）则降级跳过不画，业务侧仍能跑只是没阴影。
            auto* dc = session.device_context();
            if (dc == nullptr) break;
            if (command.shadow_blur <= 0.0F && command.shadow_spread <= 0.0F &&
                command.shadow_offset.x == 0.0F && command.shadow_offset.y == 0.0F) {
                break;  // 全零参数视为 noop（避免无意义的 Effect 开销）
            }

            // ① 算 spread 后的形状矩形（CSS box-shadow：正向外扩 / 负向内缩）
            RectF shape_bounds = command.bounds;
            shape_bounds.x      -= command.shadow_spread;
            shape_bounds.y      -= command.shadow_spread;
            shape_bounds.width  += command.shadow_spread * 2.0F;
            shape_bounds.height += command.shadow_spread * 2.0F;
            if (shape_bounds.width <= 0.0F || shape_bounds.height <= 0.0F) break;
            const float shape_radius = std::max(0.0F,
                command.corner_radius + command.shadow_spread);

            // ② 创建 CommandList 录形状
            ID2D1CommandList* shape_list = nullptr;
            if (FAILED(dc->CreateCommandList(&shape_list)) || shape_list == nullptr) break;

            ID2D1Image* prev_target = nullptr;
            dc->GetTarget(&prev_target);
            dc->SetTarget(shape_list);

            ID2D1SolidColorBrush* white = nullptr;
            if (SUCCEEDED(dc->CreateSolidColorBrush(
                    D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &white)) && white != nullptr) {
                const D2D1_RECT_F sr = D2D1::RectF(
                    shape_bounds.x, shape_bounds.y,
                    shape_bounds.x + shape_bounds.width,
                    shape_bounds.y + shape_bounds.height);
                if (shape_radius > 0.0F) {
                    const D2D1_ROUNDED_RECT rr = {sr, shape_radius, shape_radius};
                    dc->FillRoundedRectangle(rr, white);
                } else {
                    dc->FillRectangle(sr, white);
                }
                white->Release();
            }
            shape_list->Close();

            // ③ 恢复 target 到原 DCRenderTarget
            dc->SetTarget(prev_target);
            if (prev_target != nullptr) prev_target->Release();

            // ④ Shadow effect + DrawImage
            // v1.23.1: 用 session 缓存的 Shadow effect，不每帧 CreateEffect。
            ID2D1Effect* shadow_fx = session.shadow_effect();
            if (shadow_fx != nullptr) {
                shadow_fx->SetInput(0, shape_list);
                const float sigma = command.shadow_blur / 3.0F;  // CSS blur ≈ 3·sigma
                shadow_fx->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, sigma);
                const D2D1_VECTOR_4F shadow_color = {
                    command.color.r, command.color.g,
                    command.color.b, command.color.a};
                shadow_fx->SetValue(D2D1_SHADOW_PROP_COLOR, shadow_color);

                // shape_list 内坐标本身已是绝对坐标；DrawImage 的 target_offset
                // 是把 effect 输出整体再平移多少。CSS box-shadow 的 offset 就是
                // 阴影相对原始形状的平移，所以这里直接用 shadow_offset。
                const D2D1_POINT_2F target_pt = D2D1::Point2F(
                    command.shadow_offset.x, command.shadow_offset.y);
                dc->DrawImage(shadow_fx, &target_pt);
                // 不 Release：session 缓存复用。
            }

            shape_list->Release();
            break;
        }
        case DrawCommandKind::FillBackdropBlur: {
            // v1.23 (2026-05-15): 毛玻璃 / backdrop blur。
            // ① CopyFromRenderTarget 把 bounds 区域"截止本命令之前画的所有
            // 内容"抓成 ID2D1Bitmap 快照；② CLSID_D2D1GaussianBlur 模糊；
            // ③ DrawImage 回原位；④ 叠半透明 tint（圆角矩形）。
            //
            // 命令顺序保证：backdrop widget 在 z-order 上排在被模糊内容之后，
            // 所以 replay 到本命令时 render target 上已有完整背景。
            //
            // D2D 1.1 不可用 / CopyFromRenderTarget 失败 → 降级：只叠 tint，
            // 不崩。埋点 log 标 "backdrop" tag 便于定位。
            auto* dc = session.device_context();
            const RectF& bb = command.bounds;
            const int bw = static_cast<int>(bb.width + 0.5F);
            const int bh = static_cast<int>(bb.height + 0.5F);
            bool blur_ok = false;

            if (dc != nullptr && bw > 0 && bh > 0) {
                ID2D1Bitmap* snap = nullptr;
                const D2D1_SIZE_U snap_size = {
                    static_cast<UINT32>(bw), static_cast<UINT32>(bh)};
                const D2D1_BITMAP_PROPERTIES snap_props = D2D1::BitmapProperties(
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                      D2D1_ALPHA_MODE_PREMULTIPLIED));
                if (SUCCEEDED(target->CreateBitmap(snap_size, snap_props, &snap)) &&
                    snap != nullptr) {
                    const int bx = static_cast<int>(bb.x + 0.5F);
                    const int by = static_cast<int>(bb.y + 0.5F);
                    const D2D1_POINT_2U dst_pt = {0U, 0U};
                    const D2D1_RECT_U src_rect = {
                        static_cast<UINT32>(bx < 0 ? 0 : bx),
                        static_cast<UINT32>(by < 0 ? 0 : by),
                        static_cast<UINT32>((bx < 0 ? 0 : bx) + bw),
                        static_cast<UINT32>((by < 0 ? 0 : by) + bh)};
                    const HRESULT copy_hr =
                        snap->CopyFromRenderTarget(&dst_pt, target, &src_rect);
                    if (SUCCEEDED(copy_hr)) {
                        // v1.23.1: 用 session 缓存的 GaussianBlur effect。
                        ID2D1Effect* blur_fx = session.blur_effect();
                        if (blur_fx != nullptr) {
                            blur_fx->SetInput(0, snap);
                            blur_fx->SetValue(
                                D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,
                                command.shadow_blur / 3.0F);
                            dc->DrawImage(blur_fx,
                                          D2D1::Point2F(bb.x, bb.y));
                            // 不 Release：session 缓存复用。
                            blur_ok = true;
                        }
                    } else {
                        HUI_LOG_W("backdrop",
                            "CopyFromRenderTarget failed, hr=" +
                            std::to_string(static_cast<long>(copy_hr)));
                    }
                    snap->Release();
                }
            }
            if (!blur_ok) {
                HUI_LOG_W("backdrop",
                    "blur skipped (dc/size/copy unavailable), tint-only fallback");
            }

            // ④ 叠半透明 tint —— 即使 blur 降级也画，保证 navbar 仍有底色。
            if (command.color.a > 0.001F) {
                ensure_aa(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                if (command.corner_radius > 0.0F) {
                    const D2D1_ROUNDED_RECT rr = {d2d_rect, command.corner_radius,
                                                   command.corner_radius};
                    target->FillRoundedRectangle(rr, brush);
                } else {
                    target->FillRectangle(d2d_rect, brush);
                }
            }
            break;
        }
        }
    }

    target->PopAxisAlignedClip();
    const HRESULT end_status = target->EndDraw();
    if (end_status == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
        // 设备丢失（GPU reset / driver reload 等）时 D2D 会要求重建 target。
        // 下次 BindDC 会失败，session 会回退。V1 不做主动重建，下一帧自然恢复。
    }
}

void paint_widget_tree(const Widget& widget, PaintContext& context, RectF clip_rect);
void clear_dirty_widget_tree(Widget& widget);
bool has_dirty(DirtyFlags flags, DirtyFlags target);
bool has_dirty_widget_tree(const Widget& widget, DirtyFlags target);
bool dirty_bounds_widget_tree(const Widget& widget, DirtyFlags target, RectF& bounds);

// v1.24 (2026-05-15): 算自上次 tick 以来的真实 wall-clock delta（秒），并更新
// last_tick_qpc。WM_TIMER 与 WM_MOUSEWHEEL 两条 tick 驱动路径共用，保证用同一
// 时间基准、delta 连续不跳变。
float compute_tick_delta(Window& window) {
    LARGE_INTEGER freq{};
    LARGE_INTEGER now{};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    std::int64_t& last = WindowRuntimeAccess::last_tick_qpc(window);
    float delta_seconds = 1.0F / 60.0F;
    if (last != 0 && freq.QuadPart > 0) {
        const std::int64_t diff = now.QuadPart - last;
        delta_seconds = static_cast<float>(
            static_cast<double>(diff) / static_cast<double>(freq.QuadPart));
        if (delta_seconds > 0.5F) delta_seconds = 0.5F;
        if (delta_seconds < 0.0F) delta_seconds = 1.0F / 60.0F;
    }
    last = now.QuadPart;
    return delta_seconds;
}

void ensure_animation_timer(Window& window) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr ||
        WindowRuntimeAccess::animation_timer_active(window)) {
        return;
    }

    SetTimer(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)), kAnimationTimerId,
             kAnimationTimerIntervalMs, nullptr);
    WindowRuntimeAccess::animation_timer_active(window) = true;
}

void stop_animation_timer(Window& window) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr ||
        !WindowRuntimeAccess::animation_timer_active(window)) {
        return;
    }

    KillTimer(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)),
              kAnimationTimerId);
    WindowRuntimeAccess::animation_timer_active(window) = false;
}

void invalidate_window(Window& window, BOOL erase = FALSE) {
    if (WindowRuntimeAccess::native_handle(window) != nullptr) {
        InvalidateRect(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)), nullptr,
                       erase);
    }
}

RECT logical_rect_to_device_rect(Window& window, RectF rect, float inflate = 4.0F) {
    const float scale = normalize_scale(WindowRuntimeAccess::dpi_scale(window));
    const RectF device = scaled_rect(inflated_rect(rect, inflate), scale);
    return RECT{
        scale_to_long(device.x, 1.0F),
        scale_to_long(device.y, 1.0F),
        scale_to_long(device.x + device.width, 1.0F),
        scale_to_long(device.y + device.height, 1.0F),
    };
}

void invalidate_window_rect(Window& window, RectF rect, BOOL erase = FALSE) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr || rect.empty()) {
        return;
    }

    RECT native_rect = logical_rect_to_device_rect(window, rect);
    InvalidateRect(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)), &native_rect,
                   erase);
}

bool is_large_dirty_area(Window& window, RectF dirty_bounds) {
    if (WindowRuntimeAccess::content(window) == nullptr) {
        return true;
    }

    const RectF root_frame = WindowRuntimeAccess::content(window)->frame();
    const float root_area = root_frame.width * root_frame.height;
    const float dirty_area = dirty_bounds.width * dirty_bounds.height;
    return root_area > 0.0F && dirty_area / root_area > 0.25F;
}

void schedule_deferred_repaint(Window& window) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr ||
        WindowRuntimeAccess::deferred_repaint_pending(window)) {
        return;
    }

    WindowRuntimeAccess::deferred_repaint_pending(window) = true;
    PostMessageW(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)),
                 kDeferredRepaintMessage, 0, 0);
}

void invalidate_dirty_window(Window& window, BOOL erase = FALSE) {
    if (WindowRuntimeAccess::content(window) == nullptr) {
        invalidate_window(window, erase);
        return;
    }

    // dirty_bounds_widget_tree 的返回值天然就是 has_dirty，直接把两步合成一步，
    // WM_MOUSEMOVE / hover 路径省掉一半 O(N) 遍历。
    RectF dirty_bounds{};
    if (dirty_bounds_widget_tree(*WindowRuntimeAccess::content(window), DirtyFlags::Paint,
                                 dirty_bounds)) {
        if (is_large_dirty_area(window, dirty_bounds)) {
            schedule_deferred_repaint(window);
            return;
        }

        invalidate_window_rect(window, dirty_bounds, erase);
    }
}

void repaint_window_now(Window& window, BOOL erase = FALSE) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr) {
        return;
    }

    RedrawWindow(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)), nullptr,
                 nullptr, RDW_INVALIDATE | RDW_UPDATENOW | (erase ? RDW_ERASE : RDW_NOERASE));
}

void repaint_window_rect_now(Window& window, RectF rect, BOOL erase = FALSE) {
    if (WindowRuntimeAccess::native_handle(window) == nullptr || rect.empty()) {
        return;
    }

    RECT native_rect = logical_rect_to_device_rect(window, rect);
    RedrawWindow(reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window)), &native_rect,
                 nullptr, RDW_INVALIDATE | RDW_UPDATENOW | (erase ? RDW_ERASE : RDW_NOERASE));
}

void repaint_dirty_window_now(Window& window, BOOL erase = FALSE, bool defer_large = true) {
    if (WindowRuntimeAccess::content(window) == nullptr) {
        repaint_window_now(window, erase);
        return;
    }

    RectF dirty_bounds{};
    if (dirty_bounds_widget_tree(*WindowRuntimeAccess::content(window), DirtyFlags::Paint,
                                 dirty_bounds)) {
        if (defer_large && is_large_dirty_area(window, dirty_bounds)) {
            schedule_deferred_repaint(window);
            return;
        }

        repaint_window_rect_now(window, dirty_bounds, erase);
    }
}

void flush_deferred_repaint(Window& window) {
    WindowRuntimeAccess::deferred_repaint_pending(window) = false;
    if (WindowRuntimeAccess::content(window) == nullptr ||
        !has_dirty_widget_tree(*WindowRuntimeAccess::content(window), DirtyFlags::Paint)) {
        return;
    }

    repaint_window_now(window, FALSE);
}

void release_back_buffer(Window& window) {
    HDC memory_dc = reinterpret_cast<HDC>(WindowRuntimeAccess::back_buffer_dc(window));
    HBITMAP bitmap = reinterpret_cast<HBITMAP>(WindowRuntimeAccess::back_buffer_bitmap(window));
    HGDIOBJ old_bitmap =
        reinterpret_cast<HGDIOBJ>(WindowRuntimeAccess::back_buffer_old_bitmap(window));

    if (memory_dc != nullptr) {
        if (old_bitmap != nullptr) {
            SelectObject(memory_dc, old_bitmap);
        }

        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }

        DeleteDC(memory_dc);
    }

    WindowRuntimeAccess::back_buffer_dc(window) = nullptr;
    WindowRuntimeAccess::back_buffer_bitmap(window) = nullptr;
    WindowRuntimeAccess::back_buffer_old_bitmap(window) = nullptr;
    WindowRuntimeAccess::back_buffer_width(window) = 0;
    WindowRuntimeAccess::back_buffer_height(window) = 0;
}

HDC ensure_back_buffer(Window& window, HDC target_dc, int width, int height) {
    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    HDC memory_dc = reinterpret_cast<HDC>(WindowRuntimeAccess::back_buffer_dc(window));
    if (memory_dc != nullptr && WindowRuntimeAccess::back_buffer_width(window) == width &&
        WindowRuntimeAccess::back_buffer_height(window) == height) {
        return memory_dc;
    }

    release_back_buffer(window);

    memory_dc = CreateCompatibleDC(target_dc);
    if (memory_dc == nullptr) {
        return nullptr;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(target_dc, width, height);
    if (bitmap == nullptr) {
        DeleteDC(memory_dc);
        return nullptr;
    }

    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    WindowRuntimeAccess::back_buffer_dc(window) = memory_dc;
    WindowRuntimeAccess::back_buffer_bitmap(window) = bitmap;
    WindowRuntimeAccess::back_buffer_old_bitmap(window) = old_bitmap;
    WindowRuntimeAccess::back_buffer_width(window) = width;
    WindowRuntimeAccess::back_buffer_height(window) = height;
    return memory_dc;
}

void render_window_content(Window& window, HDC hdc, const RECT& client_rect,
                           const RECT& paint_rect) {
    const int client_width = client_rect.right - client_rect.left;
    const int client_height = client_rect.bottom - client_rect.top;
    if (client_width <= 0 || client_height <= 0) {
        return;
    }

    RECT clipped_paint{
        std::max(client_rect.left, paint_rect.left),
        std::max(client_rect.top, paint_rect.top),
        std::min(client_rect.right, paint_rect.right),
        std::min(client_rect.bottom, paint_rect.bottom),
    };
    if (clipped_paint.right <= clipped_paint.left || clipped_paint.bottom <= clipped_paint.top) {
        clipped_paint = client_rect;
    }

    const int paint_width = clipped_paint.right - clipped_paint.left;
    const int paint_height = clipped_paint.bottom - clipped_paint.top;
    const float dpi_scale = normalize_scale(WindowRuntimeAccess::dpi_scale(window));
    const float logical_width = static_cast<float>(client_width) / dpi_scale;
    const float logical_height = static_cast<float>(client_height) / dpi_scale;
    const RectF clip_rect{
        static_cast<float>(clipped_paint.left),
        static_cast<float>(clipped_paint.top),
        static_cast<float>(paint_width),
        static_cast<float>(paint_height),
    };

    HDC memory_dc = ensure_back_buffer(window, hdc, client_width, client_height);
    if (memory_dc == nullptr) {
        return;
    }

    const int saved_dc = SaveDC(memory_dc);
    IntersectClipRect(memory_dc, clipped_paint.left, clipped_paint.top, clipped_paint.right,
                      clipped_paint.bottom);

    // D2D 的 Clear 会在 BeginDraw 之后清 clip 区；这里保留一次 GDI 清底作为
    // D2D 不可用时的兜底（replay_paint_commands 的 fallback 路径也会走到）。
    fill_rect_gdi(memory_dc, clipped_paint, Color::from_rgba8(24U, 28U, 34U));

    if (window.content() != nullptr) {
        // 只在尺寸变化或 Layout 脏时才 arrange。之前每帧都 arrange 一次，对
        // gallery 这种几百个 widget 的树，光 set_frame 链的等值比较都是一笔
        // 明显开销（尤其 WM_TIMER 16ms 触发的动画路径上）。
        float& aw = WindowRuntimeAccess::arranged_width(window);
        float& ah = WindowRuntimeAccess::arranged_height(window);
        const bool size_changed = aw != logical_width || ah != logical_height;
        const bool layout_dirty = has_dirty(window.content()->dirty_flags(), DirtyFlags::Layout) ||
                                  has_dirty_widget_tree(*window.content(), DirtyFlags::Layout);
        if (size_changed || layout_dirty) {
            window.content()->arrange(RectF{
                0.0F,
                0.0F,
                logical_width,
                logical_height,
            });
            aw = logical_width;
            ah = logical_height;
        }

        const RectF logical_clip{
            clip_rect.x / dpi_scale,
            clip_rect.y / dpi_scale,
            clip_rect.width / dpi_scale,
            clip_rect.height / dpi_scale,
        };

        PaintContext& context = WindowRuntimeAccess::paint_context(window);
        context.clear();
        paint_widget_tree(*window.content(), context, logical_clip);
        const RECT render_target_rect{0, 0, client_width, client_height};
        replay_paint_commands(memory_dc, context, dpi_scale, clip_rect, PointF{},
                              render_target_rect);
        clear_dirty_widget_tree(*window.content());
    }

    RestoreDC(memory_dc, saved_dc);
    BitBlt(hdc, clipped_paint.left, clipped_paint.top, paint_width, paint_height, memory_dc,
           clipped_paint.left, clipped_paint.top, SRCCOPY);
}

void paint_widget_tree(const Widget& widget, PaintContext& context, RectF clip_rect) {
    if (!widget.visible()) {
        return;
    }

    if (!rects_intersect(inflated_rect(widget.frame(), 8.0F), clip_rect)) {
        return;
    }

    // v1.20 (2026-05-14): widget.clips_self() == true 时，自动 push_clip 包住
    // widget.paint。让 image bg cover、装饰渲染等"画到溢出 frame"的 widget
    // 不再需要业务手 push/pop_clip。jtui_pro/main.cpp ImageBgWidget 是首个用例。
    const bool clip_self = widget.clips_self();
    if (clip_self) {
        const RectF self_clip = intersect_rect(clip_rect, widget.frame());
        context.push_clip(self_clip);
    }
    widget.paint(context);
    if (clip_self) {
        context.pop_clip();
    }

    // 容器声明 clips_children = true 时(典型:ScrollView)走严格 clip 路径:
    //   1. child_clip 收紧到 intersect(parent_clip, widget.frame), 让 child 在
    //      frame 外整体被 paint_widget_tree 早停 (child frame 完全外则跳过)。
    //   2. v1.1 (2026-04-27): 额外 push_clip / pop_clip 让"部分相交"的 child
    //      paint 命令在 D2D 端真被 PushAxisAlignedClip 切。v1 时 inflated_rect
    //      相交检测只能跳过完全在外的 child, 部分超出仍画整 child → 滚动列表
    //      最后一条 item / scroll 偏出 viewport 的 content 会溢出到容器之外,
    //      典型是 ProfilePage 滚动覆盖到顶部 caption 区。
    const bool clip_to_frame = widget.clips_children();
    const RectF child_clip = clip_to_frame
                                 ? intersect_rect(clip_rect, widget.frame())
                                 : clip_rect;
    if (clip_to_frame) {
        context.push_clip(child_clip);
    }

    for (const auto& child : widget.children()) {
        const auto* child_widget = dynamic_cast<const Widget*>(child.get());
        if (child_widget != nullptr) {
            paint_widget_tree(*child_widget, context, child_clip);
        }
    }

    if (clip_to_frame) {
        context.pop_clip();
    }

    // v1.18 (2026-05-03): 前景层 — children + clip 全部画完后调一次,让 widget
    // 把"必须在内容之上"的元素 (典型: ScrollView 滚动条) 画在最上层。
    // 顺序保证: 自身 paint → push_clip → children paint → pop_clip → paint_overlay。
    widget.paint_overlay(context);
}

bool has_dirty(DirtyFlags flags, DirtyFlags target) {
    return (flags & target) != DirtyFlags::None;
}

bool has_dirty_widget_tree(const Widget& widget, DirtyFlags target) {
    if (has_dirty(widget.dirty_flags(), target)) {
        return true;
    }
    // subtree_dirty 摘要：子树里没人标过 target 的话直接跳过递归。
    // gallery 这类几百个 widget、动画期间只有 1-2 个真脏的场景，能把
    // O(N) 全树扫描压到 O(dirty_path_length)。
    if (!has_dirty(widget.subtree_dirty_flags(), target)) {
        return false;
    }

    for (const auto& child : widget.children()) {
        const auto* child_widget = dynamic_cast<const Widget*>(child.get());
        if (child_widget != nullptr && has_dirty_widget_tree(*child_widget, target)) {
            return true;
        }
    }

    return false;
}

bool dirty_bounds_widget_tree(const Widget& widget, DirtyFlags target, RectF& bounds) {
    bool found = false;
    if (has_dirty(widget.dirty_flags(), target)) {
        bounds = widget.frame();
        found = true;
    }

    // subtree_dirty 没标过 target 时，子树里也不会有 dirty，跳过递归。
    if (has_dirty(widget.subtree_dirty_flags(), target)) {
        for (const auto& child : widget.children()) {
            const auto* child_widget = dynamic_cast<const Widget*>(child.get());
            if (child_widget == nullptr) {
                continue;
            }

            RectF child_bounds{};
            if (dirty_bounds_widget_tree(*child_widget, target, child_bounds)) {
                bounds = found ? union_rect(bounds, child_bounds) : child_bounds;
                found = true;
            }
        }
    }

    return found;
}

void clear_dirty_widget_tree(Widget& widget) {
    widget.clear_dirty();
    // 子树整树清完后，subtree_dirty 摘要也要清掉，否则下次 mark_dirty 的早停判定
    // 会因为祖先 subtree_dirty 仍然包含目标 flag 而提前返回，新的 dirty 反而冒不上来。
    widget.clear_subtree_dirty();

    for (const auto& child : widget.children()) {
        auto* child_widget = dynamic_cast<Widget*>(child.get());
        if (child_widget != nullptr) {
            clear_dirty_widget_tree(*child_widget);
        }
    }
}

bool tick_widget_tree(Widget& widget, float delta_seconds) {
    if (!widget.visible()) {
        return false;
    }

    bool active = widget.tick(delta_seconds);
    for (const auto& child : widget.children()) {
        auto* child_widget = dynamic_cast<Widget*>(child.get());
        if (child_widget != nullptr) {
            active = tick_widget_tree(*child_widget, delta_seconds) || active;
        }
    }

    return active;
}

EventState load_event_state(Window& window) {
    EventState state{
        WindowRuntimeAccess::hovered_widget(window),
        WindowRuntimeAccess::pressed_widget(window),
        WindowRuntimeAccess::focused_widget(window),
    };
    // v1.2 (2026-04-28): sanitize 防御 widget 子树替换的悬空指针。
    // ScrollView::set_content / Panel::clear_children / FlexBox 重建等容器内部
    // 替换子树时不会通知 Window EventState (只有 Window::set_content 整树替换会
    // 主动清), 残留指针导致下次事件 dispatch UAF — chat streaming / 媒体宿主
    // 时间线高频重建 (60-240fps) 必崩。这里在每次事件分发开头走一遍 widget_in_tree
    // 验证, 不在树里的 raw pointer 直接置 nullptr。
    Widget* root = WindowRuntimeAccess::content(window).get();
    EventDispatcher::sanitize(state, root);
    return state;
}

void save_event_state(Window& window, const EventState& state) {
    WindowRuntimeAccess::hovered_widget(window) = state.hovered;
    WindowRuntimeAccess::pressed_widget(window) = state.pressed;
    WindowRuntimeAccess::focused_widget(window) = state.focused;
}

PointF lparam_to_client_point(LPARAM lparam) {
    return PointF{
        static_cast<float>(GET_X_LPARAM(lparam)),
        static_cast<float>(GET_Y_LPARAM(lparam)),
    };
}

PointF lparam_to_logical_point(Window& window, LPARAM lparam) {
    return device_to_logical(lparam_to_client_point(lparam),
                             WindowRuntimeAccess::dpi_scale(window));
}


// v1.21 (2026-05-04): hover widget 的 cursor_kind 转 Win32 HCURSOR + SetCursor。
// 在 WM_SETCURSOR (Win32 推荐位) 和 WM_MOUSEMOVE 后 (覆盖首帧进入新 widget 的
// 边界情况) 都会调一次, IDC_* 是系统共享句柄不分配, 重复调 LoadCursorW 廉价。
//
// 返回 true 表示已 SetCursor (调用方可以在 WM_SETCURSOR 里返回 TRUE 抑制类默认),
// false 表示让默认箭头生效。
bool apply_widget_cursor(Widget* hovered) {
    if (hovered == nullptr) {
        return false;
    }
    HCURSOR hc = nullptr;
    switch (hovered->cursor_kind()) {
    case CursorKind::IBeam:
        hc = LoadCursor(nullptr, IDC_IBEAM);
        break;
    case CursorKind::Hand:
        hc = LoadCursor(nullptr, IDC_HAND);
        break;
    case CursorKind::Default:
        return false;
    }
    if (hc == nullptr) {
        return false;
    }
    SetCursor(hc);
    return true;
}

bool perform_window_action(Window& window, WindowAction action) {
    HWND hwnd = reinterpret_cast<HWND>(WindowRuntimeAccess::native_handle(window));
    if (hwnd == nullptr) {
        return false;
    }

    switch (action) {
    case WindowAction::Minimize:
        ShowWindow(hwnd, SW_MINIMIZE);
        return true;
    case WindowAction::ToggleMaximize: {
        const bool maximized = IsZoomed(hwnd) != FALSE;
        ShowWindow(hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
        return true;
    }
    case WindowAction::Close:
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        return true;
    case WindowAction::None:
    case WindowAction::DragMove:
        break;
    }

    return false;
}

void activate_widget(Window& window, Widget* widget, PointF point) {
    if (widget == nullptr || !widget->enabled()) {
        return;
    }

    // 系统级 window action（拖拽 / 最小化 / 最大化 / 关闭）优先于业务 click：
    // frameless 窗口的标题栏区点击不应该再触发 widget 业务回调。
    if (perform_window_action(window, widget->window_action_at(point))) {
        return;
    }

    // 业务 click 沿 root → target 的命中链走 capture/target/bubble 三阶段。
    // Target phase Widget::on_event 默认实现会调 on_click(point)。祖先 widget
    // 想拦截或观察可以 override on_event，按 phase 判定后调 ev.handled = true。
    Widget* root = WindowRuntimeAccess::content(window).get();
    if (root == nullptr) {
        return;
    }
    std::vector<Widget*> path;
    EventDispatcher::build_path(*root, point, path);
    if (path.empty()) {
        return;
    }
    MouseEvent mouse{};
    mouse.kind = EventKind::MouseClick;
    mouse.type = EventType::MouseUp;
    mouse.position = point;
    Event event = mouse;
    EventDispatcher::dispatch_event_along_path(path, event);
    // 不在这里 invalidate：紧接着 WM_LBUTTONUP 会走同步 repaint_dirty_window_now，
    // 再遍历一次脏树只会多花时间。
}

bool register_window_class() {
    static bool registered = false;
    if (registered) {
        return true;
    }

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = [](HWND hwnd, UINT message, WPARAM wparam,
                                  LPARAM lparam) -> LRESULT {
        auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (message) {
        case kDeferredRepaintMessage:
            if (window != nullptr) {
                flush_deferred_repaint(*window);
                return 0;
            }
            break;
        case kPendingCallbackMessage:
            // 安全地 drain 所有 post 进来的 callback。这条消息独立于 widget tick /
            // event dispatch,callback 内部 rebuild widget tree 不会 UAF。
            if (window != nullptr && window->application() != nullptr) {
                window->application()->drain_pending_callbacks();
                if (WindowRuntimeAccess::content(*window) != nullptr) {
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                }
            }
            return 0;
        case WM_NCCREATE: {
            const auto* create_struct = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            auto* created_window = reinterpret_cast<Window*>(create_struct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created_window));
            WindowRuntimeAccess::dpi_scale(*created_window) = window_dpi_scale(hwnd);
            return TRUE;
        }
        case WM_NCCALCSIZE:
            if (window != nullptr && window->options().frameless && wparam == TRUE) {
                return 0;
            }
            break;
        case WM_NCPAINT:
            if (window != nullptr && window->options().frameless) {
                return 0;
            }
            break;
        case WM_NCACTIVATE:
            if (window != nullptr && window->options().frameless) {
                return TRUE;
            }
            break;
        case WM_NCHITTEST: {
            if (window == nullptr || !window->options().frameless) {
                break;
            }

            const float dpi_scale = WindowRuntimeAccess::dpi_scale(*window);
            const LONG border = window->options().resizable ? scale_to_long(6.0F, dpi_scale) : 0L;
            RECT native_rect{};
            GetWindowRect(hwnd, &native_rect);
            const LONG x = GET_X_LPARAM(lparam);
            const LONG y = GET_Y_LPARAM(lparam);

            const bool left = border > 0 && x >= native_rect.left && x < native_rect.left + border;
            const bool right =
                border > 0 && x < native_rect.right && x >= native_rect.right - border;
            const bool top = border > 0 && y >= native_rect.top && y < native_rect.top + border;
            const bool bottom =
                border > 0 && y < native_rect.bottom && y >= native_rect.bottom - border;

            if (top && left) {
                return HTTOPLEFT;
            }
            if (top && right) {
                return HTTOPRIGHT;
            }
            if (bottom && left) {
                return HTBOTTOMLEFT;
            }
            if (bottom && right) {
                return HTBOTTOMRIGHT;
            }
            if (left) {
                return HTLEFT;
            }
            if (right) {
                return HTRIGHT;
            }
            if (top) {
                return HTTOP;
            }
            if (bottom) {
                return HTBOTTOM;
            }

            if (WindowRuntimeAccess::content(*window) != nullptr) {
                POINT client_point{x, y};
                ScreenToClient(hwnd, &client_point);
                const PointF logical_point = device_to_logical(
                    PointF{static_cast<float>(client_point.x), static_cast<float>(client_point.y)},
                    dpi_scale);
                Widget* hit = EventDispatcher::hit_test(
                    *WindowRuntimeAccess::content(*window), logical_point);
                if (hit != nullptr &&
                    hit->window_action_at(logical_point) == WindowAction::DragMove) {
                    return HTCAPTION;
                }
            }

            return HTCLIENT;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC hdc = BeginPaint(hwnd, &paint);

            RECT client_rect{};
            GetClientRect(hwnd, &client_rect);

            if (window != nullptr) {
                render_window_content(*window, hdc, client_rect, paint.rcPaint);
            } else {
                fill_rect_gdi(hdc, client_rect, Color::from_rgba8(24U, 28U, 34U));
            }

            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_DROPFILES: {
            // v1.22 (2026-05-04): Explorer 拖文件释放到客户区。
            // HDROP 持有文件路径列表 + drop 点 (client area, device px)。
            // 取所有 widechar 文件名转 utf8, 把 device px 转 logical px (与
            // widget RectF 同坐标), 调 application 注册的 callback。
            // DragFinish 必须配对释放 HDROP。
            if (window == nullptr) {
                return 0;
            }
            HDROP hdrop = reinterpret_cast<HDROP>(wparam);
            if (hdrop == nullptr) {
                return 0;
            }
            POINT drop_pt{};
            // BOOL = TRUE 表示落在客户区, FALSE 表示落在非客户区 (例如标题栏)。
            // 我们仍接受 — 业务自己用 widget.frame().contains 判定有效目标。
            DragQueryPoint(hdrop, &drop_pt);
            const UINT count = DragQueryFileW(hdrop, 0xFFFFFFFFU, nullptr, 0);
            std::vector<std::string> paths;
            paths.reserve(count);
            for (UINT i = 0; i < count; ++i) {
                const UINT len = DragQueryFileW(hdrop, i, nullptr, 0);
                if (len == 0) {
                    continue;
                }
                // DragQueryFileW 第 4 参 cch 包含 null 结尾, 实际写入 len 个 wchar
                // (不含 null), buffer 需要 len+1 cch。
                std::wstring wide(static_cast<std::size_t>(len), L'\0');
                DragQueryFileW(hdrop, i, wide.data(), len + 1U);
                std::string utf8 = wide_to_utf8(wide);
                if (!utf8.empty()) {
                    paths.push_back(std::move(utf8));
                }
            }
            DragFinish(hdrop);

            if (window->application() != nullptr && !paths.empty()) {
                const PointF logical = device_to_logical(
                    PointF{static_cast<float>(drop_pt.x),
                            static_cast<float>(drop_pt.y)},
                    WindowRuntimeAccess::dpi_scale(*window));
                window->application()->invoke_files_dropped(logical,
                                                              std::move(paths));
                // 业务可能在 callback 里改 widget tree (rebuild_input_card),
                // 触发一次 paint 让结果立刻可见。
                if (WindowRuntimeAccess::content(*window) != nullptr) {
                    invalidate_dirty_window(*window, FALSE);
                }
            }
            return 0;
        }
        case WM_SETCURSOR: {
            // v1.21 (2026-05-04): 客户区命中时按 hover widget.cursor_kind 设光标。
            // LOWORD(lparam) = HitTest 结果, HTCLIENT 才是 widget 区, 标题栏 /
            // 边框等非客户区让 DefWindowProc 处理 (resize 箭头 / 标题栏 sizing 等)。
            // 返回 TRUE 抑制 Windows 用类默认 IDC_ARROW 重置。
            if (window != nullptr && LOWORD(lparam) == HTCLIENT) {
                Widget* hovered = WindowRuntimeAccess::hovered_widget(*window);
                if (apply_widget_cursor(hovered)) {
                    return TRUE;
                }
            }
            break;
        }
        case WM_MOUSEMOVE: {
            if (window == nullptr) {
                return 0;
            }

            TRACKMOUSEEVENT event{};
            event.cbSize = sizeof(event);
            event.dwFlags = TME_LEAVE;
            event.hwndTrack = hwnd;
            TrackMouseEvent(&event);

            EventState state = load_event_state(*window);
            EventDispatcher::dispatch_pointer_move(WindowRuntimeAccess::content(*window).get(),
                                                   lparam_to_logical_point(*window, lparam), state);
            save_event_state(*window, state);
            // v1.21 (2026-05-04): hover 已更新, 立即应用 cursor_kind。WM_SETCURSOR
            // 用的是上一帧的 hovered_widget, 进入新 widget 的第一帧光标会落后,
            // 这里显式 SetCursor 弥补; 后续 WM_SETCURSOR 在更早时机处理稳态。
            apply_widget_cursor(state.hovered);
            // 直接调 invalidate_dirty_window —— 它内部会一次遍历同时判定 has_dirty
            // 和 bounds，不需要再走一遍 has_dirty_widget_tree 预检。
            if (WindowRuntimeAccess::content(*window) != nullptr) {
                invalidate_dirty_window(*window, FALSE);
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            if (window != nullptr) {
                EventState state = load_event_state(*window);
                EventDispatcher::dispatch_pointer_leave(state);
                save_event_state(*window, state);
                if (WindowRuntimeAccess::content(*window) != nullptr &&
                    has_dirty_widget_tree(*WindowRuntimeAccess::content(*window), DirtyFlags::Paint)) {
                    invalidate_dirty_window(*window, FALSE);
                }
            }
            return 0;
        case WM_LBUTTONDOWN: {
            if (window == nullptr || WindowRuntimeAccess::content(*window) == nullptr) {
                return 0;
            }

            SetFocus(hwnd);
            const PointF point = lparam_to_logical_point(*window, lparam);
            // v1.4 (2026-04-28): ModalLayer outside-press。在分发 LBUTTONDOWN
            // 给 widget 之前先派发给 application 的模态层栈, 任意 layer 的
            // contains_inside(point) 返回 false 就触发 on_outside_press 关闭弹层
            // (典型: AboutDialog 点遮罩外、SidebarFlyoutMenu 点菜单外)。命中后
            // **消费**该次点击避免下方 widget 同时收到 down → click, 与桌面端
            // "点外侧只关弹层不触发底层 widget" 的语义一致。
            if (window->application() != nullptr &&
                window->application()->try_dispatch_modal_outside_press(point)) {
                repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                return 0;
            }
            EventState state = load_event_state(*window);
            Widget* hit = EventDispatcher::dispatch_pointer_down(
                WindowRuntimeAccess::content(*window).get(), point, state);
            if (hit != nullptr) {
                SetCapture(hwnd);
            }
            const bool focus_changed = EventDispatcher::set_focus(
                state, hit != nullptr && hit->accepts_focus() ? hit : nullptr);
            save_event_state(*window, state);
            if (focus_changed) {
                invalidate_dirty_window(*window, FALSE);
            }
            // 按下也可能启动动画（Button 的 press 过渡、后续其他受 pressed() 影响的组件），
            // 不在这里 ensure 的话 tick 要等到 LBUTTONUP 才跑，按下期间根本不动。
            ensure_animation_timer(*window);
            // 关键：在同步 paint 之前先推进一帧动画。否则这一帧画的还是动画目标
            // 为 0 的初始值，真正的第一帧动画要等 WM_TIMER（~16-30ms 以后）
            // 才出现，用户感受就是"按下和反馈之间有一拍"。
            if (WindowRuntimeAccess::content(*window) != nullptr) {
                tick_widget_tree(*WindowRuntimeAccess::content(*window), 1.0F / 60.0F);
            }
            // 鼠标按下也算一次 tick，更新时间戳，下次 WM_TIMER delta 才不会从
            // 上次定时器时间起算（那时间戳通常远在过去）。
            {
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);
                WindowRuntimeAccess::last_tick_qpc(*window) = now.QuadPart;
            }
            repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (window == nullptr) {
                return 0;
            }

            const PointF point = lparam_to_logical_point(*window, lparam);
            EventState state = load_event_state(*window);
            Widget* activation = nullptr;
            EventDispatcher::dispatch_pointer_up(WindowRuntimeAccess::content(*window).get(), point,
                                                 state, activation);
            save_event_state(*window, state);

            if (activation != nullptr) {
                activate_widget(*window, activation, point);
            }

            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }

            EventState post_state = load_event_state(*window);
            EventDispatcher::dispatch_pointer_move(WindowRuntimeAccess::content(*window).get(),
                                                   point, post_state);
            save_event_state(*window, post_state);

            ensure_animation_timer(*window);
            // 同 LBUTTONDOWN：先推进一帧动画，再同步 paint，让释放后的过渡
            // (Switch 的 knob 往回滑 / Button 的亮度恢复) 从这一帧就开始。
            if (WindowRuntimeAccess::content(*window) != nullptr) {
                tick_widget_tree(*WindowRuntimeAccess::content(*window), 1.0F / 60.0F);
            }
            {
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);
                WindowRuntimeAccess::last_tick_qpc(*window) = now.QuadPart;
            }
            // 点击释放是用户输入路径，强制同步绘制，否则主题/语言这类改全树的操作会被
            // defer_large 延到下一条 PostMessage，用户能明显感知到一帧以上的延迟。
            repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
            return 0;
        }
        // v1.3 (2026-04-28): 右键支持。RBUTTONDOWN/UP 沿 path 把 MouseEvent.button =
        // Right 分发, widget on_event 内通过 .button 区分 (例如 MarkdownText 右键
        // 弹复制 / 业务层右键菜单)。focus / capture 行为复用 LBUTTONDOWN 路径,
        // 因为右键也算"激活" widget (右键拖选目前不做, RBUTTONUP 不调 dispatch_pointer_up
        // 避免清掉左键 pressed 状态; 直接同步 dispatch event 不维护 pressed)。
        case WM_RBUTTONDOWN: {
            if (window == nullptr || WindowRuntimeAccess::content(*window) == nullptr) {
                return 0;
            }
            const PointF point = lparam_to_logical_point(*window, lparam);
            // v1.4 (2026-04-28): 右键也走 modal outside-press, 与左键同语义 — 在
            // SidebarFlyoutMenu 浮起时右键卡片外应直接关菜单, 不让底层 widget
            // 收到 down 弹自己的右键菜单造成菜单叠加。
            if (window->application() != nullptr &&
                window->application()->try_dispatch_modal_outside_press(point)) {
                repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                return 0;
            }
            EventState state = load_event_state(*window);
            // dispatch right press: 不通过 dispatch_pointer_down (那会改 state.pressed
            // 串通 click 链, 影响 LBUTTON 拖动状态)。直接构建 path 走事件分发。
            std::vector<Widget*> path;
            EventDispatcher::build_path(*WindowRuntimeAccess::content(*window), point, path);
            if (!path.empty()) {
                MouseEvent ev{};
                ev.kind = EventKind::MouseDown;
                ev.type = EventType::MouseDown;
                ev.position = point;
                ev.pressed = true;
                ev.button = MouseButton::Right;
                Event event = ev;
                EventDispatcher::dispatch_event_along_path(path, event);
            }
            save_event_state(*window, state);
            repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
            return 0;
        }
        case WM_RBUTTONUP: {
            if (window == nullptr || WindowRuntimeAccess::content(*window) == nullptr) {
                return 0;
            }
            const PointF point = lparam_to_logical_point(*window, lparam);
            EventState state = load_event_state(*window);
            std::vector<Widget*> path;
            EventDispatcher::build_path(*WindowRuntimeAccess::content(*window), point, path);
            if (!path.empty()) {
                MouseEvent ev{};
                ev.kind = EventKind::MouseUp;
                ev.type = EventType::MouseUp;
                ev.position = point;
                ev.pressed = false;
                ev.button = MouseButton::Right;
                Event event = ev;
                EventDispatcher::dispatch_event_along_path(path, event);
            }
            save_event_state(*window, state);
            repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
            return 0;
        }
        case WM_CHAR:
            if (window != nullptr) {
                EventState state = load_event_state(*window);
                // v1.2 (2026-04-28): wparam 是 UTF-16 code unit. BMP 内中文 /
                // 日文 / 韩文 / 全角符号都是单 unit, 直接当 char32_t 派发. BMP
                // 外 (>= 0x10000) 走代理对 (surrogate pair) 需要状态机暂存上半,
                // 这是后续 IME polish, 中文常用字不会命中.
                const auto cp = static_cast<char32_t>(wparam);
                if (EventDispatcher::dispatch_text_input(state, cp)) {
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    return 0;
                }
            }
            break;
        // v1.4 (2026-04-28): IME 候选窗 / 组词窗位置跟随 focused widget。
        //
        // 旧行为: jtUI 框架不处理 WM_IME_*, OS 默认把 IME 候选窗位置放在窗口左
        // 上角 (frameless 自定义 caption 又叠加 DWM 偏移, 视觉上常落到屏幕右上
        // 角"飘着"), 用户在 ChatPage / Login / Settings 输入中文体验极差。
        //
        // 新行为: 拦 WM_IME_STARTCOMPOSITION (开始组词) + WM_IME_COMPOSITION
        // (组词中, 持续命中保证多字符输入候选窗位置实时刷新), 通过 ImmGetContext
        // 拿当前 IMC, 用 ImmSetCompositionWindow + ImmSetCandidateWindow 把组词
        // 窗 / 候选窗的 ptCurrentPos 钉到 focused widget frame 的左下角 (设备
        // 坐标), 让候选窗紧贴在输入框下方, 与系统原生输入框 (Edit 控件) 行为
        // 一致。
        //
        // 简化: 用 widget frame 左下角作为候选锚点, 不精确到光标 caret 位置 —
        // 后续要做精确跟随时, 让 Widget 暴露 caret_local_rect() 接口, 这里取
        // frame.x + caret_rect.x 即可。当前 widget 内部光标位置不外露, 用 frame
        // 已能解决"跑到屏幕右上"的核心问题。
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
            if (window != nullptr) {
                EventState state = load_event_state(*window);
                if (state.focused != nullptr) {
                    const auto frame = state.focused->frame();
                    const float scale = WindowRuntimeAccess::dpi_scale(*window);
                    // 逻辑坐标 → 设备坐标 (Win32 Imm API 用 client 区设备像素)
                    POINT anchor{
                        static_cast<LONG>(frame.x * scale),
                        static_cast<LONG>((frame.y + frame.height) * scale),
                    };
                    HIMC himc = ImmGetContext(hwnd);
                    if (himc != nullptr) {
                        COMPOSITIONFORM cf{};
                        cf.dwStyle = CFS_FORCE_POSITION;
                        cf.ptCurrentPos = anchor;
                        ImmSetCompositionWindow(himc, &cf);
                        CANDIDATEFORM cand{};
                        cand.dwStyle = CFS_CANDIDATEPOS;
                        cand.ptCurrentPos = anchor;
                        ImmSetCandidateWindow(himc, &cand);
                        ImmReleaseContext(hwnd, himc);
                    }
                }
            }
            // 不 return 0, fall through 让 DefWindowProc 继续处理 IME 默认行为
            // (组词字符串绘制 / 提交到 WM_CHAR 等)。我们只调整候选窗位置。
            break;
        case WM_MOUSEWHEEL: {
            if (window == nullptr || WindowRuntimeAccess::content(*window) == nullptr) {
                return 0;
            }
            // WM_MOUSEWHEEL 的 lparam 是屏幕坐标，要转 client。
            POINT screen_pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            ScreenToClient(hwnd, &screen_pt);
            const PointF logical = device_to_logical(
                PointF{static_cast<float>(screen_pt.x), static_cast<float>(screen_pt.y)},
                WindowRuntimeAccess::dpi_scale(*window));
            // WHEEL_DELTA = 120 是一个"刻度"。除掉得到浮点 notch 数。
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam))
                                / static_cast<float>(WHEEL_DELTA);
            if (EventDispatcher::dispatch_pointer_wheel(
                    WindowRuntimeAccess::content(*window).get(), logical, delta)) {
                // v1.24: 平滑滚动靠 tick 缓动。但滚轮连续转动时 WM_MOUSEWHEEL
                // 会塞满消息队列、饿死低优先级的 WM_TIMER → tick 跳帧 → 滚动
                // 顿（拖拽走 WM_MOUSEMOVE、位置在事件里直接更新，故丝滑）。
                // 这里在 wheel 回调内直接 tick + 重绘一帧，保证每个 wheel 消息
                // 都推进缓动；timer 只负责 wheel 停下后的收尾缓动。
                ensure_animation_timer(*window);
                const float tick_delta = compute_tick_delta(*window);
                tick_widget_tree(*WindowRuntimeAccess::content(*window), tick_delta);
                repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (window != nullptr) {
                // GetKeyState 高位（0x8000）表示当前键按下，低位是 toggle 状态。
                // 这里只关心瞬时按下，所以用 < 0（int16_t 高位即符号位）。
                const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                // v1.4 (2026-04-28): Modal ESC 优先。WM_KEYDOWN 派发顺序:
                //   1) ESC + 无修饰键 → 模态层栈顶 on_escape (倒序遍历, 命中即停)
                //   2) 命中失败再走全局快捷键 (Application::shortcuts_)
                //   3) 再没匹配上才 dispatch_key_down 给 focused widget
                // 这样 AboutDialog / SidebarFlyoutMenu 等弹层 push 进来后, ESC
                // 可以无差别消费, 不和业务侧的 register_shortcut(ESC) 冲突。
                constexpr std::int32_t vk_escape = 0x1B;
                if (wparam == vk_escape && !ctrl && !shift && !alt &&
                    window->application() != nullptr &&
                    window->application()->try_dispatch_modal_escape()) {
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    return 0;
                }
                // 全局快捷键优先：先匹配 Application::shortcuts_，命中则消费，
                // 不再 dispatch 给 focused widget。Ctrl+B / Esc / Ctrl+, 等都走这里。
                if (window->application() != nullptr &&
                    window->application()->try_invoke_shortcut(
                        static_cast<std::int32_t>(wparam), ctrl, shift, alt)) {
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    return 0;
                }
                EventState state = load_event_state(*window);
                // VK_TAB 走焦点循环；shift 决定方向。Tab 不传给 widget——v1 简化，
                // TextInput 等不期望吃 Tab 当字符。如果未来要让 widget 自己消费 Tab，
                // 加 Widget::wants_tab() 之类 hook。
                constexpr std::int32_t vk_tab = 0x09;
                if (wparam == vk_tab) {
                    Widget* root_widget = WindowRuntimeAccess::content(*window).get();
                    if (EventDispatcher::advance_focus(state, root_widget, /*reverse=*/shift)) {
                        save_event_state(*window, state);
                        repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    }
                    return 0;
                }
                if (EventDispatcher::dispatch_key_down(state, static_cast<std::int32_t>(wparam),
                                                       shift, ctrl, alt)) {
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    return 0;
                }
            }
            break;
        case WM_TIMER:
            if (window != nullptr && wparam == kAnimationTimerId &&
                WindowRuntimeAccess::content(*window) != nullptr) {
                // v1.24 (2026-05-15): 周期 timer 常驻，不再每帧 kill-then-set。
                // 原模式帧间隔 = 渲染耗时 T + interval(16ms)，平滑滚动靠 tick
                // 时帧率掉到 ~38fps 且抖动（拖拽走 WM_MOUSEMOVE 无此 16ms 等待，
                // 所以"拖拽丝滑、滚轮顿"）。改成周期 timer 后帧间隔 = max(T,
                // 16ms)，动画全部结束才 KillTimer。
                //
                // 真实 wall-clock delta（避免硬编码 1/60 让 AudioPlayer 的
                // played_position 偏离 WASAPI 真实播放速度）。
                const float delta_seconds = compute_tick_delta(*window);

                if (tick_widget_tree(*WindowRuntimeAccess::content(*window),
                                      delta_seconds)) {
                    // 同步绘制：把 tick → pixel 关在同一个 WM_TIMER 回调里，
                    // 不走 InvalidateRect 的异步 WM_PAINT（会吃消息调度延迟抽帧）。
                    repaint_dirty_window_now(*window, FALSE, /*defer_large=*/false);
                    // timer 周期常驻，无需重新 SetTimer。
                } else {
                    // 没有 widget 还需要 tick —— 停掉周期 timer 省 CPU。
                    stop_animation_timer(*window);
                }
                return 0;
            }
            break;
        case WM_DPICHANGED:
            if (window != nullptr) {
                WindowRuntimeAccess::dpi_scale(*window) = dpi_to_scale(HIWORD(wparam));
                const auto* suggested_rect = reinterpret_cast<const RECT*>(lparam);
                if (suggested_rect != nullptr) {
                    SetWindowPos(hwnd, nullptr, suggested_rect->left, suggested_rect->top,
                                 suggested_rect->right - suggested_rect->left,
                                 suggested_rect->bottom - suggested_rect->top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }
                invalidate_window(*window, FALSE);
                return 0;
            }
            break;
        case WM_SIZE:
            if (window != nullptr) {
                // 拿新 client area 物理 size,转 logical(除 dpi_scale)。
                // wparam 是 size type (SIZE_MINIMIZED 等);lparam 低 16 位 = width,
                // 高 16 位 = height。
                if (wparam != SIZE_MINIMIZED) {
                    const float dpi_scale = normalize_scale(WindowRuntimeAccess::dpi_scale(*window));
                    const float new_w = static_cast<float>(LOWORD(lparam)) / dpi_scale;
                    const float new_h = static_cast<float>(HIWORD(lparam)) / dpi_scale;
                    // emit signal 让 host 重 build content tree。host 通常通过
                    // Application::post 排到帧间隙(避免在 WM_SIZE 栈里析构 widget)。
                    window->on_resized().emit(SizeF{new_w, new_h});
                }
                invalidate_window(*window, FALSE);
            } else {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (window != nullptr) {
                stop_animation_timer(*window);
                release_back_buffer(*window);
                WindowRuntimeAccess::native_handle(*window) = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProcW(hwnd, message, wparam, lparam);
    };
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = nullptr;
    window_class.lpszClassName = kWindowClassName;

    const ATOM atom = RegisterClassExW(&window_class);
    if (atom == 0) {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    registered = true;
    return true;
}

DWORD compute_window_style(const WindowOptions& options) {
    DWORD style = WS_SYSMENU | WS_MINIMIZEBOX;
    if (options.frameless) {
        style |= WS_POPUP;
    } else {
        style |= WS_CAPTION;
    }

    if (options.resizable) {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }

    return style;
}

void suppress_native_frame(HWND handle) {
    // v1.1 (2026-04-27): 让 frameless 窗口获得 DWM 原生外阴影 + 圆角. 让窗口
    // 与桌面有视觉分离感 (与 qml 主壳 / 现代 Windows 应用一致)。
    //
    // 关键技术点:
    //   1. DwmExtendFrameIntoClientArea(MARGINS{0,0,0,1}): 让 DWM 把 client
    //      区域延伸 1px 到原生 frame 之内, 触发 DWM 在 client 之外画 frame
    //      shadow / 圆角。0/0/0/1 是最小值 (只在底部延伸 1px), 用户视觉感知
    //      不到这 1px 的延伸但能拿到完整 shadow。
    //   2. NCRENDERING_POLICY 保持默认 (USEWINDOWSTYLE), 不再 DWMNCRP_DISABLED:
    //      之前禁了 NC 渲染 → DWM 不画 shadow。WM_NCPAINT 在 wnd_proc 里
    //      return 0 已经屏蔽 native caption / 边框文字, NC rendering 只剩 DWM
    //      shadow 部分。
    //   3. NCCALCSIZE return 0 (wnd_proc 已有), 让 client 占满窗口, frameless
    //      视觉不变。
    const MARGINS shadow_margins{0, 0, 0, 1};
    DwmExtendFrameIntoClientArea(handle, &shadow_margins);

    SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

} // namespace
#endif

Window::Window(WindowOptions options) : options_(std::move(options)) {}

const WindowOptions& Window::options() const noexcept {
    return options_;
}

const std::string& Window::title() const noexcept {
    return options_.title;
}

bool Window::visible() const noexcept {
    return visible_;
}

Widget* Window::content() const noexcept {
    return content_.get();
}

void Window::set_title(std::string title) {
    options_.title = std::move(title);
#if defined(_WIN32)
    if (native_handle_ != nullptr) {
        const std::wstring wide_title = utf8_to_wide(options_.title);
        SetWindowTextW(reinterpret_cast<HWND>(native_handle_),
                       wide_title.empty() ? L"" : wide_title.c_str());
    }
#endif
}

void Window::set_visible(bool visible) noexcept {
    visible_ = visible;
#if defined(_WIN32)
    if (native_handle_ != nullptr) {
        ShowWindow(reinterpret_cast<HWND>(native_handle_), visible_ ? SW_SHOW : SW_HIDE);
    }
#endif
}

void Window::set_content(std::unique_ptr<Widget> root) {
    // 关键：旧 content 不立刻析构，挪到 pending_destroy_。
    // 否则当 set_content 在事件回调里被调用（典型 rebuild 模式）时，
    // Signal::emit 的栈还在旧 widget 内部 unwind，立刻 delete 旧树会
    // 触发 use-after-free。pending_destroy_ 由消息循环每次
    // DispatchMessage 后 drain，那时所有事件 unwind 已完成。
    if (content_) {
        pending_destroy_.push_back(std::move(content_));
    }
    content_ = std::move(root);
#if defined(_WIN32)
    WindowRuntimeAccess::hovered_widget(*this) = nullptr;
    WindowRuntimeAccess::pressed_widget(*this) = nullptr;
    WindowRuntimeAccess::focused_widget(*this) = nullptr;
    invalidate_window(*this, TRUE);
#endif
}

void Window::drain_pending_destroy() noexcept {
    // clear() 会真正析构里面的 unique_ptr<Widget>。在消息循环里调，确保
    // 所有事件回调和 Signal::emit unwind 已彻底返回。
    pending_destroy_.clear();
}

Window& Application::create_window(WindowOptions options) {
    windows_.push_back(std::make_unique<Window>(std::move(options)));
    windows_.back()->application_ = this;
    return *windows_.back();
}

const std::vector<std::unique_ptr<Window>>& Application::windows() const noexcept {
    return windows_;
}

ShortcutId Application::register_shortcut(Shortcut shortcut) {
    const ShortcutId id = next_shortcut_id_++;
    shortcuts_.push_back(ShortcutEntry{id, std::move(shortcut)});
    return id;
}

void Application::remove_shortcut(ShortcutId id) {
    shortcuts_.erase(
        std::remove_if(shortcuts_.begin(), shortcuts_.end(),
                       [id](const ShortcutEntry& e) { return e.id == id; }),
        shortcuts_.end());
}

// v1.10 (2026-04-29): 真实 DirectWrite 字宽测量。复用 DirectTextSession 的
// dwrite_factory + text_format cache。CreateTextLayout 给 100000 max width
// 让 NO_WRAP 单行布局, GetMetrics 拿 widthIncludingTrailingWhitespace。
//
// 注: 返回 logical px (96 DPI) 宽度, 与 widget RectF 同坐标系; replay_paint_commands
// 内部把 font_size / bounds 都按 dpi_scale 缩放后再交 D2D 渲染, 渲染出的 device px
// 已被 logical 反向除回去 — 直接返回 96 DPI advance, 调用方不需 * scale。
#if defined(_WIN32)
float Application::measure_line_height(float font_size, bool bold) {
    if (font_size <= 0.0F) return 0.0F;
    DirectTextSession& session = direct_text_session();
    if (!session.ok()) return font_size * 1.4F;
    IDWriteTextFormat* format = session.text_format(font_size, bold, TextAlignment::Leading);
    if (format == nullptr) return font_size * 1.4F;
    IDWriteTextLayout* layout = nullptr;
    const HRESULT hr = session.dwrite_factory()->CreateTextLayout(
        L"M", 1, format, 100000.0F, 100000.0F, &layout);
    if (FAILED(hr) || layout == nullptr) return font_size * 1.4F;
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    layout->Release();
    // metrics.height = paragraph height = 1 line * line_height (单字单行)
    return metrics.height > 0.0F ? metrics.height : font_size * 1.4F;
}

float Application::measure_text_width(const std::string& utf8, float font_size, bool bold) {
    if (utf8.empty() || font_size <= 0.0F) return 0.0F;
    DirectTextSession& session = direct_text_session();
    if (!session.ok()) return 0.0F;
    IDWriteTextFormat* format = session.text_format(font_size, bold, TextAlignment::Leading);
    if (format == nullptr) return 0.0F;
    const std::wstring wide = utf8_to_wide(utf8);
    if (wide.empty()) return 0.0F;
    IDWriteTextLayout* layout = nullptr;
    const HRESULT hr = session.dwrite_factory()->CreateTextLayout(
        wide.c_str(), static_cast<UINT32>(wide.size()),
        format, /*maxWidth=*/100000.0F, /*maxHeight=*/100000.0F, &layout);
    if (FAILED(hr) || layout == nullptr) return 0.0F;
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    layout->Release();
    return metrics.widthIncludingTrailingWhitespace;
}
#else
// Linux / 非 Win32: 没有 DirectWrite, 用启发式估算让上层布局逻辑能跑。
// 宽度按 0.6 倍 font_size * 字符数估算 (CJK 全角字符不准, 但 Linux 只用于
// platform-neutral 单测和 MinGW 交叉编译, 真实渲染在 Win 上). bold 也无差异。
float Application::measure_line_height(float font_size, bool /*bold*/) {
    return font_size > 0.0F ? font_size * 1.4F : 0.0F;
}

float Application::measure_text_width(const std::string& utf8, float font_size, bool /*bold*/) {
    if (utf8.empty() || font_size <= 0.0F) return 0.0F;
    return font_size * 0.6F * static_cast<float>(utf8.size());
}
#endif

bool Application::try_invoke_shortcut(std::int32_t key_code, bool ctrl, bool shift,
                                      bool alt) const {
    for (const auto& entry : shortcuts_) {
        const Shortcut& s = entry.shortcut;
        if (s.key_code == key_code && s.ctrl == ctrl && s.shift == shift && s.alt == alt) {
            if (s.callback) {
                s.callback();
            }
            return true;
        }
    }
    return false;
}

// v1.4 (2026-04-28): ModalLayer 注册 + 派发实现。
ModalLayerId Application::push_modal_layer(ModalLayer layer) {
    const ModalLayerId id = next_modal_layer_id_++;
    modal_layers_.push_back(ModalLayerEntry{id, std::move(layer)});
    return id;
}

void Application::pop_modal_layer(ModalLayerId id) {
    modal_layers_.erase(
        std::remove_if(modal_layers_.begin(), modal_layers_.end(),
                       [id](const ModalLayerEntry& e) { return e.id == id; }),
        modal_layers_.end());
}

bool Application::try_dispatch_modal_escape() {
    // 倒序: 最后 push 的栈顶最先吃 ESC, 模拟"先关最近打开的弹窗"。
    // on_escape 在迭代时可能调 pop_modal_layer 删自己, vector 迭代器失效 — 故
    // 拷贝一份 id 序列, 逐个查表派发。
    std::vector<ModalLayerId> ids;
    ids.reserve(modal_layers_.size());
    for (const auto& e : modal_layers_) ids.push_back(e.id);
    for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
        const ModalLayerId id = *it;
        // 重新查找 entry (前面的 callback 可能已经把它 pop 掉)
        auto found = std::find_if(modal_layers_.begin(), modal_layers_.end(),
                                  [id](const ModalLayerEntry& e) { return e.id == id; });
        if (found == modal_layers_.end()) continue;
        if (!found->layer.on_escape) continue;
        // 拷贝 callback 后再调用, 防止 callback 内部 pop 自己导致 found 引用悬空。
        auto cb = found->layer.on_escape;
        if (cb()) return true;
    }
    return false;
}

bool Application::try_dispatch_modal_outside_press(PointF point) {
    // 全量遍历 (拷贝 id), 任意 layer 的 contains_inside(point) 返回 false 即触发
    // 该 layer 的 on_outside_press。返回 true 表示有 layer 被通知 (本次点击应被
    // 模态拦截消费, 不再走 widget dispatch)。
    bool any_triggered = false;
    std::vector<ModalLayerId> ids;
    ids.reserve(modal_layers_.size());
    for (const auto& e : modal_layers_) ids.push_back(e.id);
    for (const auto id : ids) {
        auto found = std::find_if(modal_layers_.begin(), modal_layers_.end(),
                                  [id](const ModalLayerEntry& e) { return e.id == id; });
        if (found == modal_layers_.end()) continue;
        if (!found->layer.contains_inside) continue;  // 不参与 outside-press 的 layer 跳过
        const bool inside = found->layer.contains_inside(point);
        if (inside) continue;
        if (found->layer.on_outside_press) {
            // 同 escape: 拷贝 callback 后再调, 防 callback 内部 pop 自己。
            auto cb = found->layer.on_outside_press;
            cb();
            any_triggered = true;
        }
    }
    return any_triggered;
}

Application::~Application() {
    // 等所有 run_async worker 退出 — worker 内部 *this 还会被访问(post + release)。
    // 不等就 UAF:Application 死后 worker 完成时访问 self → 段错误。
    std::unique_lock<std::mutex> lk(worker_done_mu_);
    worker_done_cv_.wait(lk, [this] { return in_flight_workers_.load() == 0; });
}

void Application::register_async_worker() {
    in_flight_workers_.fetch_add(1, std::memory_order_acq_rel);
}

void Application::release_async_worker() {
    if (in_flight_workers_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        std::lock_guard<std::mutex> lk(worker_done_mu_);
        worker_done_cv_.notify_all();
    }
}

void Application::post(std::function<void()> callback) {
    if (!callback) return;
    {
        std::lock_guard<std::mutex> lk(pending_mu_);
        pending_callbacks_.push_back(std::move(callback));
    }

#if defined(_WIN32)
    // 触发 message pump:发一个自定义 WM_APP 消息让 WndProc 在下次 iteration 调
    // drain_pending_callbacks。pump 任意 windows 上的 hwnd 都行;选第一个。
    // 注:windows_ 由主线程写,post 可能从 worker thread 调 — 读 windows_ 在
    // Application 创建 window 之后才会发生(host 生命周期保证),且 native_handle_
    // 只在 create_window 时设一次,worker 读不会和写竞争。
    if (!windows_.empty() && windows_.front()->native_handle_ != nullptr) {
        // 复用文件顶部 anonymous namespace 中定义的 kPendingCallbackMessage,
        // 避免局部 constexpr 隐藏全局声明触发 C4459 警告。
        PostMessageW(reinterpret_cast<HWND>(windows_.front()->native_handle_),
                     kPendingCallbackMessage, 0, 0);
    }
#else
    // Linux:没 GUI loop,Application::run 直接 return 0。post 没"延后"语义,
    // 立即同步调 callback(因为不会有 "下一帧" 的概念)。drain 在这里执行。
    drain_pending_callbacks();
#endif
}

void Application::drain_pending_callbacks() {
    // swap 出本地 vector 再调,让 callback 内部 post 新 callback 不会让我们
    // 当前迭代器失效(新 callback 进入 pending_callbacks_,留待下次 drain)。
    // mutex 只盖 swap,callback 调用过程在锁外 — 否则 callback 内部调 post
    // 会死锁。
    std::vector<std::function<void()>> local;
    {
        std::lock_guard<std::mutex> lk(pending_mu_);
        local.swap(pending_callbacks_);
    }
    for (auto& cb : local) {
        if (cb) cb();
    }
}

#if defined(_WIN32)
namespace {

// 把系统 timer 精度拉到 1ms。Windows 默认系统时钟 tick 是 15.6ms，意味着
// SetTimer(16ms) 实际到 WM_TIMER 的时间可能飘到 30ms+，这是 Button 按下后
// "差半拍才变暗"的最根本来源。timeBeginPeriod(1) 之后，16ms 的 timer 能
// 稳定在 ~16ms 触发。退出时 timeEndPeriod 对齐恢复。
//
// Windows 10 21H2 之后 timeBeginPeriod 改为进程内生效，不会再全局拖累其他
// 进程功耗；旧系统仍然全局 —— 对一个活跃前台 UI 应用来说这个代价可接受。
class HighResolutionTimerScope {
  public:
    HighResolutionTimerScope() {
        TIMECAPS caps{};
        if (timeGetDevCaps(&caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            period_ = std::max<UINT>(caps.wPeriodMin, 1U);
            if (timeBeginPeriod(period_) == TIMERR_NOERROR) {
                active_ = true;
            }
        }
    }

    HighResolutionTimerScope(const HighResolutionTimerScope&) = delete;
    HighResolutionTimerScope& operator=(const HighResolutionTimerScope&) = delete;

    ~HighResolutionTimerScope() {
        if (active_) {
            timeEndPeriod(period_);
        }
    }

  private:
    UINT period_{1};
    bool active_{false};
};

} // namespace
#endif

int Application::run() {
#if defined(_WIN32)
    if (windows_.empty()) {
        return 0;
    }

    HighResolutionTimerScope high_res_timer{};

    (void)enable_process_dpi_awareness();

    if (!register_window_class()) {
        return -1;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);

    for (const auto& window : windows_) {
        const DWORD style = compute_window_style(window->options_);
        const float initial_dpi_scale = system_dpi_scale();
        window->dpi_scale_ = initial_dpi_scale;
        RECT bounds{
            0,
            0,
            scale_to_long(window->options_.size.width, initial_dpi_scale),
            scale_to_long(window->options_.size.height, initial_dpi_scale),
        };

        // frameless 下 WM_NCCALCSIZE 返回 0，client == window，
        // 再 AdjustWindowRectEx 会把 WS_THICKFRAME 的边框也算进 client，
        // 导致请求的 size 比实际 client 小一圈，右下会露出底层背景。
        if (!window->options_.frameless) {
            if (AdjustWindowRectExForDpi(
                    &bounds, style, FALSE, 0,
                    static_cast<UINT>(round_to_int(initial_dpi_scale * 96.0F))) == FALSE) {
                AdjustWindowRectEx(&bounds, style, FALSE, 0);
            }
        }

        const std::wstring wide_title = utf8_to_wide(window->options_.title);
        HWND handle =
            CreateWindowExW(0, kWindowClassName, wide_title.empty() ? L"jtUI" : wide_title.c_str(),
                            style, CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left,
                            bounds.bottom - bounds.top, nullptr, nullptr, instance, window.get());

        if (handle == nullptr) {
            return -1;
        }

        window->native_handle_ = handle;
        window->dpi_scale_ = window_dpi_scale(handle);
        if (window->options_.frameless) {
            suppress_native_frame(handle);
        }

        // v1.22 (2026-05-04): 接受 Explorer 拖文件释放。Win32 legacy 路径
        // (DragAcceptFiles + WM_DROPFILES) 比 OLE IDropTarget 简单一个量级,
        // 且 jtui 暂不需要 drag-enter / drag-over 视觉反馈, 走这条路足够。
        // 业务侧通过 application.set_files_dropped_callback 注册 drop 回调,
        // 平台层在 WM_DROPFILES 取出 HDROP 转 utf8 路径列表 + drop 点回调过去。
        DragAcceptFiles(handle, TRUE);
        // UIPI 过滤: Windows Vista+ 默认阻止"低权限源 -> 高权限目标"的 drop,
        // 例如普通 Explorer 拖文件到管理员启动的本应用会被静默忽略 (没有任何
        // 可见错误)。ChangeWindowMessageFilterEx 显式放行 WM_DROPFILES 让
        // 这种场景也能 work。同 app 同权限不影响, 失败 (旧系统 / 政策受限)
        // 也只是丢失高低权限场景, 同权限仍正常 — 忽略返回值。
        ChangeWindowMessageFilterEx(handle, WM_DROPFILES,    MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(handle, WM_COPYDATA,     MSGFLT_ALLOW, nullptr);
        ChangeWindowMessageFilterEx(handle, 0x0049U /*WM_COPYGLOBALDATA*/,
                                    MSGFLT_ALLOW, nullptr);

        // v1.4 (2026-04-28): 窗口居中。CreateWindowExW 用 CW_USEDEFAULT 时 Win32
        // 自己挑位置 (历史行为偏屏幕一侧, 多显示器 / 高 DPI 场景常出现在右上角),
        // 用户体验差。改为创建后用 monitor 工作区 (rcWork 不含任务栏) 居中放置。
        // 不直接在 CreateWindowExW 传定位坐标, 因为高 DPI 情况下 size 在创建后
        // 才被 WM_DPICHANGED 校正; 创建后用 GetWindowRect 拿真实尺寸再算居中更稳。
        {
            HMONITOR monitor = MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(monitor, &mi) != FALSE) {
                RECT win_rect{};
                if (GetWindowRect(handle, &win_rect) != FALSE) {
                    const int win_w = win_rect.right - win_rect.left;
                    const int win_h = win_rect.bottom - win_rect.top;
                    const int work_w = mi.rcWork.right - mi.rcWork.left;
                    const int work_h = mi.rcWork.bottom - mi.rcWork.top;
                    const int cx = mi.rcWork.left + (work_w - win_w) / 2;
                    const int cy = mi.rcWork.top + (work_h - win_h) / 2;
                    SetWindowPos(handle, nullptr, cx, cy, 0, 0,
                                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }

        ensure_animation_timer(*window);
        ShowWindow(handle, window->visible_ ? SW_SHOW : SW_HIDE);
        UpdateWindow(handle);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        // 事件 unwind 完成 —— 现在才安全地真正 delete 旧 widget 树。
        for (auto& w : windows_) {
            if (w) w->drain_pending_destroy();
        }
    }

    return static_cast<int>(message.wParam);
#else
    return 0;
#endif
}

} // namespace hui
