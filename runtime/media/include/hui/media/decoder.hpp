#pragma once

#include <memory>
#include <optional>
#include <string>

#include "hui/media/frame_types.hpp"

namespace hui {

// 解码器抽象。平台 backend（Windows Media Foundation / 后续 Linux FFmpeg 等）
// 实现这对接口；factory 按平台返回实体。
//
// 接口有意做小：open/read/seek/close 四件套，不暴露底层格式/编解码细节。
// VideoPlayer / AudioPlayer 组件按 60 FPS 驱动 read_frame，结合 RealtimeRingBuffer
// 做 producer-consumer 解耦。
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool is_open() const = 0;

    // 返回下一帧；EOF 或尚未可用 → nullopt
    virtual std::optional<VideoFrame> read_frame() = 0;

    virtual void seek(double seconds) = 0;
    [[nodiscard]] virtual double duration() const = 0;

    // 帧的标准像素尺寸（方便上层分配）。未打开时 {0,0}。
    [[nodiscard]] virtual int width() const = 0;
    [[nodiscard]] virtual int height() const = 0;
};

class IAudioDecoder {
public:
    virtual ~IAudioDecoder() = default;

    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool is_open() const = 0;

    virtual std::optional<AudioBuffer> read_buffer() = 0;

    virtual void seek(double seconds) = 0;
    [[nodiscard]] virtual double duration() const = 0;

    [[nodiscard]] virtual int channels() const = 0;
    [[nodiscard]] virtual int sample_rate() const = 0;
};

// 平台无关的音频输出。渲染设备抽象，典型实现是 WASAPI（Windows）。
// Linux 当前只需要 stub 能通过，等 Stage C 后半补 PulseAudio/ALSA。
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    // 准备输出设备。format 跟着上游 decoder 走。
    virtual bool open(int sample_rate, int channels) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool is_open() const = 0;

    // 投放一包样本到设备缓冲。非阻塞，返回实际接受的样本数。
    virtual std::size_t submit(const AudioBuffer& buffer) = 0;

    virtual void set_volume(float linear) = 0;  // 0..1
    virtual void pause() = 0;
    virtual void resume() = 0;
};

// Factory：按平台返回实体。Windows 下返回 MF/WASAPI；其他平台返回 Null stub。
[[nodiscard]] std::unique_ptr<IVideoDecoder> create_video_decoder();
[[nodiscard]] std::unique_ptr<IAudioDecoder> create_audio_decoder();
[[nodiscard]] std::unique_ptr<IAudioOutput> create_audio_output();

}  // namespace hui
