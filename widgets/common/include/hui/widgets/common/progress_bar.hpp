#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

enum class ProgressBarMode {
    Determinate,    // 显示具体进度 value / [0,1]
    Indeterminate,  // 不定长，扫描动画
};

// 水平进度条。高度默认 8px（和设计图里彩色 bar 一致）。
// indeterminate 模式下 tick 推 sweep_offset_ 让一段 "扫光" 来回走。
class ProgressBar : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "ProgressBar";
    }

    [[nodiscard]] float value() const noexcept { return value_; }

    void set_value(float value) noexcept {
        const float clamped = std::clamp(value, 0.0F, 1.0F);
        if (clamped == value_) return;
        value_ = clamped;
        changed_.emit(value_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_mode(ProgressBarMode mode) noexcept {
        if (mode_ == mode) return;
        mode_ = mode;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) return;
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    // 轨道高度（默认 8 px）
    void set_thickness(float px) noexcept {
        thickness_ = std::max(2.0F, px);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_label(std::string label) {
        label_ = std::move(label);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<float>& on_changed() noexcept { return changed_; }

    bool tick(float delta) override {
        if (mode_ != ProgressBarMode::Indeterminate) return false;
        // 扫光每秒移动 1.2 个周期；phase_ 归一化在 [0, 2)
        phase_ += delta * 1.2F;
        if (phase_ >= 2.0F) phase_ -= 2.0F;
        mark_dirty(DirtyFlags::Paint);
        return true;
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const Color fill = theme::color_for(tone_);

        const float label_h = label_.empty() ? 0.0F : 18.0F;
        if (!label_.empty()) {
            context.draw_text(RectF{frame().x, frame().y, frame().width, label_h},
                              label_, c.text_secondary, TextAlignment::Leading, 12.0F, true);
        }

        const float track_y = frame().y + label_h + (frame().height - label_h - thickness_) * 0.5F;
        const RectF track{frame().x, track_y, frame().width, thickness_};
        context.fill_rounded_rect(track, c.track, thickness_ * 0.5F);

        if (mode_ == ProgressBarMode::Determinate) {
            const RectF filled{track.x, track.y, track.width * value_, track.height};
            context.fill_rounded_rect(filled, fill, thickness_ * 0.5F);
            return;
        }

        // Indeterminate：一段 30% 宽度的光条来回扫。phase 在 [0, 2)
        // 映射到位置 [-segment_w, track.width]，产生左右往复的感觉。
        const float seg_w = track.width * 0.3F;
        const float pos_range = track.width + seg_w;
        const float t = phase_ < 1.0F ? phase_ : (2.0F - phase_);
        const float x = -seg_w + pos_range * t;
        const float clipped_x = std::max(track.x, track.x + x);
        const float clipped_right = std::min(track.x + track.width, track.x + x + seg_w);
        const float w = std::max(0.0F, clipped_right - clipped_x);
        if (w > 0.0F) {
            context.fill_rounded_rect(RectF{clipped_x, track.y, w, track.height},
                                      fill, thickness_ * 0.5F);
        }
    }

private:
    float value_ {0.0F};
    float thickness_ {8.0F};
    float phase_ {0.0F};
    ProgressBarMode mode_ {ProgressBarMode::Determinate};
    theme::Tone tone_ {theme::Tone::Accent};
    std::string label_ {};
    Signal<float> changed_ {};
};

}  // namespace hui
