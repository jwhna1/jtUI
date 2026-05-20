#pragma once

// jtui_studio brand tokens —— jtUI Studio 子品牌（桌面原生媒体工作台）。
// 葡萄紫 #7C3AED accent + 深紫黑底，主张 "Build Media-Native. Native FAST."
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 主底 #0A0612（深紫黑），紫调被压到接近黑，accent #7C3AED 在黑底上有
//     力道。卡片底 #16101F 跟主底拉开一层但仍属深紫色谱。
//   - light 主底 #FAF7FD（极淡紫白），accent 加深到 #5B21B6 才不在白底飘。
//   - 媒体类品牌的中性灰带紫调，跟"工作室/调色台"的语义对齐。

#include "jtui/jtui.hpp"

namespace jtui_studio::brand {

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

    // accent (葡萄紫)
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;   // pill 内染色 / 媒体卡片角标底
    jtui::Color accent_fg;     // 盖在 accent 上的文字色

    // 状态
    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;
    jtui::Color success;

    // 媒体专用
    jtui::Color waveform_line;   // 波形主线
    jtui::Color waveform_fill;   // 波形填充（淡）
    jtui::Color media_overlay;   // 视频上的半透明蒙层底
};

inline constexpr Palette dark = {
    .bg_window     = jtui::Color::from_hex("#0A0612"),
    .bg_panel      = jtui::Color::from_hex("#110A1A"),
    .bg_card       = jtui::Color::from_hex("#16101F"),
    .bg_card_hover = jtui::Color::from_hex("#1E162B"),
    .bg_field      = jtui::Color::from_hex("#1A1226"),
    .border        = jtui::Color::from_hex("#221830"),
    .border_strong = jtui::Color::from_hex("#2E2244"),
    .divider       = jtui::Color::from_hex("#1A1226"),
    .text_strong   = jtui::Color::from_hex("#FFFFFF"),
    .text_primary  = jtui::Color::from_hex("#F5F0FB"),
    .text_secondary= jtui::Color::from_hex("#A89BBA"),
    .text_muted    = jtui::Color::from_hex("#8073A0"),
    .text_disabled = jtui::Color::from_hex("#4D4360"),
    .accent        = jtui::Color::from_hex("#7C3AED"),
    .accent_hover  = jtui::Color::from_hex("#9657F0"),
    .accent_pressed= jtui::Color::from_hex("#6929D2"),
    .accent_soft   = jtui::Color::from_hex("#2C1448"),
    .accent_fg     = jtui::Color::from_hex("#FAF7FD"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#6FB8FF"),
    .success       = jtui::Color::from_hex("#4ADE80"),
    .waveform_line = jtui::Color::from_hex("#9657F0"),
    .waveform_fill = jtui::Color{0.486F, 0.227F, 0.929F, 0.25F},  // accent @ 25% alpha
    .media_overlay = jtui::Color{0.039F, 0.024F, 0.071F, 0.55F},  // bg_window @ 55%
};

inline constexpr Palette light = {
    .bg_window     = jtui::Color::from_hex("#FAF7FD"),
    .bg_panel      = jtui::Color::from_hex("#FFFFFF"),
    .bg_card       = jtui::Color::from_hex("#F2EDF7"),
    .bg_card_hover = jtui::Color::from_hex("#E8E0F0"),
    .bg_field      = jtui::Color::from_hex("#EDE5F3"),
    .border        = jtui::Color::from_hex("#E0D6EA"),
    .border_strong = jtui::Color::from_hex("#C7B8D8"),
    .divider       = jtui::Color::from_hex("#EBE3F0"),
    .text_strong   = jtui::Color::from_hex("#0A0612"),
    .text_primary  = jtui::Color::from_hex("#1A1226"),
    .text_secondary= jtui::Color::from_hex("#5A4A75"),
    .text_muted    = jtui::Color::from_hex("#8073A0"),
    .text_disabled = jtui::Color::from_hex("#B8A8C8"),
    .accent        = jtui::Color::from_hex("#5B21B6"),
    .accent_hover  = jtui::Color::from_hex("#7C3AED"),
    .accent_pressed= jtui::Color::from_hex("#4C1D95"),
    .accent_soft   = jtui::Color::from_hex("#EDE5FA"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .success       = jtui::Color::from_hex("#15803D"),
    .waveform_line = jtui::Color::from_hex("#5B21B6"),
    .waveform_fill = jtui::Color{0.357F, 0.129F, 0.714F, 0.18F},  // accent light @ 18% alpha
    .media_overlay = jtui::Color{0.980F, 0.969F, 0.992F, 0.60F},  // bg_window light @ 60%
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

}  // namespace jtui_studio::brand
