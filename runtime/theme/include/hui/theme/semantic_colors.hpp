#pragma once

#include "hui/foundation/color.hpp"

namespace hui::theme {

// 语义色：widget 只关心这些 tag，不直接碰 Palette 里的色阶。
// dark/light 两张 SemanticColor 实例走不同的 Palette 阶梯映射。
struct SemanticColor {
    // 背景层级（从窗口背景到最上层卡片）
    Color bg_base;        // 整窗背景
    Color bg_surface;     // 一级面板
    Color bg_surface_alt; // 二级面板 / 卡片里的分区
    Color bg_raised;      // 卡片 / Dialog / Popover
    Color bg_overlay;     // 模态半透明遮罩

    // 文本
    Color text_primary;   // 标题 / 正文主色
    Color text_secondary; // 副文本
    Color text_muted;     // 说明文字 / helper
    Color text_disabled;  // 禁用态文字
    Color text_inverse;   // 放在 accent 实色上的文字（通常是白）

    // 边框 / 分隔线
    Color border;
    Color border_strong;  // 强分隔（input 边、tab 底线）
    Color divider;        // 弱分隔

    // 输入控件
    Color field_bg;
    Color field_bg_hover;
    Color field_bg_active;
    Color field_border;
    Color field_border_focus;

    // 强调色（按用户指定 #22C55E，两主题 accent 同色，hover/pressed 微调明度）
    Color accent;
    Color accent_hover;
    Color accent_pressed;
    Color accent_soft;    // 淡背景（如 badge 背景）
    Color accent_fg;      // 覆盖在 accent 上的文字/图标

    // 状态色
    Color success;
    Color warning;
    Color danger;
    Color danger_hover;
    Color info;

    // 轨道 / 进度槽底（Slider/Gauge/ProgressBar 通用）
    Color track;
    Color track_soft;
};

}  // namespace hui::theme
