// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#pragma once

#include <array>
#include <string_view>

#include "hui/theme/typography_tokens.hpp"

namespace hui::theme::vscode {

// VS Code 风格 typography 22 级。完整对照 PDF "Typography" spec：
//   Heading 1-6 / Body 1-2 / Label 1-4 / Markdown H1-H6+P / Code 1-2
// 与 runtime/theme/typography_tokens.hpp 中 8 级抽象 TypographyTokens 并存，
// 现有 widget 不强制迁移。新组件 (StatusBar / Tree / Toast) 显式引用本表。

// 等宽字体回退链 (Code 1 / Code 2 引用)。
// 顺序：跨平台开发字体 -> Windows -> macOS -> 通用 monospace。
constexpr std::array<std::string_view, 5> kMonoFallbackChain {{
    "JetBrains Mono",
    "Cascadia Code",
    "Consolas",         // Windows 默认 monospace
    "Menlo",            // macOS / PDF spec 指定
    "monospace",
}};

// UPPERCASE 标记 —— PDF 中 Heading 4 / Heading 5 明确 "Uppercase"。
// TextStyle 不带这个语义，单独枚举出来供 widget 在 layout 阶段决定是否
// 走 to_upper 变换 (Codicons / 中文不应被强制大写，由 widget 自己判断)。
enum class TextTransform : unsigned char {
    None,
    Uppercase,
};

struct VsTextStyle {
    float size;
    FontWeight weight;
    float line_height;
    float letter_spacing;
    TextTransform transform;
};

struct VsTypographyTokens {
    // Heading 系列
    VsTextStyle heading_1;  // 26px Medium       Extension title
    VsTextStyle heading_2;  // 14px Medium       Editor Tabs
    VsTextStyle heading_3;  // 13px Heavy        Title
    VsTextStyle heading_4;  // 11px Bold UPPER   Sidebar Group / Category / Badge
    VsTextStyle heading_5;  // 11px Medium UPPER Sidebar Title / Panel Tabs
    VsTextStyle heading_6;  // 11px Heavy        Debugger list item

    // Body
    VsTextStyle body_1;     // 13px Medium       Body / Input / Ext Desc
    VsTextStyle body_2;     // 12px Medium       Statusbar / Error validation

    // Label
    VsTextStyle label_1;    // 14px Medium       Ext publisher id
    VsTextStyle label_2;    // 12px Bold         Ext Author
    VsTextStyle label_3;    // 11px Medium       Extension version / meta
    VsTextStyle label_4;    // 9px Bold          Search badge

    // Markdown
    VsTextStyle markdown_h1;        // 26px Normal
    VsTextStyle markdown_h2;        // 26px Normal (PDF 标 26，与 H1 同号，靠 widget 加边距区分)
    VsTextStyle markdown_h3;        // 26px Normal
    VsTextStyle markdown_h4;        // 13px Bold
    VsTextStyle markdown_h5;        // 11px Bold
    VsTextStyle markdown_h6;        // 9px Bold
    VsTextStyle markdown_paragraph; // 13px Normal

    // Code (使用 kMonoFallbackChain)
    VsTextStyle code_1;     // 12px Regular Menlo
    VsTextStyle code_2;     // 12px Bold Menlo

    // 图标字号 (Codicons / Seti 字体使用)
    VsTextStyle icon_codicon;  // 16px
    VsTextStyle icon_seti;     // 16px
};

[[nodiscard]] const VsTypographyTokens& typography() noexcept;

}  // namespace hui::theme::vscode
