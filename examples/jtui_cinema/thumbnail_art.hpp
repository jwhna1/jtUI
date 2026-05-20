#pragma once

// jtui_cinema 自定义缩略图 widget —— 纯几何 + 渐变，零 PNG 依赖。
//
// 5 种地理风格化插画（剪影 + 暖橘点缀，影院剧照气质）：
//   id=0  China        屋檐 + 红灯笼
//   id=1  Mongolia     草原 + 蒙古包 + 暖日
//   id=2  Vietnam      椰树叶剪影
//   id=3  Japan        富士山 + 红日
//   id=4  Taiwan       夜市招牌堆叠 + 灯笼串
//
// 缩略图按"夜景剧照"调，固定走深色背景；accent 走业务侧传入的暖橘 brand color，
// 跟 theme 切换时颜色微调（dark 暖橘亮，light 暖橘加深）。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <vector>

#include "jtui/jtui.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

namespace jtui_cinema {

class ThumbnailArt : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "ThumbnailArt";
    }

    // CSS overflow:hidden 语义：runtime 会自动 push_clip(self.frame) / pop_clip
    // 包裹 paint()，所有越过 frame 矩形的几何被截掉。圆角内的越界（小三角飞
    // 出 corner）还要靠"顶点远离 corner"约束，矩形 clip 帮不到圆角。
    [[nodiscard]] bool clips_self() const noexcept override { return true; }

    void set_style(int style_id) noexcept { style_id_ = style_id; }
    void set_accent(jtui::Color accent) noexcept { accent_ = accent; }
    void set_corner_radius(float r) noexcept { corner_radius_ = r; }

    void paint(hui::PaintContext& ctx) const override {
        const auto f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        switch (style_id_) {
            case 0:  paint_china(ctx, f);    break;
            case 1:  paint_mongolia(ctx, f); break;
            case 2:  paint_vietnam(ctx, f);  break;
            case 3:  paint_japan(ctx, f);    break;
            case 4:  paint_taiwan(ctx, f);   break;
            default: paint_china(ctx, f);    break;
        }
    }

private:
    // 共用小工具：取 frame 相对点
    [[nodiscard]] static hui::PointF p(const hui::RectF& f, float nx, float ny) noexcept {
        return {f.x + f.width * nx, f.y + f.height * ny};
    }
    [[nodiscard]] static hui::RectF r(const hui::RectF& f, float nx, float ny,
                                       float nw, float nh) noexcept {
        return {f.x + f.width * nx, f.y + f.height * ny,
                f.width * nw, f.height * nh};
    }

    // ───────── Style 0: China —— 屋檐 + 红灯笼 ───────────────────
    void paint_china(hui::PaintContext& ctx, const hui::RectF& f) const {
        // bg：暗酒红渐变
        ctx.fill_gradient_rect(f,
            jtui::Color::from_hex("#2A0F12"),
            jtui::Color::from_hex("#180609"),
            corner_radius_);

        // 远屋檐（2 层叠瓦，远暗近亮）
        // 远屋檐
        ctx.fill_polygon({
            p(f, 0.00F, 0.55F), p(f, 0.20F, 0.45F),
            p(f, 0.50F, 0.42F), p(f, 0.80F, 0.45F),
            p(f, 1.00F, 0.55F), p(f, 1.00F, 0.65F),
            p(f, 0.00F, 0.65F)},
            jtui::Color::from_hex("#3A1218"));

        // 中屋檐（前飞檐）—— 顶点收回 frame 内 [0, 1]，clips_self 兜底但内收更稳
        ctx.fill_polygon({
            p(f, 0.00F, 0.78F), p(f, 0.10F, 0.62F),
            p(f, 0.50F, 0.58F), p(f, 0.90F, 0.62F),
            p(f, 1.00F, 0.78F), p(f, 1.00F, 0.88F),
            p(f, 0.00F, 0.88F)},
            jtui::Color::from_hex("#4A1820"));

        // 屋檐高光（暖橘细线沿屋脊）
        ctx.line(p(f, 0.10F, 0.62F), p(f, 0.50F, 0.58F), accent_, 1.5F);
        ctx.line(p(f, 0.50F, 0.58F), p(f, 0.90F, 0.62F), accent_, 1.5F);

        // 2 个红灯笼悬在屋檐前
        const jtui::Color lantern_body = jtui::Color::from_hex("#C92038");
        const jtui::Color lantern_glow = jtui::Color{1.0F, 0.45F, 0.20F, 0.55F};
        for (float nx : {0.28F, 0.66F}) {
            // 流苏（上吊绳）
            ctx.line(p(f, nx, 0.18F), p(f, nx, 0.30F),
                     jtui::Color::from_hex("#1A0608"), 1.0F);
            // 灯笼柔光圈
            ctx.fill_ellipse(
                r(f, nx - 0.115F, 0.27F, 0.23F, 0.30F),
                lantern_glow);
            // 灯笼主体
            ctx.fill_ellipse(
                r(f, nx - 0.085F, 0.30F, 0.17F, 0.24F),
                lantern_body);
            // 暖橘环（accent）
            ctx.stroke_ellipse(
                r(f, nx - 0.085F, 0.30F, 0.17F, 0.24F),
                accent_, 1.8F);
            // 灯笼底坠
            ctx.line(p(f, nx, 0.54F), p(f, nx, 0.60F), accent_, 1.2F);
        }

        // 角落 accent 装饰（微小三角）—— 远离右上 corner 圆角范围（避免在圆角内
        // 露半截）。原 (0.92,0.06) 在 radius=20 时进圆角区，现移到中部偏右上。
        ctx.fill_polygon({
            p(f, 0.84F, 0.13F), p(f, 0.92F, 0.13F), p(f, 0.88F, 0.20F)},
            accent_);
    }

    // ───────── Style 1: Mongolia —— 草原 + 蒙古包 + 暖日 ──────────
    void paint_mongolia(hui::PaintContext& ctx, const hui::RectF& f) const {
        // bg：黎明蓝紫 → 草原深绿
        ctx.fill_gradient_rect(f,
            jtui::Color::from_hex("#1A1F32"),
            jtui::Color::from_hex("#1F2A1A"),
            corner_radius_);

        // 暖日（大圆，右上 1/3 处）
        ctx.fill_ellipse(
            r(f, 0.62F, 0.10F, 0.22F, 0.36F),
            jtui::Color{0.98F, 0.58F, 0.24F, 0.55F});
        ctx.fill_ellipse(
            r(f, 0.66F, 0.14F, 0.14F, 0.24F),
            accent_);

        // 3 层远山（多边形山脊，越近越亮）
        // 远山
        ctx.fill_polygon({
            p(f, 0.00F, 0.55F), p(f, 0.18F, 0.40F),
            p(f, 0.32F, 0.48F), p(f, 0.55F, 0.38F),
            p(f, 0.78F, 0.45F), p(f, 1.00F, 0.40F),
            p(f, 1.00F, 0.60F), p(f, 0.00F, 0.60F)},
            jtui::Color::from_hex("#2A2E3A"));
        // 中山
        ctx.fill_polygon({
            p(f, 0.00F, 0.68F), p(f, 0.22F, 0.55F),
            p(f, 0.48F, 0.62F), p(f, 0.72F, 0.52F),
            p(f, 1.00F, 0.60F), p(f, 1.00F, 0.72F),
            p(f, 0.00F, 0.72F)},
            jtui::Color::from_hex("#1F2A22"));

        // 草原前景（深绿）
        ctx.fill_polygon({
            p(f, 0.00F, 0.72F), p(f, 1.00F, 0.72F),
            p(f, 1.00F, 1.00F), p(f, 0.00F, 1.00F)},
            jtui::Color::from_hex("#16201A"));

        // 2 个蒙古包剪影（白圆顶 + 矩形帐底）
        for (float nx : {0.30F, 0.55F}) {
            // 圆顶
            ctx.fill_ellipse(
                r(f, nx - 0.06F, 0.66F, 0.12F, 0.10F),
                jtui::Color::from_hex("#D8C8A8"));
            // 帐底矩形
            ctx.fill_rect(
                r(f, nx - 0.06F, 0.71F, 0.12F, 0.08F),
                jtui::Color::from_hex("#D8C8A8"));
            // 小门（暖橘）
            ctx.fill_rect(
                r(f, nx - 0.012F, 0.74F, 0.024F, 0.05F),
                accent_);
        }

        // 远处的一个小蒙古包
        ctx.fill_ellipse(
            r(f, 0.78F, 0.68F, 0.06F, 0.05F),
            jtui::Color::from_hex("#9A8E78"));
        ctx.fill_rect(
            r(f, 0.78F, 0.70F, 0.06F, 0.04F),
            jtui::Color::from_hex("#9A8E78"));
    }

    // ───────── Style 2: Vietnam —— 椰树叶剪影 ────────────────────
    void paint_vietnam(hui::PaintContext& ctx, const hui::RectF& f) const {
        // bg：青绿 → 暗绿
        ctx.fill_gradient_rect(f,
            jtui::Color::from_hex("#0F2A24"),
            jtui::Color::from_hex("#0A1812"),
            corner_radius_);

        // 远处水雾（横向半透白带）
        ctx.fill_rect(
            r(f, 0.00F, 0.55F, 1.00F, 0.06F),
            jtui::Color{1.0F, 1.0F, 1.0F, 0.04F});

        // 椰树叶剪影 —— 用 bezier 画 5 条叶脉从同一根部散开（左下角）
        const hui::PointF root = p(f, 0.18F, 0.78F);
        const jtui::Color leaf = jtui::Color::from_hex("#0E3D2A");
        const struct { hui::PointF c1, c2, end; } leaves[] = {
            {p(f, 0.05F, 0.55F),  p(f, 0.10F, 0.20F), p(f, 0.30F, 0.05F)},
            {p(f, 0.20F, 0.45F),  p(f, 0.35F, 0.15F), p(f, 0.55F, 0.10F)},
            {p(f, 0.40F, 0.50F),  p(f, 0.65F, 0.22F), p(f, 0.85F, 0.18F)},
            {p(f, 0.30F, 0.70F),  p(f, 0.55F, 0.55F), p(f, 0.85F, 0.42F)},
            {p(f, 0.10F, 0.85F),  p(f, 0.35F, 0.92F), p(f, 0.60F, 0.92F)},
        };
        for (const auto& lf : leaves) {
            ctx.draw_bezier(root, lf.c1, lf.c2, lf.end, leaf, 4.0F);
            // 叶肌（更深一层）
            ctx.draw_bezier(root, lf.c1, lf.c2, lf.end,
                            jtui::Color::from_hex("#072518"), 1.0F);
        }

        // 远处 3 个小红灯笼悬挂
        for (float nx : {0.55F, 0.72F, 0.88F}) {
            ctx.line(p(f, nx, 0.04F), p(f, nx, 0.12F),
                     jtui::Color::from_hex("#0A1812"), 1.0F);
            ctx.fill_ellipse(
                r(f, nx - 0.022F, 0.11F, 0.044F, 0.058F),
                jtui::Color::from_hex("#C92038"));
            ctx.stroke_ellipse(
                r(f, nx - 0.022F, 0.11F, 0.044F, 0.058F),
                accent_, 1.0F);
        }

        // 椰树干（垂直深线，根部上来）
        ctx.line(p(f, 0.18F, 0.78F), p(f, 0.20F, 1.00F),
                 jtui::Color::from_hex("#0A1812"), 6.0F);
    }

    // ───────── Style 3: Japan —— 富士山 + 红日 ────────────────────
    void paint_japan(hui::PaintContext& ctx, const hui::RectF& f) const {
        // bg：暗紫 → 暖琥珀
        ctx.fill_gradient_rect(f,
            jtui::Color::from_hex("#1A1A2A"),
            jtui::Color::from_hex("#2A1A1A"),
            corner_radius_);

        // 红日（右侧大圆）
        ctx.fill_ellipse(
            r(f, 0.58F, 0.15F, 0.30F, 0.48F),
            jtui::Color{0.98F, 0.32F, 0.18F, 0.30F});
        ctx.fill_ellipse(
            r(f, 0.63F, 0.20F, 0.20F, 0.32F),
            jtui::Color::from_hex("#D04030"));

        // 远云（水平细线）
        for (float ny : {0.32F, 0.38F, 0.44F}) {
            ctx.fill_rect(
                r(f, 0.10F + (ny - 0.32F) * 0.5F, ny,
                  0.40F - (ny - 0.32F) * 0.8F, 0.012F),
                jtui::Color{1.0F, 0.95F, 0.85F, 0.18F});
        }

        // 富士山主体（左侧大三角）—— 左下顶点 x 内收远离左下 corner 圆角
        ctx.fill_polygon({
            p(f, 0.08F, 1.00F), p(f, 0.45F, 0.40F),
            p(f, 0.85F, 1.00F)},
            jtui::Color::from_hex("#1F1F32"));

        // 富士山顶部白雪（小三角，盖在主山头）
        ctx.fill_polygon({
            p(f, 0.34F, 0.55F), p(f, 0.45F, 0.40F),
            p(f, 0.56F, 0.55F), p(f, 0.52F, 0.58F),
            p(f, 0.48F, 0.55F), p(f, 0.43F, 0.60F),
            p(f, 0.38F, 0.57F)},
            jtui::Color::from_hex("#E8E0E8"));

        // 富士山轮廓 accent 一条（山脊）
        ctx.line(p(f, 0.45F, 0.40F), p(f, 0.85F, 1.00F), accent_, 1.0F);

        // 远处小山
        ctx.fill_polygon({
            p(f, 0.60F, 1.00F), p(f, 0.78F, 0.78F),
            p(f, 0.95F, 1.00F)},
            jtui::Color::from_hex("#161628"));
    }

    // ───────── Style 4: Taiwan —— 夜市招牌 + 灯笼 ────────────────
    void paint_taiwan(hui::PaintContext& ctx, const hui::RectF& f) const {
        // bg：深紫红 → 黑
        ctx.fill_gradient_rect(f,
            jtui::Color::from_hex("#1F0A16"),
            jtui::Color::from_hex("#0A0408"),
            corner_radius_);

        // 招牌堆叠：4 个不同宽窄、不同高低的矩形
        struct Sign {
            float nx, ny, nw, nh;
            jtui::Color fill;
            bool warm_glow;
        };
        const Sign signs[] = {
            {0.06F, 0.30F, 0.20F, 0.10F, jtui::Color::from_hex("#3A1820"), true},
            {0.30F, 0.42F, 0.16F, 0.08F, jtui::Color::from_hex("#2A1218"), false},
            {0.50F, 0.34F, 0.24F, 0.12F, jtui::Color::from_hex("#3A1820"), true},
            {0.78F, 0.40F, 0.16F, 0.10F, jtui::Color::from_hex("#2A1218"), false},
        };
        for (const auto& s : signs) {
            ctx.fill_rect(r(f, s.nx, s.ny, s.nw, s.nh), s.fill);
            if (s.warm_glow) {
                // 招牌底部一条暖橘灯条
                ctx.fill_rect(
                    r(f, s.nx, s.ny + s.nh - 0.012F, s.nw, 0.014F),
                    accent_);
            }
            // 招牌竖向支柱（细灰线）
            ctx.line(
                p(f, s.nx + s.nw * 0.5F, s.ny + s.nh),
                p(f, s.nx + s.nw * 0.5F, 1.0F),
                jtui::Color::from_hex("#1F0F14"), 2.0F);
        }

        // 灯笼串：上方横挂的 6 个小红圆
        ctx.line(
            p(f, 0.00F, 0.16F), p(f, 1.00F, 0.18F),
            jtui::Color::from_hex("#1F0F14"), 1.0F);
        const float dots_nx[] = {0.10F, 0.25F, 0.40F, 0.55F, 0.70F, 0.85F};
        int idx = 0;
        for (float nx : dots_nx) {
            const float bob = (idx % 2 == 0) ? 0.0F : 0.012F;
            ctx.fill_ellipse(
                r(f, nx - 0.03F, 0.17F + bob, 0.06F, 0.07F),
                jtui::Color::from_hex("#C92038"));
            ctx.stroke_ellipse(
                r(f, nx - 0.03F, 0.17F + bob, 0.06F, 0.07F),
                accent_, 1.0F);
            ++idx;
        }

        // 远处地面反光（横向暖橘极淡渐变带）
        ctx.fill_rect(
            r(f, 0.00F, 0.92F, 1.00F, 0.04F),
            jtui::Color{0.98F, 0.58F, 0.24F, 0.10F});
    }

    int          style_id_ {0};
    jtui::Color  accent_ {jtui::Color::from_hex("#FB923C")};
    float        corner_radius_ {0.0F};
};

}  // namespace jtui_cinema
