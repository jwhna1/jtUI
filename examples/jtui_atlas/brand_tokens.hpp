#pragma once

// jtui_atlas brand tokens —— jtUI Atlas 子品牌（observability / BI 数据大屏）。
// 森林深绿 + 黄金边点缀。"Watch every signal." 调性。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 主底 #050A08（绿调黑），accent 提亮到 emerald-500 #10B981 才能让
//     数据可视化在黑底上有信息感；金色 #D4AF37 当点缀（trust/quality 标记）
//     而不是大面积填充，避免黄金渐变烂大街。
//   - light 主底 #F8FAF8（极淡绿白），accent 加深到 forest #064E3B，金色
//     压暗到 #A88726 保持克制。

#include "jtui/jtui.hpp"

namespace jtui_atlas::brand {

struct Palette {
    // 背景层
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

    // accent (森林绿系)
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;

    // 状态
    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;
    jtui::Color success;

    // 黄金点缀（trust badge / KPI 金边）
    jtui::Color gold;
    jtui::Color gold_soft;

    // chart 专用
    jtui::Color chart_line;
    jtui::Color chart_fill;
    jtui::Color chart_grid;
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#050A08"),
    .bg_panel      = jtui::Color::from_hex("#0A0F0D"),
    .bg_card       = jtui::Color::from_hex("#101814"),
    .bg_card_hover = jtui::Color::from_hex("#162420"),
    .bg_field      = jtui::Color::from_hex("#0F1612"),
    .border        = jtui::Color::from_hex("#1B2520"),
    .border_strong = jtui::Color::from_hex("#293530"),
    .divider       = jtui::Color::from_hex("#152019"),
    .text_strong   = jtui::Color::from_hex("#FFFFFF"),
    .text_primary  = jtui::Color::from_hex("#F0F5F2"),
    .text_secondary= jtui::Color::from_hex("#9CB0A6"),
    .text_muted    = jtui::Color::from_hex("#6B8076"),
    .text_disabled = jtui::Color::from_hex("#3F4F47"),
    .accent        = jtui::Color::from_hex("#10B981"),
    .accent_hover  = jtui::Color::from_hex("#34D399"),
    .accent_pressed= jtui::Color::from_hex("#059669"),
    .accent_soft   = jtui::Color::from_hex("#0A2118"),
    .accent_fg     = jtui::Color::from_hex("#050A08"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#6FB8FF"),
    .success       = jtui::Color::from_hex("#22C55E"),
    .gold          = jtui::Color::from_hex("#D4AF37"),
    .gold_soft     = jtui::Color::from_hex("#2A210A"),
    .chart_line    = jtui::Color::from_hex("#10B981"),
    .chart_fill    = jtui::Color{0.063F, 0.725F, 0.506F, 0.18F},  // accent @ 18% alpha
    .chart_grid    = jtui::Color::from_hex("#172620"),
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#F8FAF8"),
    .bg_panel      = jtui::Color::from_hex("#FFFFFF"),
    .bg_card       = jtui::Color::from_hex("#FFFFFF"),
    .bg_card_hover = jtui::Color::from_hex("#F0F5F2"),
    .bg_field      = jtui::Color::from_hex("#F2F6F3"),
    .border        = jtui::Color::from_hex("#E0E8E3"),
    .border_strong = jtui::Color::from_hex("#C7D4CD"),
    .divider       = jtui::Color::from_hex("#EDF1ED"),
    .text_strong   = jtui::Color::from_hex("#050A08"),
    .text_primary  = jtui::Color::from_hex("#0F1612"),
    .text_secondary= jtui::Color::from_hex("#4A5950"),
    .text_muted    = jtui::Color::from_hex("#6B8076"),
    .text_disabled = jtui::Color::from_hex("#A8B5AE"),
    .accent        = jtui::Color::from_hex("#064E3B"),
    .accent_hover  = jtui::Color::from_hex("#047857"),
    .accent_pressed= jtui::Color::from_hex("#022C21"),
    .accent_soft   = jtui::Color::from_hex("#D1FAE5"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .gold          = jtui::Color::from_hex("#A88726"),
    .gold_soft     = jtui::Color::from_hex("#FBF3D9"),
    .chart_line    = jtui::Color::from_hex("#064E3B"),
    .chart_fill    = jtui::Color{0.024F, 0.306F, 0.231F, 0.12F},
    .chart_grid    = jtui::Color::from_hex("#E5ECE8"),
};

[[nodiscard]] inline const Palette& active() noexcept {
    return (jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark) ? dark : light;
}

inline constexpr float radius_xs   = 6.0F;
inline constexpr float radius_sm   = 10.0F;
inline constexpr float radius_md   = 14.0F;
inline constexpr float radius_lg   = 20.0F;
inline constexpr float radius_pill = 9999.0F;

inline constexpr float space_4  = 4.0F;
inline constexpr float space_8  = 8.0F;
inline constexpr float space_12 = 12.0F;
inline constexpr float space_16 = 16.0F;
inline constexpr float space_20 = 20.0F;
inline constexpr float space_24 = 24.0F;
inline constexpr float space_32 = 32.0F;

}  // namespace jtui_atlas::brand
