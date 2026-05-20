#include "hui/theme/theme.hpp"

namespace hui::theme {

// tokens_dark.cpp / tokens_light.cpp 提供
const SemanticColor& dark_semantic_colors() noexcept;
const SemanticColor& light_semantic_colors() noexcept;

namespace {

constexpr Palette make_palette() {
    return Palette {
        .green_50   = Color::from_hex("#F0FDF4"),
        .green_100  = Color::from_hex("#DCFCE7"),
        .green_200  = Color::from_hex("#BBF7D0"),
        .green_300  = Color::from_hex("#86EFAC"),
        .green_400  = Color::from_hex("#4ADE80"),
        .green_500  = Color::from_hex("#22C55E"),
        .green_600  = Color::from_hex("#16A34A"),
        .green_700  = Color::from_hex("#15803D"),
        .green_800  = Color::from_hex("#166534"),
        .green_900  = Color::from_hex("#14532D"),
        .green_950  = Color::from_hex("#052E16"),

        .neutral_0     = Color::from_hex("#FFFFFF"),
        .neutral_50    = Color::from_hex("#FAFAFA"),
        .neutral_100   = Color::from_hex("#F5F5F5"),
        .neutral_200   = Color::from_hex("#E5E5E5"),
        .neutral_300   = Color::from_hex("#D4D4D4"),
        .neutral_400   = Color::from_hex("#A3A3A3"),
        .neutral_500   = Color::from_hex("#737373"),
        .neutral_600   = Color::from_hex("#525252"),
        .neutral_700   = Color::from_hex("#404040"),
        .neutral_800   = Color::from_hex("#262626"),
        .neutral_850   = Color::from_hex("#1F1F1F"),
        .neutral_900   = Color::from_hex("#171717"),
        .neutral_950   = Color::from_hex("#0A0A0A"),
        .neutral_1000  = Color::from_hex("#000000"),

        .red_500    = Color::from_hex("#EF4444"),
        .red_600    = Color::from_hex("#DC2626"),
        .orange_500 = Color::from_hex("#F97316"),
        .orange_600 = Color::from_hex("#EA580C"),
        .amber_500  = Color::from_hex("#F59E0B"),
        .yellow_500 = Color::from_hex("#EAB308"),
        .blue_500   = Color::from_hex("#3B82F6"),
        .blue_600   = Color::from_hex("#2563EB"),
        .violet_500 = Color::from_hex("#8B5CF6"),
        .pink_500   = Color::from_hex("#EC4899"),
    };
}

constexpr SpacingTokens kSpacing {};   // 结构体默认值即定义
constexpr RadiusTokens kRadius {};
constexpr Palette kPalette = make_palette();

constexpr TypographyTokens make_typography() {
    // DirectWrite pt 单位在 96dpi 下 ≈ px。这里直接以 px 表达，绘制端按 1:1 用。
    return TypographyTokens {
        .display  = TextStyle {48.0F, FontWeight::Bold,     1.10F, 0.0F},
        .title    = TextStyle {28.0F, FontWeight::Semibold, 1.25F, 0.0F},
        .heading  = TextStyle {20.0F, FontWeight::Semibold, 1.30F, 0.0F},
        .subtitle = TextStyle {16.0F, FontWeight::Medium,   1.40F, 0.0F},
        .body     = TextStyle {14.0F, FontWeight::Regular,  1.50F, 0.0F},
        .label    = TextStyle {13.0F, FontWeight::Medium,   1.40F, 0.2F},
        .caption  = TextStyle {12.0F, FontWeight::Regular,  1.40F, 0.2F},
        .mono     = TextStyle {14.0F, FontWeight::Regular,  1.40F, 0.0F},
    };
}

constexpr TypographyTokens kTypography = make_typography();

// 单例状态。两张 snapshot 都是 static const，current_mode_ 决定指向哪张。
ThemeMode& current_mode_ref() noexcept {
    static ThemeMode mode = ThemeMode::Dark;
    return mode;
}

Signal<ThemeMode>& changed_signal() noexcept {
    static Signal<ThemeMode> signal;
    return signal;
}

const ThemeSnapshot& dark_snapshot() noexcept {
    static const ThemeSnapshot snap {
        ThemeMode::Dark,
        kPalette,
        dark_semantic_colors(),
        kSpacing,
        kRadius,
        kTypography,
        default_elevation_dark(),
    };
    return snap;
}

const ThemeSnapshot& light_snapshot() noexcept {
    static const ThemeSnapshot snap {
        ThemeMode::Light,
        kPalette,
        light_semantic_colors(),
        kSpacing,
        kRadius,
        kTypography,
        default_elevation_light(),
    };
    return snap;
}

}  // namespace

const Palette& default_palette() noexcept { return kPalette; }
const SpacingTokens& default_spacing() noexcept { return kSpacing; }
const RadiusTokens& default_radius() noexcept { return kRadius; }
const TypographyTokens& default_typography() noexcept { return kTypography; }

const ThemeSnapshot& Theme::current() noexcept {
    return current_mode_ref() == ThemeMode::Dark ? dark_snapshot() : light_snapshot();
}

ThemeMode Theme::mode() noexcept {
    return current_mode_ref();
}

void Theme::set(ThemeMode mode) {
    if (current_mode_ref() == mode) {
        return;
    }
    current_mode_ref() = mode;
    changed_signal().emit(mode);
}

void Theme::toggle() {
    set(current_mode_ref() == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
}

Signal<ThemeMode>& Theme::on_changed() noexcept {
    return changed_signal();
}

Color color_for(Tone tone) noexcept {
    const auto& c = Theme::current().color;
    switch (tone) {
    case Tone::Accent:  return c.accent;
    case Tone::Success: return c.success;
    case Tone::Warning: return c.warning;
    case Tone::Danger:  return c.danger;
    case Tone::Info:    return c.info;
    case Tone::Neutral: return c.text_muted;
    }
    return c.accent;
}

Color hover_color_for(Tone tone) noexcept {
    const auto& c = Theme::current().color;
    switch (tone) {
    case Tone::Accent:  return c.accent_hover;
    case Tone::Danger:  return c.danger_hover;
    // 其余状态色直接返回本色 —— 它们在 v1 没有明确 hover 变体。
    case Tone::Success: return c.success;
    case Tone::Warning: return c.warning;
    case Tone::Info:    return c.info;
    case Tone::Neutral: return c.text_secondary;
    }
    return c.accent_hover;
}

}  // namespace hui::theme
