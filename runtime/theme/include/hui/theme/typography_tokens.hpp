#pragma once

#include <array>
#include <string_view>

namespace hui::theme {

// 字体回退链。硬编码，用户不可配置（v1 框架定位：不做用户侧 config 层）。
// 顺序：中文优先字体 → 中文备选 → 商用备选 → 系统字体。
// DirectWrite IDWriteFontFallback 会按这个顺序解析；Linux 本地构建读不到这些
// 字体也不影响，ctest 跑 platform-neutral 路径。
constexpr std::array<std::string_view, 6> kFontFallbackChain {{
    "Source Han Sans CN",      // 思源黑体 CN
    "HarmonyOS Sans SC",       // HarmonyOS Sans 简中
    "Alibaba PuHuiTi 3.0",     // 阿里巴巴普惠体 3.0
    "Microsoft YaHei UI",      // Windows 中文回退
    "Segoe UI",                // Windows 英文回退
    "sans-serif",              // 最后一档逻辑别名
}};

// 字重（DirectWrite 数值体系：100-900）
enum class FontWeight : int {
    Regular = 400,
    Medium = 500,
    Semibold = 600,
    Bold = 700,
};

// 单个 text style。size 单位 px，line_height 是倍率（1.4 = 140%）。
struct TextStyle {
    float size;
    FontWeight weight;
    float line_height;
    float letter_spacing;  // px
};

struct TypographyTokens {
    TextStyle display;   // 超大标题（dashboard 大数字）
    TextStyle title;     // 页面标题
    TextStyle heading;   // 卡片标题
    TextStyle subtitle;  // 副标题
    TextStyle body;      // 正文
    TextStyle label;     // 按钮/输入标签
    TextStyle caption;   // 辅助说明
    TextStyle mono;      // 数字/代码等宽位
};

[[nodiscard]] const TypographyTokens& default_typography() noexcept;

}  // namespace hui::theme
