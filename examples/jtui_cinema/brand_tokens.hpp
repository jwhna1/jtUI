#pragma once

// jtui_cinema brand tokens —— jtUI Cinema 子品牌（原生影像 carousel）。
// 暖橘 #FB923C accent + 接近全黑底，主张 "World's First Native Cinematic UI."
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 主底 #050505（接近全黑），让影像缩略图作为视觉焦点，UI 退到背景。
//     accent 暖橘在近黑底上有"放映机/胶片"的温度。
//   - light 主底 #FAFAF9（极暖白），accent 加深到 #EA580C 才不刺眼。
//   - 与 jtui_hero / jtui_invest 同色系，保持 jtUI 主品牌视觉连贯。

#include "jtui/jtui.hpp"

namespace jtui_cinema::brand {

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

    // accent (暖橘)
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

    // cinema 专用
    jtui::Color play_ring;        // 视频卡片中央 play 圆环主色
    jtui::Color play_ring_idle;   // 非 active 卡片的 play 圆环（半透）
    jtui::Color watermark;        // 底部 COMING SOON 水印色
    jtui::Color thumb_overlay;    // 视频缩略图上的暗化层
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#050505"),
    .bg_panel      = jtui::Color::from_hex("#0A0A0A"),
    .bg_card       = jtui::Color::from_hex("#0F0F0F"),
    .bg_card_hover = jtui::Color::from_hex("#161616"),
    .bg_field      = jtui::Color::from_hex("#121212"),
    .border        = jtui::Color::from_hex("#1F1F1F"),
    .border_strong = jtui::Color::from_hex("#2A2A2A"),
    .divider       = jtui::Color::from_hex("#161616"),
    .text_strong   = jtui::Color::from_hex("#FFFFFF"),
    .text_primary  = jtui::Color::from_hex("#F5F5F5"),
    .text_secondary= jtui::Color::from_hex("#A8A8A8"),
    .text_muted    = jtui::Color::from_hex("#6B6B6B"),
    .text_disabled = jtui::Color::from_hex("#404040"),
    .accent        = jtui::Color::from_hex("#FB923C"),
    .accent_hover  = jtui::Color::from_hex("#FDA45F"),
    .accent_pressed= jtui::Color::from_hex("#EA8224"),
    .accent_soft   = jtui::Color::from_hex("#3D1F0A"),
    .accent_fg     = jtui::Color::from_hex("#1A0F05"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#6FB8FF"),
    .success       = jtui::Color::from_hex("#4ADE80"),
    .play_ring     = jtui::Color::from_hex("#FB923C"),
    .play_ring_idle= jtui::Color{1.0F, 1.0F, 1.0F, 0.85F},
    // 水印在近全黑 #050505 底上 alpha 0.04 几乎隐形；提到 0.10 让 "COMING SOON"
    // 仍是装饰性弱信号但肉眼能读。light 主题底色淡白，0.05 黑色对比已足够。
    .watermark     = jtui::Color{1.0F, 1.0F, 1.0F, 0.10F},
    .thumb_overlay = jtui::Color{0.0F, 0.0F, 0.0F, 0.35F},
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#FAFAF9"),
    .bg_panel      = jtui::Color::from_hex("#FFFFFF"),
    .bg_card       = jtui::Color::from_hex("#F1F1F0"),
    .bg_card_hover = jtui::Color::from_hex("#E6E6E5"),
    .bg_field      = jtui::Color::from_hex("#EDEDEC"),
    .border        = jtui::Color::from_hex("#E2E2E0"),
    .border_strong = jtui::Color::from_hex("#C7C7C5"),
    .divider       = jtui::Color::from_hex("#E8E8E6"),
    .text_strong   = jtui::Color::from_hex("#0A0A0A"),
    .text_primary  = jtui::Color::from_hex("#1A1A1A"),
    .text_secondary= jtui::Color::from_hex("#525252"),
    .text_muted    = jtui::Color::from_hex("#7A7A7A"),
    .text_disabled = jtui::Color::from_hex("#A8A8A8"),
    .accent        = jtui::Color::from_hex("#EA580C"),
    .accent_hover  = jtui::Color::from_hex("#FB923C"),
    .accent_pressed= jtui::Color::from_hex("#C2410C"),
    .accent_soft   = jtui::Color::from_hex("#FFEDD5"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .play_ring     = jtui::Color::from_hex("#EA580C"),
    .play_ring_idle= jtui::Color{1.0F, 1.0F, 1.0F, 0.92F},
    .watermark     = jtui::Color{0.0F, 0.0F, 0.0F, 0.05F},
    .thumb_overlay = jtui::Color{0.0F, 0.0F, 0.0F, 0.25F},
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

}  // namespace jtui_cinema::brand
