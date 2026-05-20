#pragma once

#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/command.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

// jtUI Button
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

enum class ButtonVariant {
    Filled,    // 实心填色（主色按钮，图中主绿按钮）
    Outlined,  // 边框 + 透明底
    Ghost,     // 无边无底，hover 才出底色
    Text,      // 纯文本链接型（无边无底，hover 换色）
    // v2 风格 (2026-05-14): 现代设计语言新增
    Tonal,             // 柔和填充: tone_soft 底 + tone 字 (Material 3 风)
    Gradient,          // 同色明暗渐变: tone_500 顶 -> tone_700 底
    GradientBlackGold, // 黑金高级感: base_19 顶 -> base_21 底 + accent 边框
};

enum class ButtonSize {
    Small,     // 28px 高，字号 12，内边距 10
    Medium,    // 36px 高，字号 13，内边距 14（默认）
    Large,     // 44px 高，字号 15，内边距 20
};

enum class ButtonShape {
    Default,   // 圆角按 size 阶梯 (小: radius.sm, 中: radius.sm, 大: radius.md)
    Pill,      // 胶囊形: corner_radius = height / 2, 两端全圆
    Circle,    // 圆形: 调用方应自行设宽高一致, corner_radius = min(w,h) / 2
    Square,    // 正方形: 调用方应自行设宽高一致, 圆角按 size 阶梯 (Icon-only 用)
};

class Button : public Widget {
  public:
    Button() = default;

    explicit Button(std::string text) : text_(std::move(text)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Button";
    }

    [[nodiscard]] const std::string& text() const noexcept {
        return text_;
    }

    void set_text(std::string text);

    void set_command(Command command) {
        command_ = std::move(command);
    }

    void set_variant(ButtonVariant variant) noexcept;
    void set_size(ButtonSize size) noexcept;
    void set_tone(theme::Tone tone) noexcept;
    void set_shape(ButtonShape shape) noexcept;

    // v1.20 (2026-05-14): per-instance 颜色 override —— business 想自定义品牌色
    // 不再需要写 BrandButton 副本（folders_app/jtui_hero/jtui_live/jtui_pro 之前
    // 各有一份 ~180 行重复）。set_colors 调过后，paint 用这 4 个色 override theme
    // tone 算出的 bg/hover/pressed/fg。默认情况下 border 自动等于 bg（视觉无边框）。
    // clear_color_override() 恢复 theme tone 路径。
    void set_colors(Color bg, Color hover, Color pressed, Color fg) noexcept;
    void clear_color_override() noexcept;

    // v1.20.1 (2026-05-14): 显式 border 颜色 + 粗细。优先级最高，会盖掉 set_colors
    // 自动推的 border = bg。传 thickness=0 或全透明色 = 不画边框。常见用法：
    // 圆角图标按钮 + bg_card + border_strong 1px 描边显示边界。
    void set_border(Color color, float thickness = 1.0F) noexcept;
    void clear_border_override() noexcept;

    // v1.20.1: 任意 corner_radius override，盖掉 ButtonShape 推的圆角。Pill / Circle
    // 这种语义化形状仍推荐用 set_shape；这接口给真正需要"我就是要 8px"的场景。
    void set_corner_radius(float radius) noexcept;
    void clear_corner_radius_override() noexcept;

    // v1.20.1: 任意 font_size + bold override，盖掉 ButtonSize 推的字号。Small/Medium
    // /Large 阶梯不够细的场景用这个。size <= 0 表示不画文字（icon-only 用）。
    void set_font_size(float size, bool bold = true) noexcept;
    void clear_font_size_override() noexcept;

    // 流光边框 (border beam): 启用后边框上有一道亮色光带持续沿轮廓滚动一圈,
    // 适合 Hero CTA / 黑金按钮等强调场景。会让 button 持续保持 dirty,
    // 不要批量开 (开 1-2 个最佳; 多了 timer 永不停, 性能下降)。
    void set_border_beam(bool enabled) noexcept;

    // 图标槽：左右各一。当前 PaintContext 只能画文本和几何体，
    // 所以图标用 utf8 字符串（可以是符号字符、emoji、或 icon font 码点）。
    // v2 接 icon font 后接口不变，只是字体切换。
    void set_leading_icon(std::string icon);
    void set_trailing_icon(std::string icon);

    // v1.20 (2026-05-14): codicon 一等公民支持。业务传 codicon name (如 "globe"
    // / "color-mode")，paint 时用 codicon 字体 + 查表转 codepoint 渲染。比 set_xxx_icon
    // 接受字面值更直观，icon-only 圆按钮场景应该用这个。
    void set_leading_codicon(std::string name);
    void set_trailing_codicon(std::string name);

    [[nodiscard]] Signal<>& on_clicked() noexcept {
        return clicked_;
    }

    void click();

    void on_click(PointF) override;

    [[nodiscard]] bool paint_reacts_to_pressed() const noexcept override {
        return true;
    }

    bool tick(float delta) override;

    void paint(PaintContext& context) const override;

  private:
    std::string text_{"Button"};
    std::string leading_icon_{};
    std::string trailing_icon_{};
    std::string leading_codicon_{};   // v1.20: codicon name (优先于 leading_icon_)
    std::string trailing_codicon_{};
    Signal<> clicked_{};
    Command command_{};
    ButtonVariant variant_{ButtonVariant::Filled};
    ButtonSize size_{ButtonSize::Medium};
    ButtonShape shape_{ButtonShape::Default};
    theme::Tone tone_{theme::Tone::Accent};
    // 0 = 松开, 1 = 按到底。tick 指数衰减到 pressed() ? 1 : 0。
    float press_progress_{0.0F};
    // border beam 相位 [0, 1). tick 持续推进, paint 时按 phase 计算光带头位置。
    bool border_beam_{false};
    float beam_phase_{0.0F};
    mutable bool animation_primed_{false};

    // v1.20: per-instance 颜色 override（见 set_colors 注释）
    Color color_bg_{};
    Color color_hover_{};
    Color color_pressed_{};
    Color color_fg_{};
    bool color_overridden_{false};
    // v1.20.1: 三个独立 override（见各 setter 注释）
    Color border_color_{};
    float border_thickness_{1.0F};
    bool border_overridden_{false};
    float corner_radius_{0.0F};
    bool corner_radius_overridden_{false};
    float font_size_{13.0F};
    bool font_bold_{true};
    bool font_size_overridden_{false};
};

} // namespace hui
