#pragma once

// jtui_pulse 矢量绘制的 "live chart" 装饰背景。
// 完全用 PaintContext 的 line / fill_rect / fill_ellipse 直接画，**不走 PixelBuffer**。
// D2D 矢量渲染走 AA，曲线锐利不失真；前一版用 320×200 PixelBuffer 拉伸到 ~570×400
// 的 chart 区域产生了明显的颗粒锯齿（用户反馈"曲线失真"）。
//
// 视觉：
//   - 8×5 网格线（极淡）
//   - 折线下方阴影区：80 段 1px 宽的 fill_rect 拼接（D2D 每段 AA 边）
//   - 折线主体：80 段 line 连接（2px 粗）
//   - 6 个数据点圆点（fill_ellipse）
//   - 折线 chart_y(xn) 由多频 sin 叠加 + 上升 trend 模拟监控数据
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <cmath>
#include <string>

#include "jtui/jtui.hpp"

namespace jtui_pulse {

class LiveChartBg : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "LiveChartBg";
    }

    // v1.20：业务零 push_clip，runtime 自动 clip
    [[nodiscard]] bool clips_self() const noexcept override { return true; }

    void set_palette(jtui::Color base, jtui::Color line, jtui::Color fill,
                     jtui::Color grid, bool /*is_dark*/) noexcept {
        base_ = base;
        line_ = line;
        fill_ = fill;
        grid_ = grid;
        mark_dirty(hui::DirtyFlags::Paint);
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 1) 卡片底色（已经被父 Panel 画了，这里跳过省一次 fill）

        // 2) 8 列 × 5 行网格线（极淡）
        constexpr int kCols = 8;
        constexpr int kRows = 5;
        for (int c = 1; c < kCols; ++c) {
            const float x = f.x + f.width * static_cast<float>(c) / static_cast<float>(kCols);
            context.line({x, f.y}, {x, f.y + f.height}, grid_, 1.0F);
        }
        for (int r = 1; r < kRows; ++r) {
            const float y = f.y + f.height * static_cast<float>(r) / static_cast<float>(kRows);
            context.line({f.x, y}, {f.x + f.width, y}, grid_, 1.0F);
        }

        // 3) 折线 + 阴影区：80 段，每段算 (xn, yn) → (px, py)
        constexpr int kSegments = 80;
        const float seg_w = f.width / static_cast<float>(kSegments);

        // 先算所有点
        struct Pt { float x, y; };
        std::vector<Pt> pts;
        pts.reserve(kSegments + 1);
        for (int i = 0; i <= kSegments; ++i) {
            const float xn = static_cast<float>(i) / static_cast<float>(kSegments);
            const float yn = chart_y_(xn);
            pts.push_back({
                f.x + f.width * xn,
                f.y + f.height * (1.0F - yn),
            });
        }

        // 阴影区：每段一个 vertical fill_rect 从折线 y 到 chart 底
        for (int i = 0; i < kSegments; ++i) {
            // 用左右 y 的均值作这段的 y
            const float top_y = (pts[i].y + pts[i + 1].y) * 0.5F;
            const float strip_h = f.y + f.height - top_y;
            if (strip_h <= 0.0F) continue;
            context.fill_rect(
                hui::RectF{pts[i].x, top_y, seg_w + 0.5F, strip_h},
                fill_);
        }

        // 折线主体：80 段 line
        for (int i = 0; i < kSegments; ++i) {
            context.line({pts[i].x, pts[i].y},
                         {pts[i + 1].x, pts[i + 1].y},
                         line_, 2.0F);
        }

        // 4) 6 个数据点圆点（每个 11px 大圆做 halo + 6px 实心）
        constexpr int kDots = 6;
        constexpr float kDotR = 5.0F;
        constexpr float kHaloR = 9.0F;
        for (int i = 0; i < kDots; ++i) {
            const float xn = static_cast<float>(i + 1) / static_cast<float>(kDots + 1);
            const float yn = chart_y_(xn);
            const float cx = f.x + f.width * xn;
            const float cy = f.y + f.height * (1.0F - yn);
            // halo
            context.fill_ellipse(
                hui::RectF{cx - kHaloR, cy - kHaloR, kHaloR * 2.0F, kHaloR * 2.0F},
                jtui::Color{line_.r, line_.g, line_.b, 0.20F});
            // 实心
            context.fill_ellipse(
                hui::RectF{cx - kDotR, cy - kDotR, kDotR * 2.0F, kDotR * 2.0F},
                line_);
        }
    }

private:
    // 折线 y 值（[0..1] 输入 → [0..1] 输出，0 = 底 1 = 顶）
    static float chart_y_(float x) noexcept {
        const float pi = 3.14159265F;
        float y = 0.45F;
        y += 0.15F * std::sin(x * 4.7F * pi);
        y += 0.10F * std::sin(x * 9.3F * pi + 1.3F);
        y += 0.06F * std::sin(x * 17.1F * pi + 0.7F);
        y += 0.20F * x;  // 整体上升 trend
        return std::clamp(y, 0.05F, 0.95F);
    }

    jtui::Color base_ {};
    jtui::Color line_ {};
    jtui::Color fill_ {};
    jtui::Color grid_ {};
};

}  // namespace jtui_pulse
