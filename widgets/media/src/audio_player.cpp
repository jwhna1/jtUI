#include "hui/widgets/media/audio_player.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <utility>

#include "hui/theme/theme.hpp"

namespace hui {

AudioPlayer::AudioPlayer()
    : decoder_(create_audio_decoder()), output_(create_audio_output()) {}

AudioPlayer::~AudioPlayer() {
    if (output_ && output_->is_open()) output_->close();
    if (decoder_ && decoder_->is_open()) decoder_->close();
}

bool AudioPlayer::set_source(const std::string& path) {
    stop();
    source_path_ = path;
    if (!decoder_) decoder_ = create_audio_decoder();
    if (!decoder_->open(path)) {
        mark_dirty(DirtyFlags::Paint);
        return false;
    }
    duration_ = decoder_->duration();
    position_ = 0.0;
    played_position_ = 0.0;
    submitted_position_ = 0.0;
    decoder_eof_ = false;
    post_eof_grace_ = 0.0F;
    peaks_.clear();
    recompute_peaks();
    mark_dirty(DirtyFlags::Paint);
    return true;
}

void AudioPlayer::play() {
    if (!decoder_ || !decoder_->is_open()) return;
    if (!output_) output_ = create_audio_output();
    if (!output_->is_open()) {
        output_->open(decoder_->sample_rate(), decoder_->channels());
    }
    output_->resume();
    set_state(PlayState::Playing);
}

void AudioPlayer::pause() {
    if (state_ != PlayState::Playing) return;
    if (output_) output_->pause();
    set_state(PlayState::Paused);
}

void AudioPlayer::stop() {
    if (state_ == PlayState::Stopped) return;
    if (output_ && output_->is_open()) output_->close();
    if (decoder_ && decoder_->is_open()) decoder_->seek(0.0);
    position_ = 0.0;
    played_position_ = 0.0;
    submitted_position_ = 0.0;
    decoder_eof_ = false;
    post_eof_grace_ = 0.0F;
    set_state(PlayState::Stopped);
    position_changed_.emit(position_);
    mark_dirty(DirtyFlags::Paint);
}

void AudioPlayer::set_volume(float linear) {
    volume_ = std::clamp(linear, 0.0F, 1.0F);
    if (output_) output_->set_volume(volume_);
}

void AudioPlayer::seek(double seconds) {
    if (!decoder_ || !decoder_->is_open()) return;
    const double target = std::clamp(seconds, 0.0, duration_);
    decoder_->seek(target);
    position_ = target;
    played_position_ = target;
    submitted_position_ = target;
    decoder_eof_ = false;
    post_eof_grace_ = 0.0F;
    position_changed_.emit(position_);
    mark_dirty(DirtyFlags::Paint);
}

bool AudioPlayer::tick(float delta) {
    if (state_ != PlayState::Playing || !decoder_ || !decoder_->is_open()) return false;

    // 双状态量：
    //   submitted_position_：解码已 submit 给 output 的最远 PTS
    //   played_position_：实际播放进度，按真实 tick wall-clock delta 累加
    //
    // 关键：一个 tick 内 while 循环 submit 多个包到 buffered ≥ target，
    // 这样即使 WM_TIMER 因 UI 抖动延迟（比如 50ms 的卡顿），下一个 tick 一次性
    // 把欠的几个包都补上，WASAPI ring 不会饿。之前每 tick 单包补，UI 抖动
    // 一次就让真实 ring 直接见底，听起来就是"一顿一顿的"。
    //
    // 1.0s 缓冲：WASAPI 内部 ring 200ms，加 800ms deque cushion，给系统调度
    // 留充足余量。1.0s 比 0.4s 多用约 8 MB 内存（48k * 2 ch * 4 byte * 1s ≈ 384 KB
    // 实际很少），换的是平稳无毛刺。
    constexpr double kTargetBufferSec = 1.0;

    if (!decoder_eof_) {
        while ((submitted_position_ - played_position_) < kTargetBufferSec) {
            auto buf = decoder_->read_buffer();
            if (!buf) {
                decoder_eof_ = true;
                // EOF 后给 WASAPI 200ms ring 多 100ms 余量（共 300ms）排空再 stop
                post_eof_grace_ = 0.3F;
                break;
            }
            if (output_) output_->submit(*buf);
            const int sr = decoder_->sample_rate();
            if (sr > 0) {
                submitted_position_ = buf->presentation_time +
                                       static_cast<double>(buf->frame_count()) /
                                           static_cast<double>(sr);
            }
        }
    }

    // 实际播放按 wall-clock 推进（WASAPI 是 real-time，1 秒墙钟 = 1 秒播放）
    played_position_ += static_cast<double>(delta);
    if (played_position_ > submitted_position_) {
        // 不能超过已 submit 的位置（理论上只在 underrun 时发生）
        played_position_ = submitted_position_;
    }
    position_ = played_position_;

    if (decoder_eof_) {
        // EOF 后 played 追上 submitted（音频真播完）+ grace 余量到了，才 stop
        if (played_position_ >= submitted_position_ - 0.01) {
            post_eof_grace_ -= delta;
            if (post_eof_grace_ <= 0.0F) {
                stop();
                return false;
            }
        }
    }

    position_changed_.emit(position_);
    mark_dirty(DirtyFlags::Paint);
    return true;
}

void AudioPlayer::on_click(PointF point) {
    if (!enabled()) return;

    if (play_button_rect().contains(point)) {
        if (state_ == PlayState::Playing) pause();
        else play();
        return;
    }

    const RectF progress = progress_rect();
    if (progress.contains(point) && duration_ > 0.0) {
        const double t = (point.x - progress.x) / progress.width;
        seek(t * duration_);
    }
}

void AudioPlayer::paint(PaintContext& context) const {
    const auto& c = theme::colors();

    context.fill_rounded_rect(frame(), c.bg_raised, theme::radius().lg);
    context.stroke_rounded_rect(frame(), c.border, theme::radius().lg, 1.0F);

    // 播放按钮（左侧圆形）
    const RectF btn = play_button_rect();
    context.fill_ellipse(btn, hovered() ? c.accent_hover : c.accent);
    const std::string glyph = state_ == PlayState::Playing ? "||" : "\xe2\x96\xb6";  // ▶
    context.draw_text(btn, glyph, c.accent_fg, TextAlignment::Center, 16.0F, true);

    // 时间文字（mm:ss / mm:ss）
    auto format_time = [](double s) {
        const int total = static_cast<int>(s);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d:%02d", total / 60, total % 60);
        return std::string{buf};
    };
    const std::string time_str = format_time(position_) + " / " + format_time(duration_);
    context.draw_text(
        RectF{btn.x + btn.width + 12.0F, frame().y + 14.0F, 120.0F, 18.0F},
        time_str, c.text_secondary, TextAlignment::Leading, 12.0F);

    if (!source_path_.empty() && (!decoder_ || !decoder_->is_open())) {
        context.draw_text(frame(), "No audio source", c.text_muted,
                          TextAlignment::Center, 13.0F);
        return;
    }

    // 进度条
    const RectF pb = progress_rect();
    context.fill_rounded_rect(pb, c.track, pb.height * 0.5F);
    const float t = duration_ > 0.0 ? static_cast<float>(position_ / duration_) : 0.0F;
    context.fill_rounded_rect(
        RectF{pb.x, pb.y, pb.width * t, pb.height}, c.accent, pb.height * 0.5F);
}

void AudioPlayer::set_state(PlayState s) {
    if (state_ == s) return;
    state_ = s;
    state_changed_.emit(s);
    mark_dirty(DirtyFlags::Paint);
}

void AudioPlayer::recompute_peaks() {
    // 生成一段静态 peaks 给 WaveformView 用。真做法是读完全部样本再 downsample，
    // 这里为了演示不阻塞 UI，造一段伪随机但稳定的峰值序列（基于 path 哈希）。
    constexpr std::size_t bucket_count = 120;
    peaks_.resize(bucket_count);
    std::size_t seed = 0;
    for (char ch : source_path_) seed = seed * 131 + static_cast<std::size_t>(ch);
    for (std::size_t i = 0; i < bucket_count; ++i) {
        const double x = static_cast<double>(i) / bucket_count;
        const double base = 0.4 + 0.35 * std::sin(x * 6.28 + static_cast<double>(seed % 31));
        const double jitter = 0.15 * std::sin(x * 18.84 + static_cast<double>(seed % 17));
        peaks_[i] = static_cast<float>(std::clamp(base + jitter, 0.05, 1.0));
    }
}

RectF AudioPlayer::play_button_rect() const {
    const float size = 40.0F;
    return RectF{frame().x + 14.0F, frame().y + (frame().height - size) * 0.5F, size, size};
}

RectF AudioPlayer::progress_rect() const {
    const float h = 6.0F;
    const float left = frame().x + 180.0F;
    const float right = frame().x + frame().width - 16.0F;
    return RectF{left, frame().y + (frame().height - h) * 0.5F + 8.0F, right - left, h};
}

}  // namespace hui
