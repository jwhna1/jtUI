#pragma once

#include "hui/foundation/color.hpp"

namespace hui::theme {

// 原始色阶梯。dark/light 共用这张表，主题切换只动 SemanticColor 的映射。
// 以 Tailwind 3.x palette 为基准（accent = green_500 = #22C55E 由用户指定，其余
// 阶梯沿 Tailwind 官方色值，方便和前端协同调色）。
struct Palette {
    // 主色：green 阶梯
    Color green_50;
    Color green_100;
    Color green_200;
    Color green_300;
    Color green_400;
    Color green_500;  // accent 主色 #22C55E
    Color green_600;
    Color green_700;
    Color green_800;
    Color green_900;
    Color green_950;

    // 中性灰
    Color neutral_0;  // 纯白
    Color neutral_50;
    Color neutral_100;
    Color neutral_200;
    Color neutral_300;
    Color neutral_400;
    Color neutral_500;
    Color neutral_600;
    Color neutral_700;
    Color neutral_800;
    Color neutral_850;  // 补档，dark surface-alt 需要
    Color neutral_900;
    Color neutral_950;
    Color neutral_1000;  // 纯黑

    // 语义辅助色（按图中右上 palette 面板看到的红/橙/黄/蓝/紫/粉）
    Color red_500;
    Color red_600;
    Color orange_500;
    Color orange_600;
    Color amber_500;
    Color yellow_500;
    Color blue_500;
    Color blue_600;
    Color violet_500;
    Color pink_500;
};

// 标准 Palette（两主题共享）。dark/light 的差异都在 SemanticColor 映射里。
[[nodiscard]] const Palette& default_palette() noexcept;

}  // namespace hui::theme
