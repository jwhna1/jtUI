#pragma once

// jtui_live brand tokens —— jtUI Live 子品牌（热重载）。钢青 accent #38BDF8。
// 双主题 dark / light。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 纯黑底，钢青 accent 给"代码 / 实时"那种工程师冷调；不要蓝紫渐变。
//   - light 加深钢青到 #0284C7 才不在白底飘。
//   - 文字五档；额外加 code_string / code_keyword / code_comment 让代码片段
//     有最小语法高亮风。

#include "jtui/jtui.hpp"

namespace jtui_live::brand {

struct Palette {
    // 背景
    jtui::Color bg_window;
    jtui::Color bg_panel;
    jtui::Color bg_card;
    jtui::Color bg_card_hover;
    jtui::Color bg_field;

    // 边线
    jtui::Color border;
    jtui::Color border_strong;
    jtui::Color divider;

    // 文字
    jtui::Color text_strong;
    jtui::Color text_primary;
    jtui::Color text_secondary;
    jtui::Color text_muted;
    jtui::Color text_disabled;

    // accent 钢青
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;

    // 状态
    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;
    jtui::Color success;     // reload OK 的小标记色

    // 装饰
    jtui::Color grid_line;

    // 代码片段最小语法高亮
    jtui::Color code_string;   // "字符串" 染色
    jtui::Color code_keyword;  // jtui::Button / std::move 这类
    jtui::Color code_comment;  // 注释
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#0A0A0A"),
    .bg_panel      = jtui::Color::from_hex("#0F0F0F"),
    .bg_card       = jtui::Color::from_hex("#141414"),
    .bg_card_hover = jtui::Color::from_hex("#1C1C1C"),
    .bg_field      = jtui::Color::from_hex("#161616"),
    .border        = jtui::Color::from_hex("#1F1F1F"),
    .border_strong = jtui::Color::from_hex("#2A2A2A"),
    .divider       = jtui::Color::from_hex("#1A1A1A"),
    .text_strong   = jtui::Color::from_hex("#FFFFFF"),
    .text_primary  = jtui::Color::from_hex("#F5F5F5"),
    .text_secondary= jtui::Color::from_hex("#A3A3A3"),
    .text_muted    = jtui::Color::from_hex("#737373"),
    .text_disabled = jtui::Color::from_hex("#525252"),
    .accent        = jtui::Color::from_hex("#38BDF8"),
    .accent_hover  = jtui::Color::from_hex("#5DCBFB"),
    .accent_pressed= jtui::Color::from_hex("#1FA0DC"),
    .accent_soft   = jtui::Color::from_hex("#0C2A3A"),
    .accent_fg     = jtui::Color::from_hex("#02141F"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#4CC4FF"),
    .success       = jtui::Color::from_hex("#4ADE80"),
    .grid_line     = jtui::Color{1.0F, 1.0F, 1.0F, 0.025F},
    .code_string   = jtui::Color::from_hex("#84CC16"),  // 绿
    .code_keyword  = jtui::Color::from_hex("#38BDF8"),  // 钢青同 accent
    .code_comment  = jtui::Color::from_hex("#737373"),
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#FAFAF8"),
    .bg_panel      = jtui::Color::from_hex("#FFFFFF"),
    .bg_card       = jtui::Color::from_hex("#F4F4F0"),
    .bg_card_hover = jtui::Color::from_hex("#EAEAE6"),
    .bg_field      = jtui::Color::from_hex("#F0F0EC"),
    .border        = jtui::Color::from_hex("#E5E5E0"),
    .border_strong = jtui::Color::from_hex("#D4D4CF"),
    .divider       = jtui::Color::from_hex("#EDEDE8"),
    .text_strong   = jtui::Color::from_hex("#0A0A0A"),
    .text_primary  = jtui::Color::from_hex("#1F1F1F"),
    .text_secondary= jtui::Color::from_hex("#525252"),
    .text_muted    = jtui::Color::from_hex("#737373"),
    .text_disabled = jtui::Color::from_hex("#A3A3A3"),
    // 浅色下钢青加深到 #0284C7，避免白底刺眼
    .accent        = jtui::Color::from_hex("#0284C7"),
    .accent_hover  = jtui::Color::from_hex("#0EA5E9"),
    .accent_pressed= jtui::Color::from_hex("#0369A1"),
    .accent_soft   = jtui::Color::from_hex("#DCEFFE"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .grid_line     = jtui::Color{0.0F, 0.0F, 0.0F, 0.04F},
    .code_string   = jtui::Color::from_hex("#65A30D"),
    .code_keyword  = jtui::Color::from_hex("#0284C7"),
    .code_comment  = jtui::Color::from_hex("#737373"),
};

[[nodiscard]] inline const Palette& active() noexcept {
    return (jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark) ? dark : light;
}

inline constexpr float radius_xs   = 6.0F;
inline constexpr float radius_sm   = 10.0F;
inline constexpr float radius_md   = 14.0F;
inline constexpr float radius_lg   = 20.0F;
inline constexpr float radius_xl   = 28.0F;
inline constexpr float radius_pill = 9999.0F;

inline constexpr float space_2  = 2.0F;
inline constexpr float space_4  = 4.0F;
inline constexpr float space_8  = 8.0F;
inline constexpr float space_12 = 12.0F;
inline constexpr float space_16 = 16.0F;
inline constexpr float space_20 = 20.0F;
inline constexpr float space_24 = 24.0F;
inline constexpr float space_32 = 32.0F;
inline constexpr float space_40 = 40.0F;

}  // namespace jtui_live::brand
