#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hui/media/decoder.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

// 一个组合 widget：封装 decoder + 音频输出 + WaveformView + transport。
// 该 widget 自己 paint 的是 transport 区（播放按钮 / 时间 / 进度条），
// 波形显示由外部另加 WaveformView 挂上 peaks_property() 数据。
// 这样 gallery 能自由决定布局（波形在 transport 上方还是两侧）。
//
// 状态：Stopped / Playing / Paused。set_source(path) 重置所有状态。
// tick 中：读 decoder 推到 output；推 pts 更新进度；EOF 后自动 stop。
class AudioPlayer : public Widget {
public:
    enum class PlayState {
        Stopped,
        Playing,
        Paused,
    };

    AudioPlayer();
    ~AudioPlayer() override;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "AudioPlayer";
    }

    // 加载文件。成功返回 true。失败仍会保留 decoder 的错误状态，paint 会显示 "No audio"。
    bool set_source(const std::string& path);

    void play();
    void pause();
    void stop();

    void set_volume(float linear);
    void seek(double seconds);

    [[nodiscard]] PlayState state() const noexcept { return state_; }
    [[nodiscard]] double position() const noexcept { return position_; }
    [[nodiscard]] double duration() const noexcept { return duration_; }
    [[nodiscard]] const std::vector<float>& peaks() const noexcept { return peaks_; }

    [[nodiscard]] Signal<PlayState>& on_state_changed() noexcept { return state_changed_; }
    [[nodiscard]] Signal<double>& on_position_changed() noexcept { return position_changed_; }

    void on_click(PointF point) override;

    bool tick(float delta) override;

    void paint(PaintContext& context) const override;

private:
    void set_state(PlayState s);
    void recompute_peaks();
    [[nodiscard]] RectF play_button_rect() const;
    [[nodiscard]] RectF progress_rect() const;

    std::unique_ptr<IAudioDecoder> decoder_;
    std::unique_ptr<IAudioOutput> output_;
    PlayState state_ {PlayState::Stopped};
    double position_ {0.0};            // 显示给外部的播放位置（= played_position_）
    double played_position_ {0.0};     // 实际播放推进，按 tick wall-clock delta 累积
    double submitted_position_ {0.0};  // 解码已 submit 给 output 的最远 PTS
    double duration_ {0.0};
    float volume_ {1.0F};
    bool decoder_eof_ {false};
    float post_eof_grace_ {0.0F};      // EOF 后给 WASAPI 排空的余量
    std::string source_path_ {};
    std::vector<float> peaks_ {};  // 供 WaveformView 读
    Signal<PlayState> state_changed_ {};
    Signal<double> position_changed_ {};
};

}  // namespace hui
