#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "hui/object/widget.hpp"
#include "hui/property/realtime_source.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"
#include "hui/theme/token_override.hpp"

namespace hui {

// LevelMeter：spec 差异化组件四件套之一。
//
// 单通道音量电平表，输入域 [0, 1]。绿/黄/红三段着色（分段边界 0.7 / 0.9）。
// 可选 peak hold：保留近期峰值并按指数衰减。
//
// 数据接入两路：
//   1) 主动 set_value(v)
//   2) bind_source(RealtimeSource<float>*)：tick 检测 generation 变化把
//      source.latest() pull 进来 —— 与音频 producer（WASAPI 回调线程算 RMS
//      后 publish 的）对接。绑了之后 tick 持续 active。
//
// 多通道：v1 一个 widget 对应一个通道。立体声场景由调用方放两个 LevelMeter
// 在 FlexBox 里并排。后续要 stereo 单 widget 再扩。

class LevelMeter : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "LevelMeter";
    }

    void set_value(float v) noexcept {
        const float clamped = std::clamp(v, 0.0F, 1.0F);
        if (value_ == clamped) return;
        value_ = clamped;
        if (peak_hold_enabled_ && value_ > peak_) {
            peak_ = value_;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] float value() const noexcept { return value_; }

    [[nodiscard]] float peak() const noexcept { return peak_; }

    void set_peak_hold(bool enabled) noexcept {
        if (peak_hold_enabled_ == enabled) return;
        peak_hold_enabled_ = enabled;
        if (!enabled) {
            peak_ = value_;
        }
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool peak_hold_enabled() const noexcept { return peak_hold_enabled_; }

    void set_orientation_horizontal(bool horizontal) noexcept {
        if (horizontal_ == horizontal) return;
        horizontal_ = horizontal;
        mark_dirty(DirtyFlags::Paint);
    }

    void bind_source(RealtimeSource<float>* source) {
        if (source == nullptr) {
            probe_ = nullptr;
            pull_ = nullptr;
        } else {
            probe_ = [source]() noexcept { return source->generation(); };
            pull_ = [source]() { return source->latest(); };
        }
        last_observed_generation_ = 0;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool is_source_bound() const noexcept {
        return static_cast<bool>(probe_);
    }

    // 组件级 token override（spec §13.4）。LevelMeter 典型场景：把 success 段
    // 改成项目品牌色而非默认 theme accent，或把 danger 段改成更醒目的色。
    void set_token_override(theme::TokenOverride override) {
        token_override_ = std::make_unique<theme::TokenOverride>(std::move(override));
        mark_dirty(DirtyFlags::Paint);
    }
    void clear_token_override() {
        if (!token_override_) return;
        token_override_.reset();
        mark_dirty(DirtyFlags::Paint);
    }
    [[nodiscard]] const theme::TokenOverride* token_override() const noexcept {
        return token_override_.get();
    }

    bool tick(float delta) override {
        // 拉新值
        if (probe_) {
            const auto g = probe_();
            if (g != last_observed_generation_) {
                last_observed_generation_ = g;
                if (pull_) {
                    const auto v = pull_();
                    if (v.has_value()) {
                        set_value(*v);
                    }
                }
            }
        }
        // peak 衰减（每秒掉一段，让 peak hold 不会卡死在历史最高）
        if (peak_hold_enabled_ && peak_ > value_) {
            constexpr float decay_per_sec = 0.45F;
            const float new_peak = peak_ - delta * decay_per_sec;
            const float clamped = std::max(new_peak, value_);
            if (clamped != peak_) {
                peak_ = clamped;
                mark_dirty(DirtyFlags::Paint);
            }
        }
        // 绑了 source 就保持 timer active；没绑但开了 peak hold 也保持（让衰减跑）
        return is_source_bound() || peak_hold_enabled_;
    }

    void paint(PaintContext& context) const override {
        const auto c = theme::resolve(token_override_.get());
        const float r = theme::radius().sm;

        context.fill_rounded_rect(frame(), c.track, r);

        // 三段阈值
        constexpr float warn_threshold = 0.70F;
        constexpr float danger_threshold = 0.90F;

        auto color_at = [&](float v) -> Color {
            if (v >= danger_threshold) return c.danger;
            if (v >= warn_threshold) return c.warning;
            return c.success;
        };

        if (horizontal_) {
            // 水平：从左到右填充
            const float full_w = frame().width;
            const float fill_w = full_w * value_;
            // 分段填，让颜色随位置过渡
            float x = frame().x;
            const float warn_x = frame().x + full_w * warn_threshold;
            const float danger_x = frame().x + full_w * danger_threshold;
            const float fill_end = frame().x + fill_w;

            auto draw_seg = [&](float seg_left, float seg_right, Color color) {
                const float left = std::max(seg_left, x);
                const float right = std::min(seg_right, fill_end);
                if (right > left) {
                    context.fill_rounded_rect(
                        RectF{left, frame().y, right - left, frame().height}, color, r);
                }
            };
            draw_seg(frame().x, warn_x, c.success);
            draw_seg(warn_x, danger_x, c.warning);
            draw_seg(danger_x, frame().x + full_w, c.danger);

            if (peak_hold_enabled_ && peak_ > 0.0F) {
                const float px = frame().x + full_w * peak_;
                context.line(PointF{px, frame().y + 1.0F},
                             PointF{px, frame().y + frame().height - 1.0F},
                             color_at(peak_), 2.0F);
            }
        } else {
            // 竖直：从底向上填充（典型音频电平表的样子）
            const float full_h = frame().height;
            const float fill_h = full_h * value_;
            const float bottom = frame().y + full_h;
            const float fill_top = bottom - fill_h;
            // 段定义（屏幕 y 坐标，y 小在上）：
            //   danger 段：top 10%       — y ∈ [frame.y, bottom - full_h*0.9]
            //   warning 段：next 20%     — y ∈ [bottom - full_h*0.9, bottom - full_h*0.7]
            //   success 段：bottom 70%   — y ∈ [bottom - full_h*0.7, bottom]
            const float danger_seg_bot = bottom - full_h * danger_threshold;
            const float warn_seg_bot = bottom - full_h * warn_threshold;

            auto draw_seg = [&](float seg_top, float seg_bottom, Color color) {
                const float top = std::max(seg_top, fill_top);
                const float bot = std::min(seg_bottom, bottom);
                if (bot > top) {
                    context.fill_rounded_rect(
                        RectF{frame().x, top, frame().width, bot - top}, color, r);
                }
            };
            draw_seg(frame().y, danger_seg_bot, c.danger);
            draw_seg(danger_seg_bot, warn_seg_bot, c.warning);
            draw_seg(warn_seg_bot, bottom, c.success);

            if (peak_hold_enabled_ && peak_ > 0.0F) {
                const float py = bottom - full_h * peak_;
                context.line(PointF{frame().x + 1.0F, py},
                             PointF{frame().x + frame().width - 1.0F, py},
                             color_at(peak_), 2.0F);
            }
        }
    }

private:
    float value_{0.0F};
    float peak_{0.0F};
    bool peak_hold_enabled_{false};
    bool horizontal_{false};
    std::function<std::uint64_t()> probe_{};
    std::function<std::optional<float>()> pull_{};
    std::uint64_t last_observed_generation_{0};
    std::unique_ptr<theme::TokenOverride> token_override_{};
};

}  // namespace hui
