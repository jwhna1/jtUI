#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hui/object/widget.hpp"
#include "hui/property/realtime_source.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"
#include "hui/theme/token_override.hpp"

namespace hui {

// Timeline：spec 差异化组件四件套之一。
//
// 轴 [0, duration] 秒；playhead 是当前时间；一组 marker（time + label，画作
// 竖线 + 顶部小标签）。
//
// 三种数据来源：
//   1) 主动 set：set_playhead(seconds)
//   2) 用户交互：on_click 把 click x 换算成时间，emit on_seek（host 接住改 player）
//   3) 高频通道：bind_position_source(RealtimeSource<double>*)，tick 检测
//      generation 变化把 source.latest() 同步到 playhead_ —— 与 AudioPlayer /
//      VideoPlayer 这种另一线程驱动 position 的场景对接，不用 host 手工转发。

class Timeline : public Widget {
public:
    struct Marker {
        double time;
        std::string label;
    };

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Timeline";
    }

    void set_duration(double seconds) noexcept {
        const double clamped = seconds < 0.0 ? 0.0 : seconds;
        if (duration_ == clamped) return;
        duration_ = clamped;
        playhead_ = std::clamp(playhead_, 0.0, duration_);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] double duration() const noexcept { return duration_; }

    void set_playhead(double seconds) noexcept {
        const double clamped = std::clamp(seconds, 0.0, duration_);
        if (playhead_ == clamped) return;
        playhead_ = clamped;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] double playhead() const noexcept { return playhead_; }

    void add_marker(double time, std::string label) {
        markers_.push_back(Marker{std::clamp(time, 0.0, duration_), std::move(label)});
        mark_dirty(DirtyFlags::Paint);
    }

    void set_markers(std::vector<Marker> markers) {
        markers_ = std::move(markers);
        for (auto& m : markers_) {
            m.time = std::clamp(m.time, 0.0, duration_);
        }
        mark_dirty(DirtyFlags::Paint);
    }

    void clear_markers() {
        if (markers_.empty()) return;
        markers_.clear();
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] const std::vector<Marker>& markers() const noexcept { return markers_; }

    void bind_position_source(RealtimeSource<double>* source) {
        if (source == nullptr) {
            position_probe_ = nullptr;
            position_pull_ = nullptr;
        } else {
            position_probe_ = [source]() noexcept { return source->generation(); };
            position_pull_ = [source]() { return source->latest(); };
        }
        last_observed_generation_ = 0;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool is_position_source_bound() const noexcept {
        return static_cast<bool>(position_probe_);
    }

    [[nodiscard]] Signal<double>& on_seek() noexcept { return seek_; }

    // 组件级 token override（spec §13.4）。设了之后 paint 用 theme::resolve
    // 合并 base + 这个 override。典型用法：
    //   timeline->set_token_override(theme::TokenOverride{}.with_accent(Color::from_hex("#FF6A00")));
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

    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled();
    }

    void on_click(PointF point) override {
        if (!enabled() || duration_ <= 0.0) return;
        const float track_w = std::max(frame().width - 16.0F, 1.0F);
        const float t = std::clamp((point.x - frame().x - 8.0F) / track_w, 0.0F, 1.0F);
        const double new_pos = static_cast<double>(t) * duration_;
        set_playhead(new_pos);
        seek_.emit(new_pos);
    }

    bool on_key_down(std::int32_t key_code) override {
        if (!enabled() || duration_ <= 0.0) return false;
        constexpr std::int32_t vk_left = 0x25;
        constexpr std::int32_t vk_right = 0x27;
        constexpr std::int32_t vk_home = 0x24;
        constexpr std::int32_t vk_end = 0x23;
        // 步长：1 秒（整音视频典型场景）。Shift / Ctrl 可后续按 modifier 缩放
        constexpr double step = 1.0;
        switch (key_code) {
        case vk_left:
            set_playhead(playhead_ - step);
            seek_.emit(playhead_);
            return true;
        case vk_right:
            set_playhead(playhead_ + step);
            seek_.emit(playhead_);
            return true;
        case vk_home:
            set_playhead(0.0);
            seek_.emit(playhead_);
            return true;
        case vk_end:
            set_playhead(duration_);
            seek_.emit(playhead_);
            return true;
        default:
            return false;
        }
    }

    bool tick(float /*delta*/) override {
        if (!position_probe_) return false;
        const auto g = position_probe_();
        if (g != last_observed_generation_) {
            last_observed_generation_ = g;
            if (position_pull_) {
                const auto v = position_pull_();
                if (v.has_value()) {
                    set_playhead(*v);
                }
            }
        }
        // 绑了 source 就保持 timer active，让 producer 在另一线程 publish 能被
        // 主线程 sample（与 RealtimeCanvas 同样的"高频通道"语义）
        return true;
    }

    void paint(PaintContext& context) const override {
        // theme::resolve 把全局 base + widget-local TokenOverride 合并，没设 override
        // 时等价于拷一份 colors()。开销是一份 SemanticColor 拷贝，在 paint 路径上
        // 可以接受。
        const auto c = theme::resolve(token_override_.get());
        const float r = theme::radius().md;

        context.fill_rounded_rect(frame(), c.bg_surface_alt, r);
        if (focused()) {
            context.stroke_rounded_rect(frame(), c.accent, r, 1.5F);
        } else {
            context.stroke_rounded_rect(frame(), c.border, r, 1.0F);
        }

        if (duration_ <= 0.0) {
            context.draw_text(frame(), "Timeline (no duration)", c.text_muted,
                              TextAlignment::Center, 12.0F);
            return;
        }

        // 中心轨道
        const float padding = 8.0F;
        const float track_y = frame().y + frame().height * 0.5F - 2.0F;
        const float track_w = frame().width - padding * 2.0F;
        const RectF track{frame().x + padding, track_y, track_w, 4.0F};
        context.fill_rounded_rect(track, c.track, 2.0F);

        // 已经过区域
        const float t = static_cast<float>(playhead_ / duration_);
        const RectF passed{track.x, track.y, track.width * t, track.height};
        context.fill_rounded_rect(passed, c.accent, 2.0F);

        // markers
        for (const auto& m : markers_) {
            const float mx = track.x + track.width * static_cast<float>(m.time / duration_);
            context.line(PointF{mx, frame().y + 4.0F},
                         PointF{mx, frame().y + frame().height - 4.0F},
                         c.accent_soft, 1.0F);
            if (!m.label.empty()) {
                context.draw_text(
                    RectF{mx - 30.0F, frame().y + 2.0F, 60.0F, 14.0F},
                    m.label, c.text_secondary, TextAlignment::Center, 10.0F);
            }
        }

        // playhead 竖线 + 头部圆点
        const float head_x = track.x + track.width * t;
        context.line(PointF{head_x, frame().y + 6.0F},
                     PointF{head_x, frame().y + frame().height - 6.0F},
                     c.accent, 2.0F);
        context.fill_ellipse(
            RectF{head_x - 5.0F, frame().y + frame().height * 0.5F - 5.0F, 10.0F, 10.0F},
            c.accent);
    }

private:
    double duration_{0.0};
    double playhead_{0.0};
    std::vector<Marker> markers_{};
    Signal<double> seek_{};
    std::function<std::uint64_t()> position_probe_{};
    std::function<std::optional<double>()> position_pull_{};
    std::uint64_t last_observed_generation_{0};
    std::unique_ptr<theme::TokenOverride> token_override_{};
};

}  // namespace hui
