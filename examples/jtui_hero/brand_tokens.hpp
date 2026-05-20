#pragma once

// jtui_hero brand tokens —— 力量派 + 暖橘 accent，独立于 jtUI Theme，仅本 example 用。
// 双主题：dark / light 两套 Palette；active() 根据 jtui::theme::Theme::current() 返回。
//
// 维护：曾能混 <jwhna1@gmail.com>
//
// 配色原则：
//   - dark 主底纯黑 #0A0A0A，accent 暖橘 #FB923C；不要任何"蓝紫渐变"。
//   - light 主底象牙白 #FAFAF8，accent 在白底改用更深的橘 #EA580C 才不刺眼。
//   - 文字层级：strong / primary / secondary / muted / disabled 五档够用。

#include "jtui/jtui.hpp"

namespace jtui_hero::brand {

struct Palette {
    // 背景层
    jtui::Color bg_window;
    jtui::Color bg_panel;
    jtui::Color bg_card;
    jtui::Color bg_card_hover;
    jtui::Color bg_field;

    // 边线 / 分隔
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
    jtui::Color accent_soft;   // pill 内染色（与 bg_card 区分）
    jtui::Color accent_fg;     // 盖在 accent 上的文字色

    // 状态色
    jtui::Color danger;
    jtui::Color warning;
    jtui::Color info;

    // 装饰
    jtui::Color grid_line;     // 背景网格线（很淡）

    // 代码块特有：字符串绿 / 注释灰
    jtui::Color code_string;
    jtui::Color code_comment;
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
    .accent        = jtui::Color::from_hex("#FB923C"),
    .accent_hover  = jtui::Color::from_hex("#FDA85F"),
    .accent_pressed= jtui::Color::from_hex("#EA7C1F"),
    .accent_soft   = jtui::Color::from_hex("#3A1F0E"),
    .accent_fg     = jtui::Color::from_hex("#1A0A03"),
    .danger        = jtui::Color::from_hex("#FF5757"),
    .warning       = jtui::Color::from_hex("#FFB454"),
    .info          = jtui::Color::from_hex("#4CC4FF"),
    .grid_line     = jtui::Color{1.0F, 1.0F, 1.0F, 0.025F},
    .code_string   = jtui::Color::from_hex("#84CC16"),
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
    // 浅色下橘色降饱和加深，避免在白底刺眼
    .accent        = jtui::Color::from_hex("#EA580C"),
    .accent_hover  = jtui::Color::from_hex("#F97316"),
    .accent_pressed= jtui::Color::from_hex("#C2410C"),
    .accent_soft   = jtui::Color::from_hex("#FFEDD5"),
    .accent_fg     = jtui::Color::from_hex("#FFFFFF"),
    .danger        = jtui::Color::from_hex("#DC2626"),
    .warning       = jtui::Color::from_hex("#D97706"),
    .info          = jtui::Color::from_hex("#0284C7"),
    .grid_line     = jtui::Color{0.0F, 0.0F, 0.0F, 0.04F},
    .code_string   = jtui::Color::from_hex("#65A30D"),
    .code_comment  = jtui::Color::from_hex("#737373"),
};

// 根据 jtui::theme::Theme 当前模式返回对应 Palette 引用。
// 业务建议：build_* 函数顶部 `const auto& p = brand::active();` 之后用 p.xxx。
[[nodiscard]] inline const Palette& active() noexcept {
    return (jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark) ? dark : light;
}

// 圆角 / spacing —— 不随主题变化
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

}  // namespace jtui_hero::brand
