#pragma once

#include <string>
#include <utility>
#include <vector>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// v1.18 (2026-05-14): inline 多色 runs。
// 一行文本里需要分段染色（典型场景：hero 标题 "Build native. Ship now." 把 native /
// now 染成 accent）时用 set_runs。单段 set_content 路径完全保留，零迁移成本。
//
// 字段语义：
//   text   要画的字符串（可以含空格 / 标点；run 之间不自动加空格）
//   color  alpha < 0.001 时 = "继承 widget 当前色"（resolve_color 结果）
//   bold / bold_explicit
//          bold_explicit=false（默认）时 run 跟 widget 的 bold；true 时用 run 自己的 bold
// v1.21 (2026-05-15): 加 color_explicit 标记位。
// 原 paint 判断 `r.color.a > 0.001F` 视未指定颜色 (Color{} = 不透明纯黑 a=1.0)
// 为"显式黑色"，把 widget set_color 的默认色绕过 → dark 主题下出现"黑字黑底"。
// 现在显式构造 TextRun{text, color} 才置 explicit=true，TextRun{text} 走默认色。
struct TextRun {
    std::string text {};
    Color color {};
    bool bold {false};
    bool bold_explicit {false};
    bool color_explicit {false};

    TextRun() = default;
    TextRun(std::string t) : text(std::move(t)) {}
    TextRun(std::string t, Color c)
        : text(std::move(t)), color(c), color_explicit(true) {}
    TextRun(std::string t, Color c, bool b)
        : text(std::move(t)), color(c), bold(b),
          bold_explicit(true), color_explicit(true) {}
};

// 文本语义角色。paint 时按 role 去 theme 查颜色；需要 accent 色（成功绿）时用 Accent。
enum class TextRole {
    Primary,    // 正文、标题 → text_primary
    Secondary,  // 副文本 → text_secondary
    Muted,      // 说明 / helper → text_muted
    Disabled,   // 禁用态 → text_disabled
    Inverse,    // 盖在 accent 上的白字 → text_inverse
    Accent,     // 强调色文字 → accent
    Danger,     // 报错 / 危险文字 → danger
    Warning,    // 警示 → warning
    Info,       // 提示 → info
};

// 常用字号的语义 preset，避免业务代码到处写 14.0F / 19.0F 魔数。
// 运行时会查 theme.typography 对应阶梯。
enum class TextStylePreset {
    Display,
    Title,
    Heading,
    Subtitle,
    Body,
    Label,
    Caption,
};

class Text : public Widget {
public:
    Text() = default;

    explicit Text(std::string content)
        : content_(std::move(content)) {
    }

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Text";
    }

    [[nodiscard]] const std::string& content() const noexcept {
        return content_;
    }

    void set_content(std::string content) {
        content_ = std::move(content);
        runs_.clear();
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.18: 多段 inline 富文本。
    // 调用后 set_content 的字符串将被忽略（runs_ 非空时走多段路径）。
    // 传空 vector 视为退回单段模式（继续用 content_）。
    void set_runs(std::vector<TextRun> runs) {
        runs_ = std::move(runs);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool has_runs() const noexcept { return !runs_.empty(); }
    [[nodiscard]] const std::vector<TextRun>& runs() const noexcept { return runs_; }

    void set_role(TextRole role) noexcept {
        if (role_ == role) {
            return;
        }
        role_ = role;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] TextRole role() const noexcept {
        return role_;
    }

    // 显式指定字号（单位 px）。不调则使用 preset。
    void set_font_size(float font_size) noexcept {
        font_size_override_ = font_size;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_bold(bool bold) noexcept {
        bold_override_ = bold;
        explicit_bold_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    // 按 theme.typography 阶梯拿字号 + 字重。
    void set_preset(TextStylePreset preset) noexcept {
        preset_ = preset;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_alignment(TextAlignment alignment) noexcept {
        alignment_ = alignment;
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.17 (2026-05-03): 显式覆盖文字色, 不再走 theme TextRole 查表。
    // 业务场景: HeroCard banner 用"独立于 theme 的黑金渐变", 内部文字必须用
    // 与 banner 配套的米金 / 中金 / 暗金, 而不是 theme.text_primary (会随
    // 主题变黑/白)。clear_color_override 恢复 role 查表。
    void set_color(Color color) noexcept {
        color_override_ = color;
        color_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }
    void clear_color_override() noexcept {
        if (!color_overridden_) return;
        color_overridden_ = false;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        const theme::TextStyle style = resolve_style();
        const float size = font_size_override_ > 0.0F ? font_size_override_ : style.size;
        const bool widget_bold = explicit_bold_ ? bold_override_
                                                : static_cast<int>(style.weight) >= 600;

        // 单段（旧路径）
        if (runs_.empty()) {
            context.draw_text(frame(), content_, resolve_color(), alignment_,
                              size, widget_bold);
            return;
        }

        // 多段路径：手动按 alignment 排版，依次画 run。
        const Color default_color = resolve_color();
        const RectF f = frame();

        // 1) 测每段宽度 + 总宽
        // 注：widgets/basic 不能依赖 app 模块（架构分层），所以这里暂用 char ×
        // 0.55 × size 的粗略估算，与 examples/folders_app/brand_button.hpp 同策略。
        // 等需要精确字宽时（Windows 端 hero 标题居中产生明显错位时），把
        // measure_text_width 抽到 render 层做成 free function，由 app 层 init
        // 注入实现，Text 通过 render 层调用即可去掉这个估算。
        std::vector<float> widths;
        widths.reserve(runs_.size());
        float total_w = 0.0F;
        for (const auto& r : runs_) {
            const float w = static_cast<float>(r.text.size()) * size * 0.55F;
            widths.push_back(w);
            total_w += w;
        }

        // 2) 起始 x
        float x = f.x;
        switch (alignment_) {
        case TextAlignment::Center:
            x = f.x + (f.width - total_w) * 0.5F;
            break;
        case TextAlignment::Trailing:
            x = f.x + f.width - total_w;
            break;
        case TextAlignment::Leading:
        default:
            x = f.x;
            break;
        }

        // 3) 依次画
        for (std::size_t i = 0; i < runs_.size(); ++i) {
            const auto& r = runs_[i];
            const Color c = r.color_explicit ? r.color : default_color;
            const bool b = r.bold_explicit ? r.bold : widget_bold;
            context.draw_text(
                RectF{x, f.y, widths[i], f.height},
                r.text, c, TextAlignment::Leading, size, b);
            x += widths[i];
        }
    }

private:
    [[nodiscard]] Color resolve_color() const noexcept {
        if (color_overridden_) return color_override_;
        const auto& c = theme::colors();
        switch (role_) {
        case TextRole::Primary:   return c.text_primary;
        case TextRole::Secondary: return c.text_secondary;
        case TextRole::Muted:     return c.text_muted;
        case TextRole::Disabled:  return c.text_disabled;
        case TextRole::Inverse:   return c.text_inverse;
        case TextRole::Accent:    return c.accent;
        case TextRole::Danger:    return c.danger;
        case TextRole::Warning:   return c.warning;
        case TextRole::Info:      return c.info;
        }
        return c.text_primary;
    }

    [[nodiscard]] const theme::TextStyle& resolve_style() const noexcept {
        const auto& t = theme::typography();
        switch (preset_) {
        case TextStylePreset::Display:  return t.display;
        case TextStylePreset::Title:    return t.title;
        case TextStylePreset::Heading:  return t.heading;
        case TextStylePreset::Subtitle: return t.subtitle;
        case TextStylePreset::Body:     return t.body;
        case TextStylePreset::Label:    return t.label;
        case TextStylePreset::Caption:  return t.caption;
        }
        return t.body;
    }

    std::string content_ {};
    std::vector<TextRun> runs_ {};
    TextRole role_ {TextRole::Primary};
    TextStylePreset preset_ {TextStylePreset::Body};
    float font_size_override_ {0.0F};  // 0 = 走 preset
    bool bold_override_ {false};
    bool explicit_bold_ {false};
    TextAlignment alignment_ {TextAlignment::Leading};
    // v1.17 (2026-05-03): set_color 覆盖, color_overridden_=false 时走 role 查表。
    Color color_override_ {};
    bool  color_overridden_ {false};
};

}  // namespace hui
