#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "hui/media/texture_handle.hpp"

namespace hui {

// 解码出来的一帧视频。pixels 用 PixelBuffer 共享所有权。
struct VideoFrame {
    PixelBuffer pixels {};
    double presentation_time {0.0};  // 秒
    double duration {0.0};           // 秒

    [[nodiscard]] bool valid() const noexcept { return !pixels.empty(); }
};

// 一包音频样本。interleaved float 格式（WASAPI 默认格式也是 float32）。
struct AudioBuffer {
    std::shared_ptr<std::vector<float>> samples;  // interleaved
    int channels {0};
    int sample_rate {0};
    double presentation_time {0.0};  // 秒

    [[nodiscard]] std::size_t frame_count() const noexcept {
        if (!samples || channels <= 0) return 0;
        return samples->size() / static_cast<std::size_t>(channels);
    }

    [[nodiscard]] bool valid() const noexcept {
        return samples && !samples->empty() && channels > 0 && sample_rate > 0;
    }
};

}  // namespace hui
