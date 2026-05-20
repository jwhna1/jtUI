#include "hui/theme/elevation_tokens.hpp"
#include "hui/theme/semantic_colors.hpp"

namespace hui::theme {

namespace {

constexpr SemanticColor make_light() {
    return SemanticColor {
        .bg_base           = Color::from_hex("#FAFAFA"),  // neutral_50
        .bg_surface        = Color::from_hex("#FFFFFF"),
        .bg_surface_alt    = Color::from_hex("#F5F5F5"),  // neutral_100
        .bg_raised         = Color::from_hex("#FFFFFF"),
        .bg_overlay        = Color::from_rgba8(0U, 0U, 0U, 102U),  // 0.4 alpha

        .text_primary      = Color::from_hex("#171717"),  // neutral_900
        .text_secondary    = Color::from_hex("#525252"),  // neutral_600
        .text_muted        = Color::from_hex("#737373"),  // neutral_500
        .text_disabled     = Color::from_hex("#A3A3A3"),  // neutral_400
        .text_inverse      = Color::from_hex("#FFFFFF"),

        .border            = Color::from_hex("#E5E5E5"),  // neutral_200
        .border_strong     = Color::from_hex("#D4D4D4"),  // neutral_300
        .divider           = Color::from_hex("#E5E5E5"),

        .field_bg          = Color::from_hex("#FFFFFF"),
        .field_bg_hover    = Color::from_hex("#FAFAFA"),
        .field_bg_active   = Color::from_hex("#FFFFFF"),
        .field_border      = Color::from_hex("#D4D4D4"),
        .field_border_focus = Color::from_hex("#22C55E"),

        .accent            = Color::from_hex("#22C55E"),  // green_500 ← 用户指定
        .accent_hover      = Color::from_hex("#16A34A"),  // green_600，light 下 hover 往深走
        .accent_pressed    = Color::from_hex("#15803D"),  // green_700
        .accent_soft       = Color::from_hex("#DCFCE7"),  // green_100 淡底
        .accent_fg         = Color::from_hex("#FFFFFF"),

        .success           = Color::from_hex("#16A34A"),
        .warning           = Color::from_hex("#F59E0B"),
        .danger            = Color::from_hex("#EF4444"),
        .danger_hover      = Color::from_hex("#DC2626"),
        .info              = Color::from_hex("#2563EB"),  // blue_600

        .track             = Color::from_hex("#E5E5E5"),
        .track_soft        = Color::from_hex("#F5F5F5"),
    };
}

constexpr ElevationTokens make_light_elevation() {
    // Light 下阴影柔和，alpha 小。
    const Color soft = Color::from_rgba8(17U, 24U, 39U, 20U);   // ~0.08
    const Color medium = Color::from_rgba8(17U, 24U, 39U, 38U); // ~0.15
    const Color strong = Color::from_rgba8(17U, 24U, 39U, 51U); // ~0.2
    return ElevationTokens {
        .level_0 = Shadow {0.0F, 0.0F, 0.0F, 0.0F, Color::from_rgba8(0U, 0U, 0U, 0U)},
        .level_1 = Shadow {0.0F, 1.0F, 2.0F, 0.0F, soft},
        .level_2 = Shadow {0.0F, 4.0F, 12.0F, 0.0F, soft},
        .level_3 = Shadow {0.0F, 8.0F, 24.0F, -2.0F, medium},
        .level_4 = Shadow {0.0F, 16.0F, 40.0F, -4.0F, strong},
    };
}

constexpr SemanticColor kLight = make_light();
constexpr ElevationTokens kLightElevation = make_light_elevation();

}  // namespace

const SemanticColor& light_semantic_colors() noexcept {
    return kLight;
}

const ElevationTokens& default_elevation_light() noexcept {
    return kLightElevation;
}

}  // namespace hui::theme
