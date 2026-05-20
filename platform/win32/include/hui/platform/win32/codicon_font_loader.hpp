// Package: hui::platform::win32
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// Codicon 字体加载器 -- DirectWrite 端。把 hui::foundation::codicon_font_blob()
// 里嵌入的 ttf 注册成一个 IDWriteFontCollection, 允许 CreateTextFormat 用
// "codicon" 作为 fontFamilyName 找到字体。
//
// 实施路径: IDWriteFactory5::CreateInMemoryFontFileLoader (Win 10 1709+).
// 老 Win 10 / Win 7 fallback 暂不做 -- jtUI 主线目标已经定 Win 10+, jtUI 的
// COLR 彩色字形也走的 DWrite 1.2+ API。
#pragma once

#if defined(_WIN32)

#include <dwrite.h>

namespace hui::platform::win32 {

// 在给定的 IDWriteFactory 上注册 codicon 字体。注册成功后 codicon_collection()
// 返回的指针非空, 可以传给 CreateTextFormat 的 fontCollection 参数; family
// 名固定为 hui::foundation::kCodiconFontFamily ("codicon")。
//
// 调用时机: Application D2D / DirectWrite 初始化成功后, 在跑业务渲染前调一次。
// 重复调用幂等。
//
// 返回值: true = 注册成功 (或之前已注册成功);
//        false = factory 不支持 IDWriteFactory5 / blob 加载失败等。失败时
//        codicon_collection() 返回 nullptr, 使用方应当走 fallback (画占位 rect)。
[[nodiscard]] bool codicon_register(IDWriteFactory* factory) noexcept;

// 拿到注册成功后的 collection 指针。未注册 / 注册失败时返回 nullptr。
// 返回的指针由本模块持有, 调用方不需要 Release; 进程退出时由静态析构清理。
[[nodiscard]] IDWriteFontCollection* codicon_collection() noexcept;

}  // namespace hui::platform::win32

#endif  // _WIN32
