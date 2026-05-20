#pragma once

// jtui_invest brand tokens —— jtUI Invest 金融投资子品牌（视觉范式借鉴 桌面UI.pdf p20）。
// 近黑深底 + 酸橙绿 #C5F82A accent；涨绿 / 跌红双向语义色。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - PDF 原稿是深色 dashboard，dark 为主品牌形态：近黑 #0A0B0D 主底，
//     dashboard 大面板 / 子卡片逐层加亮一点点拉开层次。
//   - 酸橙绿 #C5F82A 在黑底极亮、信息感强；light 主题下这个色在白底几乎
//     不可读，accent 压到深橄榄 #5C7A0F。
//   - 金融语义：up（涨）翡翠绿、down（跌）珊瑚红，各配 soft 底给 chip。

#include "jtui/jtui.hpp"

namespace jtui_invest::brand {

struct Palette {
    // 背景层
    jtui::Color bg_window;
    jtui::Color bg_panel;        // dashboard 大面板
    jtui::Color bg_card;         // 子卡片
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

    // accent（酸橙绿）
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;

    // 金融双向语义
    jtui::Color up;          // 涨
    jtui::Color up_soft;     // 涨 chip 底
    jtui::Color down;        // 跌
    jtui::Color down_soft;   // 跌 chip 底

    // 毛玻璃 navbar tint（叠在模糊层之上的半透明色）
    jtui::Color glass_tint;

    // chart
    jtui::Color chart_line;
    jtui::Color chart_glow;  // 折线下方辉光填充
    jtui::Color chart_grid;
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#0A0B0D"),
    .bg_panel      = jtui::Color::from_hex("#121316"),
    .bg_card       = jtui::Color::from_hex("#16181C"),
    .bg_card_hover = jtui::Color::from_hex("#1E2126"),
    .bg_field      = jtui::Color::from_hex("#1A1C20"),
    .border        = jtui::Color::from_hex("#24262B"),
    .border_strong = jtui::Color::from_hex("#32353C"),
    .divider       = jtui::Color::from_hex("#1F2126"),
    .text_strong   = jtui::Color::from_hex("#FFFFFF"),
    .text_primary  = jtui::Color::from_hex("#E8E9EC"),
    .text_secondary= jtui::Color::from_hex("#9A9CA3"),
    .text_muted    = jtui::Color::from_hex("#6B6D75"),
    .text_disabled = jtui::Color::from_hex("#44464D"),
    .accent        = jtui::Color::from_hex("#C5F82A"),
    .accent_hover  = jtui::Color::from_hex("#D2FF4D"),
    .accent_pressed= jtui::Color::from_hex("#A8D919"),
    .accent_soft   = jtui::Color::from_hex("#27300C"),
    .accent_fg     = jtui::Color::from_hex("#0A0B0D"),
    .up            = jtui::Color::from_hex("#34D399"),
    .up_soft       = jtui::Color::from_hex("#0E2D22"),
    .down          = jtui::Color::from_hex("#F87171"),
    .down_soft     = jtui::Color::from_hex("#2D1416"),
    .glass_tint    = jtui::Color{0.058F, 0.063F, 0.074F, 0.78F},  // 加浓磨砂质感
    .chart_line    = jtui::Color::from_hex("#C5F82A"),
    .chart_glow    = jtui::Color{0.773F, 0.973F, 0.165F, 0.16F},  // accent @ 16%
    .chart_grid    = jtui::Color::from_hex("#22242A"),
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#F5F6F8"),
    .bg_panel      = jtui::Color::from_hex("#FFFFFF"),
    .bg_card       = jtui::Color::from_hex("#FAFBFC"),
    .bg_card_hover = jtui::Color::from_hex("#F0F2F5"),
    .bg_field      = jtui::Color::from_hex("#EEF0F3"),
    .border        = jtui::Color::from_hex("#E2E4E9"),
    .border_strong = jtui::Color::from_hex("#CDD0D7"),
    .divider       = jtui::Color::from_hex("#ECEEF1"),
    .text_strong   = jtui::Color::from_hex("#0A0B0D"),
    .text_primary  = jtui::Color::from_hex("#1C1E22"),
    .text_secondary= jtui::Color::from_hex("#55585F"),
    .text_muted    = jtui::Color::from_hex("#878A92"),
    .text_disabled = jtui::Color::from_hex("#B4B7BE"),
    // 酸橙绿在白底不可读 → 压到深橄榄
    .accent        = jtui::Color::from_hex("#5C7A0F"),
    .accent_hover  = jtui::Color::from_hex("#6F941A"),
    .accent_pressed= jtui::Color::from_hex("#496111"),
    .accent_soft   = jtui::Color::from_hex("#EAF4C8"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .up            = jtui::Color::from_hex("#15803D"),
    .up_soft       = jtui::Color::from_hex("#DCFCE7"),
    .down          = jtui::Color::from_hex("#DC2626"),
    .down_soft     = jtui::Color::from_hex("#FEE2E2"),
    .glass_tint    = jtui::Color{0.978F, 0.981F, 0.987F, 0.80F},  // 加浓磨砂质感
    .chart_line    = jtui::Color::from_hex("#5C7A0F"),
    .chart_glow    = jtui::Color{0.361F, 0.478F, 0.059F, 0.12F},
    .chart_grid    = jtui::Color::from_hex("#E8EAEE"),
};

[[nodiscard]] inline const Palette& active() noexcept {
    return (jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark) ? dark : light;
}

inline constexpr float radius_xs   = 8.0F;
inline constexpr float radius_sm   = 12.0F;
inline constexpr float radius_md   = 16.0F;
inline constexpr float radius_lg   = 24.0F;
inline constexpr float radius_pill = 9999.0F;

inline constexpr float space_4  = 4.0F;
inline constexpr float space_8  = 8.0F;
inline constexpr float space_12 = 12.0F;
inline constexpr float space_16 = 16.0F;
inline constexpr float space_20 = 20.0F;
inline constexpr float space_24 = 24.0F;
inline constexpr float space_32 = 32.0F;
inline constexpr float space_40 = 40.0F;

}  // namespace jtui_invest::brand
