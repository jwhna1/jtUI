#include "hui/widgets/media/video_player.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <string>
#include <utility>

#include "hui/theme/theme.hpp"

namespace hui {

namespace {
// 进程内单调 id 生成器。资源注册到渲染后端时要有稳定 id；v1 暂不做真正的
// resource registry，渲染端看到 id == 0 会画占位，看到正数会尝试从 TextureHandle
// 的 buffer 即时上载（platform layer 要做的事）。
std::atomic<std::uint64_t> g_texture_id {1};
}

VideoPlayer::VideoPlayer() : decoder_(create_video_decoder()) {
    // texture id 在 widget 生命周期内稳定不变，buffer 每帧替换。这样未来接 GPU
    // texture registry 时（stable id → cached SkImage / D2D bitmap）才能正确
    // invalidate 缓存（renderer 自己看 buffer 指针变化决定 re-upload）。之前每帧
    // fetch_add 一个新 id，等同于告诉 renderer "这是新纹理"，cache 永远 miss。
    texture_.id = g_texture_id.fetch_add(1, std::memory_order_relaxed);
}

VideoPlayer::~VideoPlayer() {
    if (decoder_ && decoder_->is_open()) decoder_->close();
}

bool VideoPlayer::set_source(const std::string& path) {
    stop();
    source_path_ = path;
    if (!decoder_) decoder_ = create_video_decoder();
    if (!decoder_->open(path)) {
        mark_dirty(DirtyFlags::Paint);
        return false;
    }
    duration_ = decoder_->duration();
    position_ = 0.0;
    current_frame_.reset();
    // 清 buffer 但保留 id 稳定，让下游 renderer 知道是同一个 texture slot 的内容更换。
    texture_.buffer = {};
    texture_.size = {};
    mark_dirty(DirtyFlags::Paint);
    return true;
}

void VideoPlayer::play() {
    if (!decoder_ || !decoder_->is_open()) return;
    set_state(PlayState::Playing);
}

void VideoPlayer::pause() {
    if (state_ != PlayState::Playing) return;
    set_state(PlayState::Paused);
}

void VideoPlayer::stop() {
    if (state_ == PlayState::Stopped) return;
    if (decoder_ && decoder_->is_open()) decoder_->seek(0.0);
    position_ = 0.0;
    current_frame_.reset();
    // 同 set_source：清 buffer 但保留 stable texture id。
    texture_.buffer = {};
    texture_.size = {};
    set_state(PlayState::Stopped);
    mark_dirty(DirtyFlags::Paint);
}

void VideoPlayer::seek(double seconds) {
    if (!decoder_ || !decoder_->is_open()) return;
    const double target = std::clamp(seconds, 0.0, duration_);
    decoder_->seek(target);
    position_ = target;
    mark_dirty(DirtyFlags::Paint);
}

bool VideoPlayer::tick(float delta) {
    if (state_ != PlayState::Playing || !decoder_ || !decoder_->is_open()) return false;

    position_ += static_cast<double>(delta);
    if (duration_ > 0.0 && position_ >= duration_) {
        stop();
        return false;
    }

    // 简单的帧跟随：position 超过 current_frame 的 pts + duration 就读下一帧。
    // 视频播放端严格按 pts 而非经过时间对齐，避免漂移。
    while (!current_frame_ ||
           current_frame_->presentation_time + current_frame_->duration <= position_) {
        auto next = decoder_->read_frame();
        if (!next) {
            stop();
            return false;
        }
        current_frame_ = std::move(next);
        // 只换 buffer / size，id 在构造时分配后保持稳定。renderer 看 buffer
        // 指针变化决定是否 re-upload；id 是 GPU registry 的稳定 cache key。
        texture_.buffer = current_frame_->pixels;
        texture_.size = SizeF{
            static_cast<float>(current_frame_->pixels.width),
            static_cast<float>(current_frame_->pixels.height),
        };
    }

    mark_dirty(DirtyFlags::Paint);
    return true;
}

void VideoPlayer::on_click(PointF point) {
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

void VideoPlayer::paint(PaintContext& context) const {
    const auto& c = theme::colors();

    context.fill_rounded_rect(frame(), c.bg_raised, theme::radius().lg);

    // 视频画面区（letterbox：按源宽高比 contain 居中显示，不强制拉伸；
    // 留黑边给非匹配比例的容器，主流播放器默认行为）
    const RectF vid = video_rect();
    context.fill_rounded_rect(vid, c.bg_base, theme::radius().md);
    if (texture_.valid()) {
        const float src_w = texture_.size.width;
        const float src_h = texture_.size.height;
        RectF dest = vid;
        if (src_w > 0.0F && src_h > 0.0F && vid.width > 0.0F && vid.height > 0.0F) {
            const float src_aspect = src_w / src_h;
            const float dst_aspect = vid.width / vid.height;
            if (src_aspect > dst_aspect) {
                // 源更宽：宽度顶满，高度按源比缩，上下留黑边
                dest.width  = vid.width;
                dest.height = vid.width / src_aspect;
                dest.x      = vid.x;
                dest.y      = vid.y + (vid.height - dest.height) * 0.5F;
            } else {
                // 源更高：高度顶满，宽度按源比缩，左右留黑边
                dest.height = vid.height;
                dest.width  = vid.height * src_aspect;
                dest.x      = vid.x + (vid.width - dest.width) * 0.5F;
                dest.y      = vid.y;
            }
        }
        context.draw_texture(dest, texture_.buffer, Color{1.0F, 1.0F, 1.0F, 1.0F},
                             texture_.id);
    } else {
        context.draw_text(vid, "\xe2\x96\xb6",  // ▶
                          c.text_muted, TextAlignment::Center, 48.0F, true);
    }

    // Transport bar 底色
    const RectF bar = transport_rect();
    context.fill_rounded_rect(bar, c.bg_surface_alt, theme::radius().sm);

    // 播放按钮
    const RectF btn = play_button_rect();
    context.fill_ellipse(btn, hovered() ? c.accent_hover : c.accent);
    const std::string glyph = state_ == PlayState::Playing ? "||" : "\xe2\x96\xb6";
    context.draw_text(btn, glyph, c.accent_fg, TextAlignment::Center, 14.0F, true);

    // 时间
    auto format_time = [](double s) {
        const int total = static_cast<int>(s);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d:%02d", total / 60, total % 60);
        return std::string{buf};
    };
    const std::string time_str = format_time(position_) + " / " + format_time(duration_);
    context.draw_text(
        RectF{btn.x + btn.width + 10.0F, bar.y + 8.0F, 110.0F, 16.0F},
        time_str, c.text_secondary, TextAlignment::Leading, 11.0F);

    // 进度条
    const RectF pb = progress_rect();
    context.fill_rounded_rect(pb, c.track, pb.height * 0.5F);
    const float t = duration_ > 0.0 ? static_cast<float>(position_ / duration_) : 0.0F;
    context.fill_rounded_rect(
        RectF{pb.x, pb.y, pb.width * t, pb.height}, c.accent, pb.height * 0.5F);
}

void VideoPlayer::set_state(PlayState s) {
    if (state_ == s) return;
    state_ = s;
    state_changed_.emit(s);
    mark_dirty(DirtyFlags::Paint);
}

RectF VideoPlayer::video_rect() const {
    const float bar_h = 48.0F;
    return RectF{frame().x + 8.0F, frame().y + 8.0F,
                 frame().width - 16.0F, frame().height - bar_h - 16.0F};
}

RectF VideoPlayer::transport_rect() const {
    const float bar_h = 48.0F;
    return RectF{frame().x + 8.0F, frame().y + frame().height - bar_h - 4.0F,
                 frame().width - 16.0F, bar_h};
}

RectF VideoPlayer::play_button_rect() const {
    const RectF bar = transport_rect();
    const float size = 32.0F;
    return RectF{bar.x + 8.0F, bar.y + (bar.height - size) * 0.5F, size, size};
}

RectF VideoPlayer::progress_rect() const {
    const RectF bar = transport_rect();
    const float h = 6.0F;
    const float left = bar.x + 160.0F;
    const float right = bar.x + bar.width - 16.0F;
    return RectF{left, bar.y + (bar.height - h) * 0.5F + 6.0F, right - left, h};
}

}  // namespace hui
