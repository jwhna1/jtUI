#pragma once

namespace hui::theme {

struct RadiusTokens {
    float none {0.0F};
    float sm {6.0F};   // button / tag
    float md {10.0F};  // input / dropdown
    float lg {14.0F};  // card
    float xl {20.0F};  // dialog / sheet
    float pill {9999.0F};  // switch / badge 胶囊
};

[[nodiscard]] const RadiusTokens& default_radius() noexcept;

}  // namespace hui::theme
