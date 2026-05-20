#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hui/foundation/color.hpp"
#include "hui/foundation/geometry.hpp"
#include "hui/foundation/pixel_buffer.hpp"

namespace hui {

enum class TextAlignment : std::uint8_t {
    Leading,   // 左对齐(LTR)/ 顶对齐
    Center,    // 居中
    Trailing,  // 右对齐(LTR)/ 底对齐 — DWrite DWRITE_TEXT_ALIGNMENT_TRAILING
};

enum class DrawCommandKind : std::uint8_t {
    FillRect,
    StrokeRect,
    FillRoundedRect,
    StrokeRoundedRect,
    FillEllipse,
    StrokeEllipse,
    Line,
    Text,
    DrawTexture,
    Bezier,            // 三次贝塞尔曲线 stroke,p0=bounds.{x,y} p1=control1 p2=control2 p3=end_point
    FillPolygon,       // 任意多边形 fill,顶点列表存 polygon_points
    FillGradientRect,  // 圆角矩形 + 纵向线性渐变(color = 顶部色,gradient_end_color = 底部色)
    // v1.1 (2026-04-27): 严格 clip 支持。PushClip 把 bounds (logical) 压入 D2D
    // PushAxisAlignedClip 栈, 后续命令被严格 clip 到该矩形, 直到对应 PopClip。
    // 用例: paint_widget_tree 在 clips_children=true 容器 (ScrollView 等) 进入
    // children 遍历前 push_clip(child_clip), 遍历完 pop_clip。让 scroll 滚出
    // viewport 的 child paint 命令真被 clip 掉, 不再溢到容器外的区域。
    PushClip,
    PopClip,
    // v1.22 (2026-05-15): 高斯模糊阴影。bounds 是被投影的目标矩形，
    // corner_radius 圆角；shadow_offset / shadow_blur / shadow_spread 走 CSS
    // box-shadow 语义；color 是阴影色（通常半透明黑/暗调）。后端用 D2D 1.1
    // CLSID_D2D1Shadow Effect 真高斯模糊。
    FillShadow,
    // v1.23 (2026-05-15): 毛玻璃 / backdrop blur。bounds 是模糊区域，
    // corner_radius 圆角，shadow_blur 复用为模糊半径，color 是叠在模糊层之上
    // 的半透明 tint。后端 D2D 1.1：CopyFromRenderTarget 抓 bounds 区域已绘
    // 内容 → CLSID_D2D1GaussianBlur 模糊 → DrawImage 回原位 → 叠 tint。
    // 命令必须排在被模糊内容的命令之后（z-order 上 backdrop widget 在后）。
    FillBackdropBlur,
};

// 多边形顶点数据(Polygon 命令携带)。共享 vector 让命令复制廉价。
struct PolygonPoints {
    std::vector<PointF> points;
};

struct DrawCommand {
    DrawCommandKind kind {DrawCommandKind::FillRect};
    RectF bounds {};
    Color color {};
    std::string text {};
    // 文本字体族。空串 = 走 Application 默认 (Segoe UI + 中文 fallback 链)。
    // 用例:  "codicon"  Codicon 字体 (Icon widget 渲染 codicon glyph)
    //        "Menlo"    Code 等宽字体 (Markdown / Code 渲染)
    // 自定义字体需 platform 层提前注册到 DirectWrite (见 platform/win32/font/)。
    std::string font_family {};
    std::uint64_t resource_id {0};
    float stroke_thickness {1.0F};
    float corner_radius {0.0F};
    float font_size {14.0F};
    bool bold {false};
    PointF end_point {};
    TextAlignment text_alignment {TextAlignment::Leading};
    // DrawTexture 命令携带的像素数据。shared_ptr<vector> 共享，复制命令廉价。
    // resource_id 是给未来纹理 registry 用的键；v1 replay 直接用 pixel_buffer。
    PixelBuffer pixel_buffer {};
    // Bezier 命令的 p1 / p2 控制点(p0 = bounds 起点,p3 = end_point)。
    PointF bezier_c1 {};
    PointF bezier_c2 {};
    // FillPolygon 顶点(顺序连接,自动闭合)。
    PolygonPoints polygon {};
    // FillGradientRect 底部色(顶部色用 color 字段)。
    Color gradient_end_color {};
    // FillShadow: 阴影偏移 / 高斯模糊半径 / 扩张。bounds=目标矩形, color=阴影色,
    // corner_radius=目标圆角。
    PointF shadow_offset {};
    float  shadow_blur {0.0F};
    float  shadow_spread {0.0F};
};

class PaintContext {
public:
    void fill_rect(RectF bounds, Color color) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillRect;
        c.bounds = bounds;
        c.color = color;
        commands_.push_back(std::move(c));
    }

    void stroke_rect(RectF bounds, Color color, float thickness = 1.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::StrokeRect;
        c.bounds = bounds;
        c.color = color;
        c.stroke_thickness = thickness;
        commands_.push_back(std::move(c));
    }

    void fill_rounded_rect(RectF bounds, Color color, float radius) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillRoundedRect;
        c.bounds = bounds;
        c.color = color;
        c.corner_radius = radius;
        commands_.push_back(std::move(c));
    }

    void stroke_rounded_rect(RectF bounds, Color color, float radius, float thickness = 1.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::StrokeRoundedRect;
        c.bounds = bounds;
        c.color = color;
        c.corner_radius = radius;
        c.stroke_thickness = thickness;
        commands_.push_back(std::move(c));
    }

    void fill_ellipse(RectF bounds, Color color) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillEllipse;
        c.bounds = bounds;
        c.color = color;
        commands_.push_back(std::move(c));
    }

    void stroke_ellipse(RectF bounds, Color color, float thickness = 1.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::StrokeEllipse;
        c.bounds = bounds;
        c.color = color;
        c.stroke_thickness = thickness;
        commands_.push_back(std::move(c));
    }

    void line(PointF start, PointF end, Color color, float thickness = 1.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::Line;
        c.bounds = RectF{start.x, start.y, 0.0F, 0.0F};
        c.color = color;
        c.stroke_thickness = thickness;
        c.end_point = end;
        commands_.push_back(std::move(c));
    }

    // 三次贝塞尔曲线(p0 起点,p1/p2 控制点,p3 终点)。stroke only — 路径填充
    // 用 fill_polygon 把 bezier 离散后传顶点(避免 PathGeometry 复杂度爆炸)。
    void draw_bezier(PointF p0, PointF p1, PointF p2, PointF p3, Color color,
                      float thickness = 1.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::Bezier;
        c.bounds = RectF{p0.x, p0.y, 0.0F, 0.0F};
        c.color = color;
        c.stroke_thickness = thickness;
        c.end_point = p3;
        c.bezier_c1 = p1;
        c.bezier_c2 = p2;
        commands_.push_back(std::move(c));
    }

    // 任意多边形填充。顶点列表自动闭合(最后一点连回第一点)。3 个以下的顶点
    // 视为退化,后端 noop。复杂多边形 (>200 顶点) 后端可能裁断 — 大型几何请
    // 拆成多个小调用。
    void fill_polygon(std::vector<PointF> points, Color color) {
        if (points.size() < 3) return;
        DrawCommand c{};
        c.kind = DrawCommandKind::FillPolygon;
        c.color = color;
        // bounds 用顶点 AABB 让 dirty rect 跟踪有数。
        float x0 = points[0].x, y0 = points[0].y, x1 = x0, y1 = y0;
        for (const auto& p : points) {
            if (p.x < x0) x0 = p.x;
            if (p.x > x1) x1 = p.x;
            if (p.y < y0) y0 = p.y;
            if (p.y > y1) y1 = p.y;
        }
        c.bounds = RectF{x0, y0, x1 - x0, y1 - y0};
        c.polygon.points = std::move(points);
        commands_.push_back(std::move(c));
    }

    // 圆角矩形 + 纵向线性渐变(top → bottom)。后端 D2D 用 LinearGradientBrush
    // (start.y = bounds.y, end.y = bounds.y + bounds.height)。radius=0 → 普通矩形。
    void fill_gradient_rect(RectF bounds, Color color_top, Color color_bottom, float radius = 0.0F) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillGradientRect;
        c.bounds = bounds;
        c.color = color_top;
        c.gradient_end_color = color_bottom;
        c.corner_radius = radius;
        commands_.push_back(std::move(c));
    }

    void draw_text(
        RectF bounds,
        std::string text,
        Color color,
        TextAlignment alignment = TextAlignment::Leading,
        float font_size = 14.0F,
        bool bold = false) {
        DrawCommand c{};
        c.kind = DrawCommandKind::Text;
        c.bounds = bounds;
        c.color = color;
        c.text = std::move(text);
        c.font_size = font_size;
        c.bold = bold;
        c.text_alignment = alignment;
        commands_.push_back(std::move(c));
    }

    // 带字体族指定的 draw_text。font_family 空时等价于上面的重载。
    void draw_text_with_font(
        RectF bounds,
        std::string text,
        std::string font_family,
        Color color,
        TextAlignment alignment = TextAlignment::Leading,
        float font_size = 14.0F,
        bool bold = false) {
        DrawCommand c{};
        c.kind = DrawCommandKind::Text;
        c.bounds = bounds;
        c.color = color;
        c.text = std::move(text);
        c.font_family = std::move(font_family);
        c.font_size = font_size;
        c.bold = bold;
        c.text_alignment = alignment;
        commands_.push_back(std::move(c));
    }

    // 渲染纹理。直接传 PixelBuffer，replay 端用它构造 Gdiplus::Bitmap 并 DrawImage。
    // resource_id 留着给将来做 GPU 端 texture 缓存 / Skia SkImage 的 key（v1 没用上）。
    // pixel_buffer.empty() 时视作占位，后端画一个 bg_raised 底色 + "Texture" 文字。
    void draw_texture(RectF bounds, PixelBuffer buffer,
                      Color tint = Color{1.0F, 1.0F, 1.0F, 1.0F},
                      std::uint64_t resource_id = 0) {
        DrawCommand c{};
        c.kind = DrawCommandKind::DrawTexture;
        c.bounds = bounds;
        c.color = tint;
        c.resource_id = resource_id;
        c.pixel_buffer = std::move(buffer);
        commands_.push_back(std::move(c));
    }

    // v1.1 严格 clip: 把 bounds (logical) 压入栈。后续命令被严格 clip 到该
    // 矩形, 直到对应的 pop_clip(). push/pop 必须配对使用; renderer 用 D2D
    // PushAxisAlignedClip / PopAxisAlignedClip 实现。
    void push_clip(RectF bounds) {
        DrawCommand c{};
        c.kind = DrawCommandKind::PushClip;
        c.bounds = bounds;
        commands_.push_back(std::move(c));
    }

    void pop_clip() {
        DrawCommand c{};
        c.kind = DrawCommandKind::PopClip;
        commands_.push_back(std::move(c));
    }

    // v1.22 (2026-05-15): 投在 bounds (含 corner_radius 圆角) 的高斯模糊阴影。
    // 语义对齐 CSS box-shadow：offset 是阴影中心相对目标的偏移；blur 是模糊半径
    // (内部 standard_deviation = blur / 3，让"blur=8" 看起来跟 CSS 一致)；
    // spread 正向外扩 / 负向内缩；color 通常半透明黑/暗。后端 D2D 1.1 用
    // CLSID_D2D1Shadow Effect 实现真高斯模糊；1.1 不可用时降级跳过不画。
    //
    // 业务一般通过 Panel::set_shadow(theme::Shadow) 间接调用，不直接 fill_shadow。
    void fill_shadow(RectF bounds, float corner_radius,
                     PointF offset, float blur, float spread, Color color) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillShadow;
        c.bounds = bounds;
        c.color = color;
        c.corner_radius = corner_radius;
        c.shadow_offset = offset;
        c.shadow_blur = blur;
        c.shadow_spread = spread;
        commands_.push_back(std::move(c));
    }

    // v1.23 (2026-05-15): 毛玻璃 / backdrop blur。把 bounds 区域已经画好的
    // 内容做高斯模糊，再叠一层半透明 tint —— 经典 macOS 磨砂导航条效果。
    // blur 是模糊半径；tint 通常是品牌底色的低 alpha 版（透出模糊背景同时
    // 保持可读性）。必须在被模糊内容画完之后才发出本命令（widget z-order
    // 上 backdrop widget 排在后面即可）。后端 D2D 1.1 不可用时降级跳过。
    void fill_backdrop_blur(RectF bounds, float corner_radius,
                            float blur, Color tint) {
        DrawCommand c{};
        c.kind = DrawCommandKind::FillBackdropBlur;
        c.bounds = bounds;
        c.color = tint;
        c.corner_radius = corner_radius;
        c.shadow_blur = blur;
        commands_.push_back(std::move(c));
    }

    [[nodiscard]] const std::vector<DrawCommand>& commands() const noexcept {
        return commands_;
    }

    // v1.20 (2026-05-04): 直接 push 一条已构造好的 DrawCommand。
    // 用例: MarkdownText 等 widget 缓存自己上一帧产出的命令切片, 滚动平移时
    // 不再走 layout_or_paint 重排版, 直接复制 cache 出的命令、平移坐标、
    // push_command 喂回 PaintContext。比每帧重跑 markdown 解析 + 行内 run
    // wrap 快一个量级。
    void push_command(DrawCommand cmd) {
        commands_.push_back(std::move(cmd));
    }

    void clear() noexcept {
        // vector::clear 不 shrink capacity，下一帧的 push_back 都是 O(1) 无分配。
        commands_.clear();
    }

    // 预留容量。Gallery 场景一帧典型 200-400 条命令，预留能把头几帧的
    // push_back realloc 开销也省掉。
    void reserve(std::size_t n) { commands_.reserve(n); }

private:
    std::vector<DrawCommand> commands_ {};
};

}  // namespace hui
