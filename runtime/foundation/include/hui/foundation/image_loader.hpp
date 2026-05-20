#pragma once

// jtUI 图片加载 —— 把 PNG/JPG/BMP 等常见格式解码成 PixelBuffer (BGRA8)。
// 维护：曾能混 <jwhna1@gmail.com>
//
// 设计：
//   - 内部用 stb_image v2.30 (third_party/stb/stb_image.h) 做解码，公共领域许可。
//   - 解码出来强制转 BGRA8 + Premultiplied alpha，与 PaintContext::draw_texture
//     在 D2D backend 的渲染管线匹配。RGBA → BGRA 在我们这一层做掉，业务方零意识。
//   - 失败返回空 PixelBuffer（empty() == true），业务方自己决定 fallback 策略
//     （例如换回程序化背景）。不抛异常。
//
// 用法：
//   const auto bg = jtui::image::load_png("assets/hero_bg.png");
//   if (!bg.empty()) context.draw_texture(bounds, bg);

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "hui/foundation/pixel_buffer.hpp"

namespace hui::image {

// 从文件路径加载。成功返回 BGRA8 PixelBuffer，失败返回 empty。
// 支持 PNG / JPG / BMP / TGA / GIF（首帧）/ PSD / HDR / PNM —— stb_image 全部覆盖。
[[nodiscard]] PixelBuffer load(std::string_view path);

// 从内存缓冲加载（业务把图片打成 .rc / 嵌入 .a 时用）。
[[nodiscard]] PixelBuffer load_from_memory(const std::uint8_t* data, std::size_t size);

// 便捷 alias，调用方语义更清晰
[[nodiscard]] inline PixelBuffer load_png(std::string_view path) { return load(path); }
[[nodiscard]] inline PixelBuffer load_jpg(std::string_view path) { return load(path); }

}  // namespace hui::image
