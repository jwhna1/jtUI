// Package: hui::foundation
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// codicon.ttf 内嵌字体访问入口。@vscode/codicons (MIT) 的二进制 ttf
// 由 scripts/codicon/generate_codicon_sources.py 生成的 embedded_codicon_font.cpp
// 提供，本头只暴露最小指针 + 长度接口，供 platform 层 (Win32 DirectWrite
// IDWriteInMemoryFontFileLoader) 加载。
#pragma once

#include <cstddef>

namespace hui::foundation {

// 字体二进制 blob 视图。data 指向只读 .rodata 段，调用方不持有所有权，
// 不需要释放。生命周期为整个进程。
struct CodiconFontBlob {
    const void* data;
    std::size_t size;
};

[[nodiscard]] CodiconFontBlob codicon_font_blob() noexcept;

// 字体在 DirectWrite 中注册时使用的逻辑 family name。所有 widget 引用
// codicon 字形时统一用这个串，避免硬编码扩散。
inline constexpr const char* kCodiconFontFamily = "codicon";

}  // namespace hui::foundation
