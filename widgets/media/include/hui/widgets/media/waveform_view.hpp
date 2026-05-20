#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// 时域音频波形显示。吃一段预处理好的 peak（每个 bar 的幅度 0..1）+ 当前 playhead
// 位置（0..1）。
//
// 为什么不直接吃 AudioBuffer 样本：完整样本密度在 44.1kHz 下一秒 44100 个点，画到
// 几百像素宽的 view 里必须做峰值抽样。上层在 AudioPlayer 里一次性把 decode 出的
// 原始样本 downsample 成 peaks（每像素一对 min/max），然后喂给 WaveformView。
class WaveformView : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "WaveformView";
    }

    // 传入的 peaks 每个元素是 0..1，数量决定柱数（一般等于目标像素宽度）。
    void set_peaks(std::vector<float> peaks) {
        peaks_ = std::move(peaks);
        mark_dirty(DirtyFlags::Paint);
    }

    // Playhead 归一化位置（0..1）。v2 加缩放/区间选中后接口会更丰富。
    void set_progress(float p) noexcept {
        if (progress_ == p) return;
        progress_ = p;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_tone(theme::Tone tone) noexcept {
        if (tone_ == tone) return;
        tone_ = tone;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        const auto& c = theme::colors();
        const Color active = theme::color_for(tone_);

        // 背景
        context.fill_rounded_rect(frame(), c.bg_surface_alt, theme::radius().md);

        if (peaks_.empty()) return;

        const float bar_gap = 1.0F;
        const float total_w = frame().width - 12.0F;  // 左右各留 6px
        const float count = static_cast<float>(peaks_.size());
        const float bar_w = (total_w - bar_gap * (count - 1.0F)) / count;
        if (bar_w <= 0.0F) return;

        const float cy = frame().y + frame().height * 0.5F;
        const float max_half_h = frame().height * 0.4F;

        for (std::size_t i = 0; i < peaks_.size(); ++i) {
            const float t = static_cast<float>(i) / count;
            const float h = max_half_h * peaks_[i];
            const float x = frame().x + 6.0F + i * (bar_w + bar_gap);
            const Color bar_color = (t <= progress_) ? active : c.border_strong;
            context.fill_rounded_rect(
                RectF{x, cy - h, bar_w, h * 2.0F}, bar_color, bar_w * 0.5F);
        }

        // playhead 竖线
        const float head_x = frame().x + 6.0F + progress_ * total_w;
        context.line(PointF{head_x, frame().y + 4.0F},
                     PointF{head_x, frame().y + frame().height - 4.0F},
                     c.accent_hover, 2.0F);
    }

private:
    std::vector<float> peaks_ {};
    float progress_ {0.0F};
    theme::Tone tone_ {theme::Tone::Accent};
};

}  // namespace hui
