#pragma once

#include "hui/foundation/color.hpp"
#include "hui/property/signal.hpp"
#include "hui/theme/color_tokens.hpp"
#include "hui/theme/elevation_tokens.hpp"
#include "hui/theme/radius_tokens.hpp"
#include "hui/theme/semantic_colors.hpp"
#include "hui/theme/spacing_tokens.hpp"
#include "hui/theme/token_override.hpp"
#include "hui/theme/typography_tokens.hpp"

namespace hui::theme {

enum class ThemeMode {
    Dark,
    Light,
};

// 通用语义色调标签。widget 需要突出"这是一个警告指标 / 这是一张成功卡片"时，
// 用 Tone 选 semantic 色，而不是接入每种色的独立 setter。
enum class Tone {
    Accent,
    Success,
    Warning,
    Danger,
    Info,
    Neutral,
};

[[nodiscard]] Color color_for(Tone tone) noexcept;
[[nodiscard]] Color hover_color_for(Tone tone) noexcept;

// 一份完整的主题快照。运行时保持两份静态实例（dark/light），Theme::current()
// 指向当前活跃那份。widget paint 时按 tag 查 semantic 色，不要持有快照指针，
// 切主题后下一帧自动读到新值。
struct ThemeSnapshot {
    ThemeMode mode;
    const Palette& palette;
    const SemanticColor& color;
    const SpacingTokens& spacing;
    const RadiusTokens& radius;
    const TypographyTokens& typography;
    const ElevationTokens& elevation;
};

// 全局单例。框架定位决定：不做 per-Window 主题，切了就全局翻。
class Theme {
public:
    [[nodiscard]] static const ThemeSnapshot& current() noexcept;
    [[nodiscard]] static ThemeMode mode() noexcept;
    static void set(ThemeMode mode);
    static void toggle();

    // 参数为切换后的新 mode。widget / app 层挂 slot 做 invalidate。
    [[nodiscard]] static Signal<ThemeMode>& on_changed() noexcept;

    Theme() = delete;
};

// 便捷访问，避免到处写 Theme::current().color.xxx
[[nodiscard]] inline const SemanticColor& colors() noexcept {
    return Theme::current().color;
}
[[nodiscard]] inline const Palette& palette() noexcept {
    return Theme::current().palette;
}
[[nodiscard]] inline const SpacingTokens& spacing() noexcept {
    return Theme::current().spacing;
}
[[nodiscard]] inline const RadiusTokens& radius() noexcept {
    return Theme::current().radius;
}
[[nodiscard]] inline const TypographyTokens& typography() noexcept {
    return Theme::current().typography;
}
[[nodiscard]] inline const ElevationTokens& elevation() noexcept {
    return Theme::current().elevation;
}

// 把全局 base SemanticColor 与一个 widget-local TokenOverride 合并，返回
// SemanticColor 拷贝。widget paint 时调 theme::resolve(token_override_.get())
// 替代 theme::colors() 即可启用组件级 override（spec §13.4）。
// override == nullptr 直接拷一份当前 base 返回。
[[nodiscard]] inline SemanticColor resolve(const TokenOverride* override) noexcept {
    return merge_override(colors(), override);
}

}  // namespace hui::theme
