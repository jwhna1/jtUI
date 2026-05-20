#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace hui {

enum class PixelFormat {
    BGRA8,  // 一般桌面渲染用（Direct2D / GDI+ Bitmap 原生格式）
    RGBA8,  // 视频解码常见（某些 FFmpeg 输出）
};

// alpha 通道的解释方式 — 影响 D2D bitmap 创建参数。
//   Ignore        : 把 alpha 视作 padding（D2D1_ALPHA_MODE_IGNORE）；
//                   视频解码器（MF_RGB32）输出常 alpha=0,需要这个模式才能可见。
//   Premultiplied : RGB 已与 alpha 预乘（D2D1_ALPHA_MODE_PREMULTIPLIED）；
//                   PNG / SVG 装饰资源走这个,透明像素 = 真正透明。
enum class AlphaMode {
    Ignore,
    Premultiplied,
};

// 像素缓冲。放在 foundation 层是为了让 render（绘制端）和 media（解码端）
// 都能引用同一个类型而不出现反向依赖：
//   - runtime/media 生产 PixelBuffer（解码帧）
//   - runtime/render 消费 PixelBuffer（draw_texture 命令）
// data 是 shared_ptr<vector>，让 producer / consumer / renderer 各自持有
// 都不影响生命周期；单帧几 MB，按值拷贝不现实，共享是唯一合理选项。
struct PixelBuffer {
    std::shared_ptr<std::vector<std::uint8_t>> data;
    int width {0};
    int height {0};
    int stride {0};  // bytes per row
    PixelFormat format {PixelFormat::BGRA8};
    AlphaMode alpha_mode {AlphaMode::Ignore};

    [[nodiscard]] bool empty() const noexcept {
        return !data || data->empty() || width <= 0 || height <= 0;
    }

    [[nodiscard]] static PixelBuffer allocate(int w, int h, PixelFormat fmt) {
        const int bpp = 4;  // BGRA8 / RGBA8 都是 4 通道 8 bit
        const int stride = w * bpp;
        PixelBuffer buf;
        buf.data = std::make_shared<std::vector<std::uint8_t>>(
            static_cast<std::size_t>(stride) * static_cast<std::size_t>(h));
        buf.width = w;
        buf.height = h;
        buf.stride = stride;
        buf.format = fmt;
        return buf;
    }
};

}  // namespace hui
