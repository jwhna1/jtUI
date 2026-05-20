#pragma once

// jtui_pro 程序化合成背景 widget。
// 用 PixelBuffer 手填 BGRA8 像素，再交给 PaintContext::draw_texture 拉伸到全屏。
// 单张 256×160 缩略图，运行时 D2D 双线性采样拉到任意 size，避免 1280×800 一帧
// 4MB 的内存压力，同时保持柔和过渡（径向高光天然适合低分辨率合成）。
//
// 视觉：
//   - 深色基底 + 右上角水青径向高光（高斯衰减）
//   - 多个光斑（不同位置 / 大小 / alpha 的小高斯）
//   - 一道斜向条纹柔光
//   - dark / light 各生成一份，按 theme 切换
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui_pro {

class GeneratedBgWidget : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "GeneratedBgWidget";
    }

    // accent / base 由业务从 brand_tokens 注入，自动适配 dark/light
    void set_palette(jtui::Color base, jtui::Color accent, bool is_dark) noexcept {
        base_ = base;
        accent_ = accent;
        is_dark_ = is_dark;
        // 调色变化让缓存 key 失效
        cache_key_ = std::string("v1:") + (is_dark ? "dark" : "light")
                   + ":" + std::to_string(int(base.r * 255))
                   + ":" + std::to_string(int(base.g * 255))
                   + ":" + std::to_string(int(base.b * 255))
                   + ":" + std::to_string(int(accent.r * 255))
                   + ":" + std::to_string(int(accent.g * 255))
                   + ":" + std::to_string(int(accent.b * 255));
        mark_dirty(hui::DirtyFlags::Paint);
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const auto& buf = cached_buffer_();
        if (!buf.empty()) {
            context.draw_texture(f, buf);
        } else {
            // fallback：直接画基底色
            context.fill_rect(f, base_);
        }
    }

private:
    // 进程级缓存：key = "v1:dark:0:0:0:..." 之类，避免相同配色重复合成
    static std::map<std::string, jtui::PixelBuffer>& cache_() {
        static std::map<std::string, jtui::PixelBuffer> m;
        return m;
    }

    [[nodiscard]] const jtui::PixelBuffer& cached_buffer_() const {
        auto& c = cache_();
        auto it = c.find(cache_key_);
        if (it != c.end()) return it->second;

        const auto buf = generate_();
        auto [ins, _] = c.emplace(cache_key_, buf);
        return ins->second;
    }

    [[nodiscard]] jtui::PixelBuffer generate_() const {
        // 256×160 缩略 —— D2D 双线性采样到任意 frame size 都柔和
        constexpr int kW = 256;
        constexpr int kH = 160;
        auto buf = jtui::PixelBuffer::allocate(kW, kH, jtui::PixelFormat::BGRA8);
        buf.alpha_mode = jtui::AlphaMode::Premultiplied;
        auto* dst = buf.data->data();

        // 几个 hot spot：(x_norm, y_norm, sigma_x_norm, sigma_y_norm, intensity)
        struct Spot {
            float cx, cy, sx, sy, k;
        };
        const std::vector<Spot> spots = {
            { 0.85F, 0.20F, 0.35F, 0.35F, 1.00F },  // 主光：右上大椭圆
            { 0.70F, 0.05F, 0.20F, 0.10F, 0.55F },  // 顶部细光
            { 0.92F, 0.55F, 0.18F, 0.30F, 0.45F },  // 右中柔光
            { 0.20F, 0.85F, 0.30F, 0.20F, 0.30F },  // 左下点缀
            { 0.50F, 0.40F, 0.45F, 0.30F, 0.18F },  // 中央铺底（很淡）
        };

        // 一道斜向条纹（用 perpendicular distance 算 + 高斯）
        // 直线方程：a*x + b*y + c = 0；归一化 (a,b) 让距离直观
        // 取从右上 (1.0, 0.2) 到左下 (0.0, 0.95)：法向 (0.75, 1.0) 归一
        const float la = 0.75F, lb = 1.0F;
        const float linv = 1.0F / std::sqrt(la * la + lb * lb);
        const float lc = -(la * 0.7F + lb * 0.4F);  // 让线穿 (0.7, 0.4)
        const float line_sigma = 0.04F;
        const float line_k = 0.35F;

        for (int y = 0; y < kH; ++y) {
            const float yn = static_cast<float>(y) / static_cast<float>(kH);
            for (int x = 0; x < kW; ++x) {
                const float xn = static_cast<float>(x) / static_cast<float>(kW);

                // 1) 累加所有光斑高斯
                float lum = 0.0F;
                for (const auto& s : spots) {
                    const float dx = (xn - s.cx) / s.sx;
                    const float dy = (yn - s.cy) / s.sy;
                    lum += s.k * std::exp(-(dx * dx + dy * dy));
                }
                // 2) 斜向条纹高斯
                const float lineDist = (la * xn + lb * yn + lc) * linv;
                lum += line_k * std::exp(-(lineDist * lineDist) / (2.0F * line_sigma * line_sigma));

                // 3) 上限 clamp
                lum = std::min(lum, 1.0F);

                // 4) base + accent * lum
                float r = base_.r + (accent_.r - base_.r) * lum;
                float g = base_.g + (accent_.g - base_.g) * lum;
                float b = base_.b + (accent_.b - base_.b) * lum;
                r = std::clamp(r, 0.0F, 1.0F);
                g = std::clamp(g, 0.0F, 1.0F);
                b = std::clamp(b, 0.0F, 1.0F);

                // BGRA8 + premultiplied alpha = 1 时 RGB 就是 RGB
                const std::size_t idx = (static_cast<std::size_t>(y) * kW
                                       + static_cast<std::size_t>(x)) * 4U;
                dst[idx + 0] = static_cast<std::uint8_t>(b * 255.0F);
                dst[idx + 1] = static_cast<std::uint8_t>(g * 255.0F);
                dst[idx + 2] = static_cast<std::uint8_t>(r * 255.0F);
                dst[idx + 3] = 255;
            }
        }
        return buf;
    }

    jtui::Color base_ {};
    jtui::Color accent_ {};
    bool is_dark_ {true};
    std::string cache_key_ {};
};

}  // namespace jtui_pro
