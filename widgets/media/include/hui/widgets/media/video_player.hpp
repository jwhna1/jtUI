#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "hui/media/decoder.hpp"
#include "hui/media/frame_types.hpp"
#include "hui/media/texture_handle.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

class VideoPlayer : public Widget {
public:
    enum class PlayState {
        Stopped,
        Playing,
        Paused,
    };

    VideoPlayer();
    ~VideoPlayer() override;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "VideoPlayer";
    }

    bool set_source(const std::string& path);

    void play();
    void pause();
    void stop();
    void seek(double seconds);

    [[nodiscard]] PlayState state() const noexcept { return state_; }
    [[nodiscard]] double position() const noexcept { return position_; }
    [[nodiscard]] double duration() const noexcept { return duration_; }

    // 暴露内部 TextureHandle（id 在 widget 生命周期内稳定，buffer 每帧换）。
    // 测试和未来 GPU registry 集成都要看到 id。
    [[nodiscard]] const TextureHandle& texture() const noexcept { return texture_; }

    [[nodiscard]] Signal<PlayState>& on_state_changed() noexcept { return state_changed_; }

    void on_click(PointF point) override;

    bool tick(float delta) override;

    void paint(PaintContext& context) const override;

private:
    void set_state(PlayState s);
    [[nodiscard]] RectF video_rect() const;
    [[nodiscard]] RectF transport_rect() const;
    [[nodiscard]] RectF play_button_rect() const;
    [[nodiscard]] RectF progress_rect() const;

    std::unique_ptr<IVideoDecoder> decoder_;
    std::optional<VideoFrame> current_frame_;
    TextureHandle texture_ {};
    PlayState state_ {PlayState::Stopped};
    double position_ {0.0};
    double duration_ {0.0};
    std::string source_path_ {};
    Signal<PlayState> state_changed_ {};
};

}  // namespace hui
