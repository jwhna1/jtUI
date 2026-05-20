#pragma once

// jtui_invest 动画 widget —— 三个 example 内自定义 widget：
//   - CountUpText      数字从 0 count-up 到目标值（ease-out）
//   - AnimatedLineChart 折线图 draw-in（曲线 + 辉光面积逐段展开）
//   - BarChart          季度柱状图，柱子高度 grow-in 动画
//
// 全部走框架 Widget::tick(delta) 机制 —— 窗口创建即启动 animation timer，
// tick 返回 true 就持续被调。动画完成时打一条 "anim" tag 的 log 便于定位。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"
#include "hui/foundation/log.hpp"

namespace jtui_invest {

// ease-out cubic：1-(1-t)^3，先快后慢，收尾柔和。
[[nodiscard]] inline float ease_out_cubic(float t) noexcept {
    const float inv = 1.0F - std::clamp(t, 0.0F, 1.0F);
    return 1.0F - inv * inv * inv;
}

// 金额格式化：整数部分千分位，可选前缀（"$" / "+$"）+ 指定小数位。
[[nodiscard]] inline std::string format_amount(double value, int decimals,
                                               const std::string& prefix) {
    const bool neg = value < 0.0;
    const double av = neg ? -value : value;
    const long long int_part = static_cast<long long>(av);
    const double frac = av - static_cast<double>(int_part);

    const std::string digits = std::to_string(int_part);
    std::string grouped;
    int cnt = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        grouped.push_back(digits[static_cast<std::size_t>(i)]);
        if (++cnt % 3 == 0 && i > 0) grouped.push_back(',');
    }
    std::reverse(grouped.begin(), grouped.end());

    std::string out;
    if (neg) out += '-';
    out += prefix;
    out += grouped;
    if (decimals > 0) {
        long long scale = 1;
        for (int i = 0; i < decimals; ++i) scale *= 10;
        long long frac_int =
            static_cast<long long>(frac * static_cast<double>(scale) + 0.5);
        if (frac_int >= scale) frac_int = scale - 1;
        std::string fs = std::to_string(frac_int);
        while (static_cast<int>(fs.size()) < decimals) {
            fs.insert(fs.begin(), '0');
        }
        out += '.';
        out += fs;
    }
    return out;
}

// ───────── CountUpText：数字 count-up ───────────────────────────────

class CountUpText : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "CountUpText";
    }

    void configure(double target, int decimals, std::string prefix,
                   float font_size, bool bold, jtui::Color color) {
        target_ = target;
        decimals_ = decimals;
        prefix_ = std::move(prefix);
        font_size_ = font_size;
        bold_ = bold;
        color_ = color;
    }
    void set_alignment(jtui::TextAlignment a) noexcept { align_ = a; }
    void set_duration(float seconds) noexcept {
        duration_ = seconds > 0.05F ? seconds : 0.05F;
    }
    // 直接跳到完成态。切主题 / i18n 整树 rebuild 后调用，让动画不重播
    // （首次 build 才播放，rebuild 直接显示终值）。
    void finish_now() noexcept { progress_ = 1.0F; }

    bool tick(float delta) override {
        if (progress_ >= 1.0F) return false;
        progress_ += delta / duration_;
        if (progress_ >= 1.0F) {
            progress_ = 1.0F;
            HUI_LOG_D("anim", "CountUpText done target=" +
                              format_amount(target_, decimals_, prefix_));
        }
        mark_dirty(hui::DirtyFlags::Paint);
        return progress_ < 1.0F;
    }

    void paint(hui::PaintContext& context) const override {
        const float eased = ease_out_cubic(progress_);
        const double current = target_ * static_cast<double>(eased);
        context.draw_text(frame(),
                          format_amount(current, decimals_, prefix_),
                          color_, align_, font_size_, bold_);
    }

private:
    double target_ {0.0};
    int decimals_ {0};
    std::string prefix_ {};
    float font_size_ {24.0F};
    bool bold_ {true};
    jtui::Color color_ {};
    jtui::TextAlignment align_ {jtui::TextAlignment::Leading};
    float progress_ {0.0F};
    float duration_ {1.2F};
};

// ───────── AnimatedLineChart：折线 draw-in ──────────────────────────

class AnimatedLineChart : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "AnimatedLineChart";
    }

    void set_data(std::vector<float> d) { data_ = std::move(d); }
    void set_palette(jtui::Color line, jtui::Color glow, jtui::Color grid) {
        line_ = line;
        glow_ = glow;
        grid_ = grid;
    }
    void set_duration(float seconds) noexcept {
        duration_ = seconds > 0.05F ? seconds : 0.05F;
    }
    // 直接跳到完成态。切主题 / i18n 整树 rebuild 后调用，让动画不重播
    // （首次 build 才播放，rebuild 直接显示终值）。
    void finish_now() noexcept { progress_ = 1.0F; }

    bool tick(float delta) override {
        if (progress_ >= 1.0F) return false;
        progress_ += delta / duration_;
        if (progress_ >= 1.0F) {
            progress_ = 1.0F;
            HUI_LOG_D("anim", "AnimatedLineChart draw-in done");
        }
        mark_dirty(hui::DirtyFlags::Paint);
        return progress_ < 1.0F;
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F || data_.size() < 2) return;

        // 横向网格线 5 条
        for (int i = 0; i <= 4; ++i) {
            const float y = f.y + f.height * static_cast<float>(i) / 4.0F;
            context.line(hui::PointF{f.x, y},
                         hui::PointF{f.x + f.width, y}, grid_, 1.0F);
        }

        // 数据范围
        float vmin = data_[0], vmax = data_[0];
        for (const float v : data_) {
            vmin = std::min(vmin, v);
            vmax = std::max(vmax, v);
        }
        const float vrange = (vmax - vmin) > 0.001F ? (vmax - vmin) : 1.0F;

        // 全部数据点坐标
        const std::size_t n = data_.size();
        const float pad = 10.0F;
        const float ch = f.height - 2.0F * pad;
        std::vector<hui::PointF> pts;
        pts.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(n - 1);
            const float norm = (data_[i] - vmin) / vrange;
            pts.push_back(hui::PointF{
                f.x + t * f.width, f.y + pad + (1.0F - norm) * ch});
        }

        // draw-in：按 eased progress 截取可见段
        const float eased = ease_out_cubic(progress_);
        const float draw_n = eased * static_cast<float>(n - 1);
        const std::size_t full = static_cast<std::size_t>(draw_n);
        const float partial = draw_n - static_cast<float>(full);

        std::vector<hui::PointF> vis;
        for (std::size_t i = 0; i <= full && i < n; ++i) {
            vis.push_back(pts[i]);
        }
        hui::PointF tip = vis.empty() ? pts[0] : vis.back();
        if (full + 1 < n && partial > 0.0F) {
            const hui::PointF a = pts[full];
            const hui::PointF b = pts[full + 1];
            tip = hui::PointF{a.x + (b.x - a.x) * partial,
                              a.y + (b.y - a.y) * partial};
            vis.push_back(tip);
        }
        if (vis.size() < 2) return;

        // 辉光面积（折线下方填充到底）
        std::vector<hui::PointF> poly = vis;
        poly.push_back(hui::PointF{tip.x, f.y + f.height - pad});
        poly.push_back(hui::PointF{f.x,   f.y + f.height - pad});
        context.fill_polygon(std::move(poly), glow_);

        // 折线
        for (std::size_t i = 0; i + 1 < vis.size(); ++i) {
            context.line(vis[i], vis[i + 1], line_, 2.5F);
        }

        // 末点高亮 dot
        context.fill_ellipse(
            hui::RectF{tip.x - 5.0F, tip.y - 5.0F, 10.0F, 10.0F}, line_);
    }

private:
    std::vector<float> data_ {};
    jtui::Color line_ {};
    jtui::Color glow_ {};
    jtui::Color grid_ {};
    float progress_ {0.0F};
    float duration_ {1.6F};
};

// ───────── BarChart：季度柱状图 grow-in ─────────────────────────────

struct BarSpec {
    std::string label;   // Q1 / Q2 ...
    std::string pct;     // +15.28% / -10.89%
    float       value;   // 用于柱高（带正负号）
    bool        positive;
};

class BarChart : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "BarChart";
    }

    void set_bars(std::vector<BarSpec> bars) { bars_ = std::move(bars); }
    void set_palette(jtui::Color up, jtui::Color up_soft, jtui::Color down,
                     jtui::Color down_soft, jtui::Color label,
                     jtui::Color axis) {
        up_ = up;
        up_soft_ = up_soft;
        down_ = down;
        down_soft_ = down_soft;
        label_ = label;
        axis_ = axis;
    }
    void set_duration(float seconds) noexcept {
        duration_ = seconds > 0.05F ? seconds : 0.05F;
    }
    // 直接跳到完成态。切主题 / i18n 整树 rebuild 后调用，让动画不重播
    // （首次 build 才播放，rebuild 直接显示终值）。
    void finish_now() noexcept { progress_ = 1.0F; }

    bool tick(float delta) override {
        if (progress_ >= 1.0F) return false;
        progress_ += delta / duration_;
        if (progress_ >= 1.0F) {
            progress_ = 1.0F;
            HUI_LOG_D("anim", "BarChart grow-in done");
        }
        mark_dirty(hui::DirtyFlags::Paint);
        return progress_ < 1.0F;
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F || bars_.empty()) return;

        // 最大绝对值用于归一化柱高
        float max_abs = 0.0F;
        for (const auto& b : bars_) {
            max_abs = std::max(max_abs, std::abs(b.value));
        }
        if (max_abs <= 0.001F) max_abs = 1.0F;

        // baseline 偏下 68%：正值柱往上长（占 ~58%），负值往下（占 ~26%）
        const float label_h = 22.0F;          // 底部 Q 标签行高
        const float plot_h = f.height - label_h;
        const float base_y = f.y + plot_h * 0.68F;
        const float up_room = base_y - f.y - 18.0F;        // 正柱最大高（留顶部标签位）
        const float down_room = f.y + plot_h - base_y - 4.0F;

        // baseline
        context.line(hui::PointF{f.x, base_y},
                     hui::PointF{f.x + f.width, base_y}, axis_, 1.0F);

        const float eased = ease_out_cubic(progress_);
        const std::size_t n = bars_.size();
        const float slot_w = f.width / static_cast<float>(n);
        const float bar_w = std::min(slot_w * 0.56F, 64.0F);

        for (std::size_t i = 0; i < n; ++i) {
            const BarSpec& b = bars_[i];
            const float cx = f.x + slot_w * (static_cast<float>(i) + 0.5F);
            const float room = b.positive ? up_room : down_room;
            const float full_h = (std::abs(b.value) / max_abs) * room;
            const float h = full_h * eased;

            hui::RectF bar_rect;
            if (b.positive) {
                bar_rect = hui::RectF{cx - bar_w * 0.5F, base_y - h, bar_w, h};
            } else {
                bar_rect = hui::RectF{cx - bar_w * 0.5F, base_y, bar_w, h};
            }
            context.fill_rounded_rect(bar_rect,
                                      b.positive ? up_soft_ : down_soft_, 6.0F);
            // 实心描边色条（细一点的内层，给柱子"实感"）
            const float inner_inset = 0.0F;
            (void)inner_inset;
            // 百分比标签：正柱标在柱顶上方，负柱标在柱底下方
            const float lbl_y = b.positive ? (base_y - full_h - 17.0F)
                                           : (base_y + down_room + 1.0F);
            context.draw_text(
                hui::RectF{cx - slot_w * 0.5F, lbl_y, slot_w, 15.0F},
                b.pct, b.positive ? up_ : down_,
                jtui::TextAlignment::Center, 11.0F, true);
        }

        // 底部 Q 标签
        for (std::size_t i = 0; i < n; ++i) {
            const float cx = f.x + slot_w * (static_cast<float>(i) + 0.5F);
            context.draw_text(
                hui::RectF{cx - slot_w * 0.5F, f.y + plot_h + 2.0F,
                           slot_w, label_h},
                bars_[i].label, label_,
                jtui::TextAlignment::Center, 12.0F, false);
        }
    }

private:
    std::vector<BarSpec> bars_ {};
    jtui::Color up_ {}, up_soft_ {}, down_ {}, down_soft_ {};
    jtui::Color label_ {}, axis_ {};
    float progress_ {0.0F};
    float duration_ {1.0F};
};

}  // namespace jtui_invest
