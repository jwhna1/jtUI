#pragma once

#include <cstdint>

#include "hui/foundation/geometry.hpp"
#include "hui/foundation/pixel_buffer.hpp"

namespace hui {

// 渲染端能识别的纹理句柄。id 是渲染端自己的映射键（0 = 无效）。
// buffer 是 CPU 侧的像素源，renderer 在 replay 时按需 upload 到 D2D bitmap /
// GDI+ bitmap / 之后接 Skia 时换成 SkImage。
struct TextureHandle {
    std::uint64_t id {0};
    SizeF size {};
    PixelBuffer buffer {};

    [[nodiscard]] bool valid() const noexcept {
        return id != 0 && !buffer.empty();
    }
};

}  // namespace hui
