// jtUI 图片加载实现 —— 见 image_loader.hpp 设计说明。
// 维护：曾能混 <jwhna1@gmail.com>

#include "hui/foundation/image_loader.hpp"

#include <cstdint>
#include <cstring>
#include <string>

// stb_image 的 implementation 必须在唯一一个 TU 里 #define
// STB_IMAGE_IMPLEMENTATION，其它地方只 include 头部分。
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC                  // 把 stb 函数限定在本 TU
#define STBI_NO_STDIO_INCLUDE_FATAL        // 不让 stb 拒绝某些环境
#define STBI_FAILURE_USERMSG               // 失败时给文字描述（debug 友好）
#include "stb_image.h"

namespace hui::image {

namespace {

// 把 stb 解码出的 4 通道 RGBA buffer 转换成 BGRA + premultiplied alpha 的 PixelBuffer。
// 跟 PaintContext::draw_texture 在 Win32 D2D backend 期望的格式匹配。
PixelBuffer make_bgra_premul_(const std::uint8_t* rgba, int w, int h) {
    PixelBuffer buf = PixelBuffer::allocate(w, h, PixelFormat::BGRA8);
    buf.alpha_mode = AlphaMode::Premultiplied;

    auto* dst = buf.data->data();
    const std::size_t total = static_cast<std::size_t>(w)
                            * static_cast<std::size_t>(h);
    for (std::size_t i = 0; i < total; ++i) {
        const std::uint8_t r = rgba[i * 4 + 0];
        const std::uint8_t g = rgba[i * 4 + 1];
        const std::uint8_t b = rgba[i * 4 + 2];
        const std::uint8_t a = rgba[i * 4 + 3];
        // premultiply：rgb *= a / 255。整数除法对常见 alpha 足够准确，
        // 视觉差异 < 1 LSB。
        const std::uint16_t af = a;
        dst[i * 4 + 0] = static_cast<std::uint8_t>((b * af + 127) / 255);  // B
        dst[i * 4 + 1] = static_cast<std::uint8_t>((g * af + 127) / 255);  // G
        dst[i * 4 + 2] = static_cast<std::uint8_t>((r * af + 127) / 255);  // R
        dst[i * 4 + 3] = a;                                                 // A
    }
    return buf;
}

}  // namespace

PixelBuffer load(std::string_view path) {
    // stbi_load 需要 nul-terminated path
    std::string p(path);
    int w = 0, h = 0, ch = 0;
    std::uint8_t* rgba = stbi_load(p.c_str(), &w, &h, &ch, 4);  // 强制 4 通道
    if (rgba == nullptr || w <= 0 || h <= 0) {
        if (rgba) stbi_image_free(rgba);
        return PixelBuffer{};
    }
    PixelBuffer buf = make_bgra_premul_(rgba, w, h);
    stbi_image_free(rgba);
    return buf;
}

PixelBuffer load_from_memory(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) return PixelBuffer{};
    int w = 0, h = 0, ch = 0;
    std::uint8_t* rgba = stbi_load_from_memory(
        data, static_cast<int>(size), &w, &h, &ch, 4);
    if (rgba == nullptr || w <= 0 || h <= 0) {
        if (rgba) stbi_image_free(rgba);
        return PixelBuffer{};
    }
    PixelBuffer buf = make_bgra_premul_(rgba, w, h);
    stbi_image_free(rgba);
    return buf;
}

}  // namespace hui::image
