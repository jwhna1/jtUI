#pragma once

namespace hui::theme {

// 4 的倍数间距阶梯，和 Tailwind/Material 保持一致。
// widget padding/gap 从这里取，避免散落的魔数。
struct SpacingTokens {
    float none {0.0F};
    float xs {4.0F};
    float sm {8.0F};
    float md {12.0F};
    float lg {16.0F};
    float xl {24.0F};
    float xxl {32.0F};
    float xxxl {48.0F};
    float huge {64.0F};
};

[[nodiscard]] const SpacingTokens& default_spacing() noexcept;

}  // namespace hui::theme
