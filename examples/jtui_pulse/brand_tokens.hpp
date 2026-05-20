#pragma once

// jtui_pulse brand tokens —— jtUI Pulse 子品牌（实时监控）。珊瑚红 accent #FB7185。
// 双主题 dark / light。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 沿黑底，珊瑚红给"告警 / pulse"那种实时感；不要蓝紫渐变。
//   - light 加深珊瑚红到 #E11D48 才不在白底飘。
//   - 多了 chart_line / chart_fill 给 live chart 装饰用。

#include "jtui/jtui.hpp"

namespace jtui_pulse::brand {

struct Palette {
    jtui::Color bg_window;
    jtui::Color bg_panel;
    jtui::Color bg_card;
    jtui::Color bg_card_hover;
    jtui::Color bg_field;

    jtui::Color border;
    jtui::Color border_strong;
    jtui::Color divider;

    jtui::Color text_strong;
    jtui::Color text_primary;
    jtui::Color text_secondary;
    jtui::Color text_muted;
    jtui::Color text_disabled;

    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;

    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;
    jtui::Color success;

    jtui::Color star;          // 5 星评分用（金黄）
    jtui::Color chart_line;    // live chart 折线（accent 同色但单独命名便于调）
    jtui::Color chart_fill;    // 折线下方阴影（accent 半透明）
    jtui::Color chart_grid;    // chart 网格线（很淡）
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
    .accent        = jtui::Color::from_hex("#FB7185"),
    .accent_hover  = jtui::Color::from_hex("#FDA4AF"),
    .accent_pressed= jtui::Color::from_hex("#E11D48"),
    .accent_soft   = jtui::Color::from_hex("#3A1019"),
    .accent_fg     = jtui::Color::from_hex("#1A0509"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#4CC4FF"),
    .success       = jtui::Color::from_hex("#4ADE80"),
    .star          = jtui::Color::from_hex("#FBBF24"),
    .chart_line    = jtui::Color::from_hex("#FB7185"),
    .chart_fill    = jtui::Color{0.984F, 0.443F, 0.522F, 0.20F},  // accent @ 20%
    .chart_grid    = jtui::Color{1.0F, 1.0F, 1.0F, 0.04F},
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
    // light 下珊瑚红加深到 #E11D48 不刺眼
    .accent        = jtui::Color::from_hex("#E11D48"),
    .accent_hover  = jtui::Color::from_hex("#F43F5E"),
    .accent_pressed= jtui::Color::from_hex("#BE123C"),
    .accent_soft   = jtui::Color::from_hex("#FFE4E6"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .star          = jtui::Color::from_hex("#D97706"),
    .chart_line    = jtui::Color::from_hex("#E11D48"),
    .chart_fill    = jtui::Color{0.882F, 0.114F, 0.282F, 0.12F},  // accent @ 12%
    .chart_grid    = jtui::Color{0.0F, 0.0F, 0.0F, 0.05F},
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

}  // namespace jtui_pulse::brand
