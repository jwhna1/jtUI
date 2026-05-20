#pragma once

// jtui_pro brand tokens —— jtUI Pro 子品牌（企业版）。水青黄金 accent #2DD4BF。
// 双主题 dark / light。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 沿黑底，水青是带黄金感的青绿，给企业版"贵气"调；不要蓝紫渐变。
//   - light 加深水青到 #0F766E 才不在白底飘。
//   - 多了 trust_badge_bg（trust 徽记墙的小标签底色）

#include "jtui/jtui.hpp"

namespace jtui_pro::brand {

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

    // 装饰
    jtui::Color stream_line;     // 顶部流线 bezier 颜色（半透明 accent）
    jtui::Color trust_bg;        // trust 徽记墙的小标签底
    jtui::Color trust_text;      // trust 徽记墙的文字
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
    .accent        = jtui::Color::from_hex("#2DD4BF"),
    .accent_hover  = jtui::Color::from_hex("#5EEAD4"),
    .accent_pressed= jtui::Color::from_hex("#14B8A6"),
    .accent_soft   = jtui::Color::from_hex("#0F2A26"),
    .accent_fg     = jtui::Color::from_hex("#02141F"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#4CC4FF"),
    .success       = jtui::Color::from_hex("#4ADE80"),
    .stream_line   = jtui::Color{0.176F, 0.831F, 0.749F, 0.30F},  // accent @ 30% alpha
    .trust_bg      = jtui::Color::from_hex("#141414"),
    .trust_text    = jtui::Color::from_hex("#A3A3A3"),
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
    // light 下水青加深到 #0F766E 不刺眼
    .accent        = jtui::Color::from_hex("#0F766E"),
    .accent_hover  = jtui::Color::from_hex("#14B8A6"),
    .accent_pressed= jtui::Color::from_hex("#115E59"),
    .accent_soft   = jtui::Color::from_hex("#CCFBF1"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .stream_line   = jtui::Color{0.059F, 0.463F, 0.431F, 0.18F},  // accent @ 18% alpha
    .trust_bg      = jtui::Color::from_hex("#F4F4F0"),
    .trust_text    = jtui::Color::from_hex("#525252"),
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

}  // namespace jtui_pro::brand
