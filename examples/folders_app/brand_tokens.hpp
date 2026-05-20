#pragma once

// folders_app 品牌 token —— 不污染 jtUI 全局 Theme。
// 双主题：dark / light 两套 Palette；active() 根据 jtui::theme::Theme::mode() 返回。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark：纯黑底 + 浅灰卡 + 荧光绿 accent 点缀。
//   - light 主底象牙白 #FAFAF8，卡片纯白；accent 绿在白底要降亮度
//     用 #15803D 才不刺眼。
//   - 严禁蓝紫渐变（项目规约）。

#include "jtui/jtui.hpp"

namespace folders_app::brand {

struct Palette {
    // 背景层（外到内逐级抬亮 / dark；浅色侧逐级降亮）
    jtui::Color bg_window;
    jtui::Color bg_panel;
    jtui::Color bg_card;
    jtui::Color bg_card_hover;
    jtui::Color bg_field;
    jtui::Color bg_active_soft;  // 侧栏 active 项的微染底

    // 边框 / 分割线
    jtui::Color border;
    jtui::Color border_strong;
    jtui::Color divider;

    // 文字
    jtui::Color text_strong;
    jtui::Color text_primary;
    jtui::Color text_secondary;
    jtui::Color text_muted;
    jtui::Color text_disabled;

    // accent (Folders 荧光绿，dark 亮 / light 深)
    jtui::Color accent;
    jtui::Color accent_hover;
    jtui::Color accent_pressed;
    jtui::Color accent_soft;
    jtui::Color accent_fg;

    // 状态色
    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;

    // 文件夹缩略图：三种"封面"层叠
    jtui::Color folder_body;
    jtui::Color folder_tab;
    jtui::Color folder_thumb;
};

inline constexpr Palette dark = {
    .bg_window      = jtui::Color::from_hex("#0A0A0A"),
    .bg_panel       = jtui::Color::from_hex("#0F0F0F"),
    .bg_card        = jtui::Color::from_hex("#141414"),
    .bg_card_hover  = jtui::Color::from_hex("#1C1C1C"),
    .bg_field       = jtui::Color::from_hex("#161616"),
    .bg_active_soft = jtui::Color::from_hex("#0E2A1C"),
    .border         = jtui::Color::from_hex("#1F1F1F"),
    .border_strong  = jtui::Color::from_hex("#2A2A2A"),
    .divider        = jtui::Color::from_hex("#1A1A1A"),
    .text_strong    = jtui::Color::from_hex("#FFFFFF"),
    .text_primary   = jtui::Color::from_hex("#E8E8E8"),
    .text_secondary = jtui::Color::from_hex("#9A9A9A"),
    .text_muted     = jtui::Color::from_hex("#6A6A6A"),
    .text_disabled  = jtui::Color::from_hex("#4A4A4A"),
    .accent         = jtui::Color::from_hex("#50F58C"),
    .accent_hover   = jtui::Color::from_hex("#6BFCA0"),
    .accent_pressed = jtui::Color::from_hex("#36DC72"),
    .accent_soft    = jtui::Color::from_hex("#143523"),
    .accent_fg      = jtui::Color::from_hex("#062012"),
    .danger         = jtui::Color::from_hex("#FF5757"),
    .warning        = jtui::Color::from_hex("#FFB454"),
    .info           = jtui::Color::from_hex("#4CC4FF"),
    .folder_body    = jtui::Color::from_hex("#1E1E1E"),
    .folder_tab     = jtui::Color::from_hex("#262626"),
    .folder_thumb   = jtui::Color::from_hex("#2E2E2E"),
};

inline constexpr Palette light = {
    .bg_window      = jtui::Color::from_hex("#FAFAF8"),
    .bg_panel       = jtui::Color::from_hex("#FFFFFF"),
    .bg_card        = jtui::Color::from_hex("#F4F4F0"),
    .bg_card_hover  = jtui::Color::from_hex("#EAEAE6"),
    .bg_field       = jtui::Color::from_hex("#F0F0EC"),
    .bg_active_soft = jtui::Color::from_hex("#DCFCE7"),  // 浅薄荷
    .border         = jtui::Color::from_hex("#E5E5E0"),
    .border_strong  = jtui::Color::from_hex("#D4D4CF"),
    .divider        = jtui::Color::from_hex("#EDEDE8"),
    .text_strong    = jtui::Color::from_hex("#0A0A0A"),
    .text_primary   = jtui::Color::from_hex("#1F1F1F"),
    .text_secondary = jtui::Color::from_hex("#525252"),
    .text_muted     = jtui::Color::from_hex("#737373"),
    .text_disabled  = jtui::Color::from_hex("#A3A3A3"),
    // 浅色下绿色降亮加深，避免在白底刺眼
    .accent         = jtui::Color::from_hex("#15803D"),
    .accent_hover   = jtui::Color::from_hex("#16A34A"),
    .accent_pressed = jtui::Color::from_hex("#166534"),
    .accent_soft    = jtui::Color::from_hex("#DCFCE7"),
    .accent_fg      = jtui::Color::from_hex("#FFFFFF"),
    .danger         = jtui::Color::from_hex("#DC2626"),
    .warning        = jtui::Color::from_hex("#D97706"),
    .info           = jtui::Color::from_hex("#0284C7"),
    // 浅色下 folder 三层颜色拉开对比；body 适度加深避免和 bg_card 融化
    .folder_body    = jtui::Color::from_hex("#E2E2DC"),
    .folder_tab     = jtui::Color::from_hex("#C8C8C2"),
    .folder_thumb   = jtui::Color::from_hex("#B0B0AA"),
};

[[nodiscard]] inline const Palette& active() noexcept {
    return (jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark) ? dark : light;
}

// 半径 / 间距 —— 不随主题
inline constexpr float radius_xs   = 6.0F;
inline constexpr float radius_sm   = 10.0F;
inline constexpr float radius_md   = 14.0F;
inline constexpr float radius_lg   = 20.0F;
inline constexpr float radius_xl   = 28.0F;
inline constexpr float radius_pill = 9999.0F;

inline constexpr float space_2  = 2.0F;
inline constexpr float space_4  = 4.0F;
inline constexpr float space_6  = 6.0F;
inline constexpr float space_8  = 8.0F;
inline constexpr float space_12 = 12.0F;
inline constexpr float space_16 = 16.0F;
inline constexpr float space_20 = 20.0F;
inline constexpr float space_24 = 24.0F;
inline constexpr float space_32 = 32.0F;
inline constexpr float space_40 = 40.0F;

}  // namespace folders_app::brand
