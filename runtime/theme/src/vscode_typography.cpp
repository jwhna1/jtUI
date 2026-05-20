// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#include "hui/theme/vscode_typography.hpp"

namespace hui::theme::vscode {

namespace {

constexpr VsTypographyTokens make_typography() {
    return VsTypographyTokens {
        // Heading
        .heading_1 = VsTextStyle {26.0F, FontWeight::Medium,   1.20F, 0.0F, TextTransform::None},
        .heading_2 = VsTextStyle {14.0F, FontWeight::Medium,   1.35F, 0.0F, TextTransform::None},
        .heading_3 = VsTextStyle {13.0F, FontWeight::Bold,     1.30F, 0.0F, TextTransform::None},
        .heading_4 = VsTextStyle {11.0F, FontWeight::Bold,     1.30F, 0.4F, TextTransform::Uppercase},
        .heading_5 = VsTextStyle {11.0F, FontWeight::Medium,   1.30F, 0.4F, TextTransform::Uppercase},
        .heading_6 = VsTextStyle {11.0F, FontWeight::Bold,     1.30F, 0.0F, TextTransform::None},

        // Body
        .body_1    = VsTextStyle {13.0F, FontWeight::Medium,   1.40F, 0.0F, TextTransform::None},
        .body_2    = VsTextStyle {12.0F, FontWeight::Medium,   1.35F, 0.0F, TextTransform::None},

        // Label
        .label_1   = VsTextStyle {14.0F, FontWeight::Medium,   1.35F, 0.0F, TextTransform::None},
        .label_2   = VsTextStyle {12.0F, FontWeight::Bold,     1.30F, 0.0F, TextTransform::None},
        .label_3   = VsTextStyle {11.0F, FontWeight::Medium,   1.30F, 0.2F, TextTransform::None},
        .label_4   = VsTextStyle { 9.0F, FontWeight::Bold,     1.20F, 0.2F, TextTransform::None},

        // Markdown
        .markdown_h1        = VsTextStyle {26.0F, FontWeight::Regular, 1.25F, 0.0F, TextTransform::None},
        .markdown_h2        = VsTextStyle {26.0F, FontWeight::Regular, 1.25F, 0.0F, TextTransform::None},
        .markdown_h3        = VsTextStyle {26.0F, FontWeight::Regular, 1.25F, 0.0F, TextTransform::None},
        .markdown_h4        = VsTextStyle {13.0F, FontWeight::Bold,    1.40F, 0.0F, TextTransform::None},
        .markdown_h5        = VsTextStyle {11.0F, FontWeight::Bold,    1.30F, 0.0F, TextTransform::None},
        .markdown_h6        = VsTextStyle { 9.0F, FontWeight::Bold,    1.20F, 0.0F, TextTransform::None},
        .markdown_paragraph = VsTextStyle {13.0F, FontWeight::Regular, 1.55F, 0.0F, TextTransform::None},

        // Code (走 kMonoFallbackChain)
        .code_1    = VsTextStyle {12.0F, FontWeight::Regular,  1.45F, 0.0F, TextTransform::None},
        .code_2    = VsTextStyle {12.0F, FontWeight::Bold,     1.45F, 0.0F, TextTransform::None},

        // Icons
        .icon_codicon = VsTextStyle {16.0F, FontWeight::Regular, 1.0F, 0.0F, TextTransform::None},
        .icon_seti    = VsTextStyle {16.0F, FontWeight::Regular, 1.0F, 0.0F, TextTransform::None},
    };
}

constexpr VsTypographyTokens kTypography = make_typography();

}  // namespace

const VsTypographyTokens& typography() noexcept {
    return kTypography;
}

}  // namespace hui::theme::vscode
