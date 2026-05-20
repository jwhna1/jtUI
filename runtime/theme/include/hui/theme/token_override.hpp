#pragma once

#include <optional>

#include "hui/foundation/color.hpp"
#include "hui/theme/semantic_colors.hpp"

namespace hui::theme {

// 组件级 token override（spec §13.4）。
//
// SemanticColor 的 partial 视图，每字段 std::optional<Color>，"set 了就用 override，
// 没 set 就走 base"。Widget 持有 std::unique_ptr<TokenOverride>，默认 nullptr 不
// 增加内存（每个 widget 仅 +8 bytes 指针），需要 override 时再 lazy 分配。
//
// 用法：
//   widget->set_token_override(theme::TokenOverride{}.with_accent(Color{...}));
//   // widget 自己 paint 时调 theme::resolve(this).accent 拿到 override 后的色
//
// 对旧 widget 兼容：旧 widget 不需要改 paint。直接 theme::colors() 仍返回 base，
// 只在 widget 主动迁到 theme::resolve(this) 之后才能感知 override。

struct TokenOverride {
    std::optional<Color> bg_base;
    std::optional<Color> bg_surface;
    std::optional<Color> bg_surface_alt;
    std::optional<Color> bg_raised;
    std::optional<Color> bg_overlay;

    std::optional<Color> text_primary;
    std::optional<Color> text_secondary;
    std::optional<Color> text_muted;
    std::optional<Color> text_disabled;

    std::optional<Color> border;
    std::optional<Color> border_strong;
    std::optional<Color> divider;

    std::optional<Color> accent;
    std::optional<Color> accent_hover;
    std::optional<Color> accent_pressed;
    std::optional<Color> accent_soft;
    std::optional<Color> accent_fg;

    std::optional<Color> success;
    std::optional<Color> warning;
    std::optional<Color> danger;
    std::optional<Color> info;

    std::optional<Color> track;
    std::optional<Color> track_soft;

    // 链式 builder 风格 setter（让 widget->set_token_override(... .with_accent(c)) 一行完成）
    TokenOverride& with_accent(Color c) {
        accent = c;
        return *this;
    }
    TokenOverride& with_accent_fg(Color c) {
        accent_fg = c;
        return *this;
    }
    TokenOverride& with_text_primary(Color c) {
        text_primary = c;
        return *this;
    }
    TokenOverride& with_bg_surface(Color c) {
        bg_surface = c;
        return *this;
    }
    TokenOverride& with_track(Color c) {
        track = c;
        return *this;
    }
};

// 在 base SemanticColor 上叠加 override，返回新 SemanticColor 拷贝。
// override == nullptr 时直接拷一份 base 返回（语义干净，性能上一次拷贝）。
[[nodiscard]] inline SemanticColor merge_override(const SemanticColor& base,
                                                  const TokenOverride* override) noexcept {
    SemanticColor merged = base;
    if (override == nullptr) {
        return merged;
    }
#define HUI_APPLY(field) if (override->field.has_value()) merged.field = *override->field
    HUI_APPLY(bg_base);
    HUI_APPLY(bg_surface);
    HUI_APPLY(bg_surface_alt);
    HUI_APPLY(bg_raised);
    HUI_APPLY(bg_overlay);
    HUI_APPLY(text_primary);
    HUI_APPLY(text_secondary);
    HUI_APPLY(text_muted);
    HUI_APPLY(text_disabled);
    HUI_APPLY(border);
    HUI_APPLY(border_strong);
    HUI_APPLY(divider);
    HUI_APPLY(accent);
    HUI_APPLY(accent_hover);
    HUI_APPLY(accent_pressed);
    HUI_APPLY(accent_soft);
    HUI_APPLY(accent_fg);
    HUI_APPLY(success);
    HUI_APPLY(warning);
    HUI_APPLY(danger);
    HUI_APPLY(info);
    HUI_APPLY(track);
    HUI_APPLY(track_soft);
#undef HUI_APPLY
    return merged;
}

}  // namespace hui::theme
