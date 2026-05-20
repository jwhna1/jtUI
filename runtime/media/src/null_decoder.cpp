#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "hui/media/decoder.hpp"

// Null 解码器：Linux 构建 + 单元测试用。读 mp4/mp3 不真解码，按伪数据"假装"
// 播放，让 widget 层和上层 app 能在非 Windows 环境走通完整流程。
//
// 视频假设一个 640x360 的彩条图；每 1/30s 输出一帧，内容随 presentation_time
// 微微变化（横向扫光条），让 gallery 视觉上能动。
// 音频假设 48kHz stereo，生成正弦波 tone。

namespace hui {

namespace {

class NullVideoDecoder : public IVideoDecoder {
public:
    bool open(const std::string& path) override {
        path_ = path;
        open_ = !path.empty();
        pts_ = 0.0;
        return open_;
    }

    void close() override {
        open_ = false;
    }

    [[nodiscard]] bool is_open() const override { return open_; }

    std::optional<VideoFrame> read_frame() override {
        if (!open_) return std::nullopt;
        if (pts_ >= duration_) return std::nullopt;

        constexpr int w = 640;
        constexpr int h = 360;
        PixelBuffer pb = PixelBuffer::allocate(w, h, PixelFormat::BGRA8);

        // 扫光条：x 位置随 pts 左右移动。
        const auto phase = static_cast<int>(pts_ * 80.0) % (w + 80);
        auto& d = *pb.data;
        for (int y = 0; y < h; ++y) {
            const auto base = static_cast<std::uint8_t>(20 + (y * 80 / h));
            for (int x = 0; x < w; ++x) {
                const int idx = (y * pb.stride) + x * 4;
                const int distance = std::abs(x - phase);
                const bool highlight = distance < 40;
                d[idx + 0] = static_cast<std::uint8_t>(highlight ? 94 : base);   // B
                d[idx + 1] = static_cast<std::uint8_t>(highlight ? 197 : base); // G
                d[idx + 2] = static_cast<std::uint8_t>(highlight ? 34 : base);   // R
                d[idx + 3] = 255;                                                 // A
            }
        }

        VideoFrame frame;
        frame.pixels = std::move(pb);
        frame.presentation_time = pts_;
        frame.duration = 1.0 / 30.0;
        pts_ += frame.duration;
        return frame;
    }

    void seek(double seconds) override {
        pts_ = seconds < 0.0 ? 0.0 : (seconds > duration_ ? duration_ : seconds);
    }

    [[nodiscard]] double duration() const override { return duration_; }
    [[nodiscard]] int width() const override { return 640; }
    [[nodiscard]] int height() const override { return 360; }

private:
    bool open_ {false};
    std::string path_ {};
    double pts_ {0.0};
    double duration_ {12.0};  // 假想 12 秒
};

class NullAudioDecoder : public IAudioDecoder {
public:
    bool open(const std::string& path) override {
        path_ = path;
        open_ = !path.empty();
        pts_ = 0.0;
        return open_;
    }

    void close() override { open_ = false; }
    [[nodiscard]] bool is_open() const override { return open_; }

    std::optional<AudioBuffer> read_buffer() override {
        if (!open_) return std::nullopt;
        if (pts_ >= duration_) return std::nullopt;

        constexpr int frames = 1024;
        constexpr double freq = 440.0;
        auto samples = std::make_shared<std::vector<float>>(
            static_cast<std::size_t>(frames * channels_));
        const double dt = 1.0 / sample_rate_;
        for (int i = 0; i < frames; ++i) {
            const float v = static_cast<float>(
                0.1 * std::sin(2.0 * 3.14159265358979323846 * freq * (pts_ + i * dt)));
            for (int ch = 0; ch < channels_; ++ch) {
                (*samples)[static_cast<std::size_t>(i * channels_ + ch)] = v;
            }
        }

        AudioBuffer buf;
        buf.samples = std::move(samples);
        buf.channels = channels_;
        buf.sample_rate = sample_rate_;
        buf.presentation_time = pts_;
        pts_ += static_cast<double>(frames) / sample_rate_;
        return buf;
    }

    void seek(double seconds) override {
        pts_ = seconds < 0.0 ? 0.0 : (seconds > duration_ ? duration_ : seconds);
    }

    [[nodiscard]] double duration() const override { return duration_; }
    [[nodiscard]] int channels() const override { return channels_; }
    [[nodiscard]] int sample_rate() const override { return sample_rate_; }

private:
    bool open_ {false};
    std::string path_ {};
    double pts_ {0.0};
    double duration_ {8.0};
    int channels_ {2};
    int sample_rate_ {48000};
};

class NullAudioOutput : public IAudioOutput {
public:
    bool open(int sample_rate, int channels) override {
        sample_rate_ = sample_rate;
        channels_ = channels;
        open_ = sample_rate > 0 && channels > 0;
        return open_;
    }

    void close() override { open_ = false; }
    [[nodiscard]] bool is_open() const override { return open_; }

    std::size_t submit(const AudioBuffer& buffer) override {
        if (!open_ || !buffer.valid()) return 0;
        // null 输出：直接吞样本，让上游认为播掉了。
        return buffer.frame_count();
    }

    void set_volume(float linear) override { volume_ = linear; }
    void pause() override { paused_ = true; }
    void resume() override { paused_ = false; }

private:
    bool open_ {false};
    bool paused_ {false};
    float volume_ {1.0F};
    int sample_rate_ {0};
    int channels_ {0};
};

}  // namespace

// 在非 Windows 平台（或 Windows 下未启用 MF 的构建）做默认实现。
// Windows native 构建会由 mf_decoder.cpp 通过 __declspec-less 的 strong symbol 替换。
// 具体：CMake 在 WIN32 时额外链入 win32 实现，这些实现取胜（链接顺序在 mf_decoder 之前
// 被吐出为 hui_media 的静态成员，weak 在平台层）。为避免符号冲突，我们让 factory
// 在 Windows 下走 platform/win32/factory.cpp；此处只是 fallback。
#if !defined(_WIN32)
std::unique_ptr<IVideoDecoder> create_video_decoder() {
    return std::make_unique<NullVideoDecoder>();
}

std::unique_ptr<IAudioDecoder> create_audio_decoder() {
    return std::make_unique<NullAudioDecoder>();
}

std::unique_ptr<IAudioOutput> create_audio_output() {
    return std::make_unique<NullAudioOutput>();
}
#endif

// 这三个工厂符号仅供 Windows 工厂回落使用（例如 MF 初始化失败时）。
namespace detail {

std::unique_ptr<IVideoDecoder> make_null_video_decoder() {
    return std::make_unique<NullVideoDecoder>();
}
std::unique_ptr<IAudioDecoder> make_null_audio_decoder() {
    return std::make_unique<NullAudioDecoder>();
}
std::unique_ptr<IAudioOutput> make_null_audio_output() {
    return std::make_unique<NullAudioOutput>();
}

}  // namespace detail

}  // namespace hui
