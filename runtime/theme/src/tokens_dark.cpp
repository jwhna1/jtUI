#include "hui/theme/elevation_tokens.hpp"
#include "hui/theme/semantic_colors.hpp"

namespace hui::theme {

namespace {

constexpr SemanticColor make_dark() {
    // Palette 对应 Tailwind green/neutral 阶梯（见 theme.cpp）。
    // Dark 主题背景从 neutral_950 逐级上浮到 neutral_800。
    return SemanticColor {
        .bg_base           = Color::from_hex("#0A0A0A"),  // neutral_950
        .bg_surface        = Color::from_hex("#171717"),  // neutral_900
        .bg_surface_alt    = Color::from_hex("#1F1F1F"),  // neutral_850
        .bg_raised         = Color::from_hex("#262626"),  // neutral_800
        .bg_overlay        = Color::from_rgba8(0U, 0U, 0U, 153U),  // 0.6 alpha

        .text_primary      = Color::from_hex("#FAFAFA"),  // neutral_50
        .text_secondary    = Color::from_hex("#A3A3A3"),  // neutral_400
        .text_muted        = Color::from_hex("#737373"),  // neutral_500
        .text_disabled     = Color::from_hex("#525252"),  // neutral_600
        .text_inverse      = Color::from_hex("#FFFFFF"),

        .border            = Color::from_hex("#262626"),  // neutral_800
        .border_strong     = Color::from_hex("#404040"),  // neutral_700
        .divider           = Color::from_hex("#1F1F1F"),  // neutral_850

        .field_bg          = Color::from_hex("#171717"),  // neutral_900
        .field_bg_hover    = Color::from_hex("#1F1F1F"),
        .field_bg_active   = Color::from_hex("#262626"),
        .field_border      = Color::from_hex("#404040"),  // neutral_700
        .field_border_focus = Color::from_hex("#22C55E"), // green_500

        .accent            = Color::from_hex("#22C55E"),  // green_500 ← 用户指定
        .accent_hover      = Color::from_hex("#4ADE80"),  // green_400，dark 下 hover 往亮走
        .accent_pressed    = Color::from_hex("#16A34A"),  // green_600
        .accent_soft       = Color::from_hex("#14532D"),  // green_900 淡底
        .accent_fg         = Color::from_hex("#FFFFFF"),

        .success           = Color::from_hex("#22C55E"),
        .warning           = Color::from_hex("#F59E0B"),  // amber_500
        .danger            = Color::from_hex("#EF4444"),  // red_500
        .danger_hover      = Color::from_hex("#DC2626"),  // red_600
        .info              = Color::from_hex("#3B82F6"),  // blue_500

        .track             = Color::from_hex("#262626"),
        .track_soft        = Color::from_hex("#1F1F1F"),
    };
}

constexpr ElevationTokens make_dark_elevation() {
    // Dark 下阴影颜色更深、alpha 更高，才能"压出来"。
    const Color shadow_color = Color::from_rgba8(0U, 0U, 0U, 153U);   // 0.6
    const Color strong_shadow = Color::from_rgba8(0U, 0U, 0U, 179U);  // 0.7
    return ElevationTokens {
        .level_0 = Shadow {0.0F, 0.0F, 0.0F, 0.0F, Color::from_rgba8(0U, 0U, 0U, 0U)},
        .level_1 = Shadow {0.0F, 1.0F, 2.0F, 0.0F, shadow_color},
        .level_2 = Shadow {0.0F, 4.0F, 8.0F, 0.0F, shadow_color},
        .level_3 = Shadow {0.0F, 8.0F, 16.0F, -2.0F, strong_shadow},
        .level_4 = Shadow {0.0F, 16.0F, 32.0F, -4.0F, strong_shadow},
    };
}

constexpr SemanticColor kDark = make_dark();
constexpr ElevationTokens kDarkElevation = make_dark_elevation();

}  // namespace

const SemanticColor& dark_semantic_colors() noexcept {
    return kDark;
}

const ElevationTokens& default_elevation_dark() noexcept {
    return kDarkElevation;
}

}  // namespace hui::theme
