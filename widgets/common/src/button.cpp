#include "hui/widgets/common/button.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "hui/foundation/codicon_font.hpp"
#include "hui/foundation/codicon_lookup.hpp"

// jtUI Button 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

namespace {

struct SizeSpec {
    float font_size;
    float padding_x;
    float icon_gap;
    float radius;
};

// codepoint → utf8（codicon BMP 范围 3 字节够）
void encode_codicon_utf8(char32_t code, std::string& out) {
    out.clear();
    if (code < 0x80U) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800U) {
        out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    }
}

SizeSpec spec_for(ButtonSize size) noexcept {
    switch (size) {
    case ButtonSize::Small:  return SizeSpec{12.0F, 10.0F, 6.0F, theme::radius().sm};
    case ButtonSize::Medium: return SizeSpec{13.0F, 14.0F, 8.0F, theme::radius().sm};
    case ButtonSize::Large:  return SizeSpec{15.0F, 20.0F, 10.0F, theme::radius().md};
    }
    return SizeSpec{13.0F, 14.0F, 8.0F, theme::radius().sm};
}

struct Palette {
    Color background;
    Color background_hover;
    Color background_pressed;
    Color border;
    Color border_hover;
    Color foreground;
};

// 用于 Gradient / GradientBlackGold variant 的额外色对。fill_gradient_rect 走顶/底色。
// Filled / Outlined / Ghost / Text / Tonal 不用这个 (background 单色已经足够)。
struct GradientStops {
    Color top;
    Color bottom;
};

// 根据 tone 推导一个"更深"的色, 用于 Gradient variant 的底色。当前 jtui::theme
// 没有完整 Tailwind 700 阶梯映射, 用 0.7 倍亮度近似 (足够拉开视觉层次)。
Color deepen(Color c, float factor) noexcept {
    return Color{
        std::clamp(c.r * factor, 0.0F, 1.0F),
        std::clamp(c.g * factor, 0.0F, 1.0F),
        std::clamp(c.b * factor, 0.0F, 1.0F),
        c.a,
    };
}

GradientStops gradient_stops_for(ButtonVariant variant, theme::Tone tone) noexcept {
    const auto& c = theme::colors();
    if (variant == ButtonVariant::GradientBlackGold) {
        // 黑金: bg_surface (基本黑) 顶 -> bg_base 底 (更深); 边框走 accent
        return GradientStops{c.bg_surface, c.bg_base};
    }
    // 同色明暗渐变: tone_500 顶 -> tone * 0.7 底
    const Color tone_color = theme::color_for(tone);
    return GradientStops{tone_color, deepen(tone_color, 0.72F)};
}

Palette palette_for(ButtonVariant variant, theme::Tone tone, bool enabled) noexcept {
    const auto& c = theme::colors();
    const Color tone_color = theme::color_for(tone);
    const Color tone_hover = theme::hover_color_for(tone);
    const Color transparent = Color::from_rgba8(0U, 0U, 0U, 0U);

    if (!enabled) {
        // 通用禁用色：填色型灰底白字，其余型灰底灰字
        if (variant == ButtonVariant::Filled || variant == ButtonVariant::Gradient
            || variant == ButtonVariant::GradientBlackGold) {
            return Palette{c.bg_raised, c.bg_raised, c.bg_raised,
                           c.border, c.border, c.text_disabled};
        }
        return Palette{transparent, transparent, transparent,
                       c.border, c.border, c.text_disabled};
    }

    switch (variant) {
    case ButtonVariant::Filled:
        return Palette{tone_color, tone_hover, tone_color,
                       tone_color, tone_hover,
                       // Warning（黄）底在 light 下白字对比差，统一给 accent_fg 即白
                       c.accent_fg};
    case ButtonVariant::Outlined:
        return Palette{transparent, c.accent_soft, c.accent_soft,
                       tone_color, tone_hover, tone_color};
    case ButtonVariant::Ghost:
        return Palette{transparent, c.field_bg_hover, c.field_bg_active,
                       transparent, transparent, tone_color};
    case ButtonVariant::Text:
        return Palette{transparent, transparent, transparent,
                       transparent, transparent, tone_color};
    case ButtonVariant::Tonal: {
        // accent_soft 在 dark theme 下是 green_900 深绿底, 在 light 下是 green_100 浅绿底
        // 文字色用 tone_color (accent / success / ...) 在两套主题下都有对比。
        const Color hover_bg = deepen(c.accent_soft, 1.15F);  // 微亮 hover
        return Palette{c.accent_soft, hover_bg, c.accent_soft,
                       transparent, transparent, tone_color};
    }
    case ButtonVariant::Gradient:
        // background = 渐变顶色 (paint 内走 fill_gradient_rect 取顶/底), 这里给个
        // 退化用单色 (hover/pressed 退化不画 gradient, 用 tone_color 即可)
        return Palette{tone_color, tone_hover, deepen(tone_color, 0.85F),
                       tone_color, tone_hover, c.accent_fg};
    case ButtonVariant::GradientBlackGold:
        // 黑金: 文字用 accent 色 (突出"金"感), 边框 accent
        return Palette{c.bg_surface, c.bg_surface_alt, c.bg_surface_alt,
                       c.accent, c.accent_hover, c.accent};
    }
    return Palette{tone_color, tone_hover, tone_color,
                   tone_color, tone_hover, c.accent_fg};
}

// 按 shape 解析 corner_radius (Pill 强制 height/2, 其他按 size 阶梯)
float corner_radius_for(ButtonShape shape, float size_radius, float frame_h) noexcept {
    switch (shape) {
    case ButtonShape::Default:
    case ButtonShape::Square:
        return size_radius;
    case ButtonShape::Pill:
    case ButtonShape::Circle:
        return frame_h * 0.5F;
    }
    return size_radius;
}

}  // namespace

void Button::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_variant(ButtonVariant variant) noexcept {
    if (variant_ == variant) return;
    variant_ = variant;
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_size(ButtonSize size) noexcept {
    if (size_ == size) return;
    size_ = size;
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_tone(theme::Tone tone) noexcept {
    if (tone_ == tone) return;
    tone_ = tone;
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_shape(ButtonShape shape) noexcept {
    if (shape_ == shape) return;
    shape_ = shape;
    mark_dirty(DirtyFlags::Paint);
}

// v1.20: per-instance 颜色 override —— 见 button.hpp 注释
void Button::set_colors(Color bg, Color hover, Color pressed, Color fg) noexcept {
    color_bg_ = bg;
    color_hover_ = hover;
    color_pressed_ = pressed;
    color_fg_ = fg;
    color_overridden_ = true;
    mark_dirty(DirtyFlags::Paint);
}

void Button::clear_color_override() noexcept {
    if (!color_overridden_) return;
    color_overridden_ = false;
    mark_dirty(DirtyFlags::Paint);
}

// v1.20.1: 显式 border override
void Button::set_border(Color color, float thickness) noexcept {
    border_color_ = color;
    border_thickness_ = thickness;
    border_overridden_ = true;
    mark_dirty(DirtyFlags::Paint);
}

void Button::clear_border_override() noexcept {
    if (!border_overridden_) return;
    border_overridden_ = false;
    mark_dirty(DirtyFlags::Paint);
}

// v1.20.1: 任意 corner_radius override
void Button::set_corner_radius(float radius) noexcept {
    corner_radius_ = std::max(0.0F, radius);
    corner_radius_overridden_ = true;
    mark_dirty(DirtyFlags::Paint);
}

void Button::clear_corner_radius_override() noexcept {
    if (!corner_radius_overridden_) return;
    corner_radius_overridden_ = false;
    mark_dirty(DirtyFlags::Paint);
}

// v1.20.1: 任意 font_size + bold override
void Button::set_font_size(float size, bool bold) noexcept {
    font_size_ = size;
    font_bold_ = bold;
    font_size_overridden_ = true;
    mark_dirty(DirtyFlags::Paint);
}

void Button::clear_font_size_override() noexcept {
    if (!font_size_overridden_) return;
    font_size_overridden_ = false;
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_border_beam(bool enabled) noexcept {
    if (border_beam_ == enabled) return;
    border_beam_ = enabled;
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_leading_icon(std::string icon) {
    leading_icon_ = std::move(icon);
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_leading_codicon(std::string name) {
    leading_codicon_ = std::move(name);
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_trailing_codicon(std::string name) {
    trailing_codicon_ = std::move(name);
    mark_dirty(DirtyFlags::Paint);
}

void Button::set_trailing_icon(std::string icon) {
    trailing_icon_ = std::move(icon);
    mark_dirty(DirtyFlags::Paint);
}

void Button::click() {
    clicked_.emit();
    command_.execute();
}

void Button::on_click(PointF) {
    if (enabled()) {
        click();
    }
}

bool Button::tick(float delta) {
    // press 动画
    const float target = pressed() ? 1.0F : 0.0F;
    bool need_more = false;
    if (press_progress_ != target) {
        // 比 Switch 快一点（~80ms），press/release 是即时反馈，滑 150ms 会感觉粘手。
        constexpr float rate = 32.0F;
        const float step = (target - press_progress_) * std::clamp(delta * rate, 0.0F, 1.0F);
        float next = press_progress_ + step;
        if (std::abs(next - target) < 0.01F) {
            next = target;
        }
        press_progress_ = next;
        mark_dirty(DirtyFlags::Paint);
        if (press_progress_ != target) {
            need_more = true;
        }
    }

    // border beam 动画: 持续推进 phase, 一圈 ~3 秒
    if (border_beam_) {
        constexpr float kBeamPeriodSeconds = 3.0F;
        beam_phase_ += delta / kBeamPeriodSeconds;
        while (beam_phase_ >= 1.0F) beam_phase_ -= 1.0F;
        while (beam_phase_ < 0.0F) beam_phase_ += 1.0F;
        mark_dirty(DirtyFlags::Paint);
        need_more = true;
    }
    return need_more;
}

void Button::paint(PaintContext& context) const {
    animation_primed_ = true;

    const SizeSpec sz = spec_for(size_);
    Palette pal = palette_for(variant_, tone_, enabled());

    // v1.20: per-instance 颜色 override，覆盖 theme tone 算出的 4 个核心字段。
    // border 字段同步设为 bg（business 调 set_colors 表示"颜色我全权管"，
    // 不再要求他们额外指定 border 色。视觉上 border = bg → 看不到边框）。
    if (color_overridden_ && enabled()) {
        pal.background         = color_bg_;
        pal.background_hover   = color_hover_;
        pal.background_pressed = color_pressed_;
        pal.foreground         = color_fg_;
        pal.border             = color_bg_;
        pal.border_hover       = color_hover_;
    }
    // v1.20.1: 显式 border override 优先级最高（盖掉上面 set_colors 推的 border = bg）
    if (border_overridden_ && enabled()) {
        pal.border       = border_color_;
        pal.border_hover = border_color_;
    }
    const float t = enabled() ? std::clamp(press_progress_, 0.0F, 1.0F) : 0.0F;
    // v1.20.1: corner_radius override 优先于 shape 推算
    const float radius = corner_radius_overridden_
        ? corner_radius_
        : corner_radius_for(shape_, sz.radius, frame().height);

    const bool is_gradient = (variant_ == ButtonVariant::Gradient
                              || variant_ == ButtonVariant::GradientBlackGold);

    // base → pressed 线性插值复用原来的动画逻辑。
    const Color base = hovered() ? pal.background_hover : pal.background;
    const Color background{
        base.r + (pal.background_pressed.r - base.r) * t,
        base.g + (pal.background_pressed.g - base.g) * t,
        base.b + (pal.background_pressed.b - base.b) * t,
        base.a + (pal.background_pressed.a - base.a) * t,
    };
    const Color border = hovered() ? pal.border_hover : pal.border;

    // 背景绘制: gradient 走 fill_gradient_rect, 其他走 fill_rounded_rect
    if (is_gradient && enabled()) {
        const GradientStops gs = gradient_stops_for(variant_, tone_);
        // press 时整体下沉一点 (顶色变深)
        const Color top = Color{
            gs.top.r * (1.0F - t * 0.15F),
            gs.top.g * (1.0F - t * 0.15F),
            gs.top.b * (1.0F - t * 0.15F),
            gs.top.a,
        };
        context.fill_gradient_rect(frame(), top, gs.bottom, radius);
    } else if (background.a > 0.0F) {
        context.fill_rounded_rect(frame(), background, radius);
    }

    // 边框: Outlined / Tonal / GradientBlackGold 显式描边; Filled / Gradient
    // 加一圈同色细线让视觉更锐利; Ghost / Text 无描边。
    // v1.20.1: border_overridden_ 时尊重业务给的 thickness
    if (border.a > 0.0F) {
        const bool need_stroke = variant_ != ButtonVariant::Ghost
                                 && variant_ != ButtonVariant::Text;
        if (need_stroke) {
            float bw = (variant_ == ButtonVariant::GradientBlackGold) ? 1.5F : 1.0F;
            if (border_overridden_) bw = border_thickness_;
            if (bw > 0.0F)
                context.stroke_rounded_rect(frame(), border, radius, bw);
        }
    }

    // 流光边框 (border beam, v2 2026-05-14): 沿矩形周长画 200 段紧贴 2.4px fill_rect,
    // 每段位置在 perim 上等间距分布, 头 alpha 1.0 -> 尾平方衰减到 0, 形成连续光带 +
    // 拖尾。fill_rect 抗锯齿走 D2D AA, 段与段紧贴, 视觉远比 28 个 ellipse 连续。
    //
    // 性能: 200 个 fill_rect 在 D2D 下每帧 ~0.2ms, 远低于一帧 16ms 预算。一个窗口
    // 同时启用 1-3 个 border beam button 完全 OK。
    if (border_beam_ && enabled()) {
        constexpr int kBeamPixels = 200;
        constexpr float kBeamArcRatio = 0.32F;  // 光带覆盖周长的 32%
        constexpr float kPixSize = 2.4F;
        const float w = frame().width;
        const float h = frame().height;
        const float perim = 2.0F * (w + h);
        const float beam_len = perim * kBeamArcRatio;
        const Color glow_base = theme::colors().accent;

        // 沿矩形周长 (x=0 起点, 顺时针) 算 (px, py)
        auto pos_on_perim = [&](float pos) {
            while (pos < 0.0F) pos += perim;
            while (pos >= perim) pos -= perim;
            float px = 0.0F;
            float py = 0.0F;
            if (pos < w) {
                px = frame().x + pos;
                py = frame().y;
            } else if (pos < w + h) {
                px = frame().x + w;
                py = frame().y + (pos - w);
            } else if (pos < 2.0F * w + h) {
                px = frame().x + w - (pos - w - h);
                py = frame().y + h;
            } else {
                px = frame().x;
                py = frame().y + h - (pos - 2.0F * w - h);
            }
            return PointF{px, py};
        };

        for (int i = 0; i < kBeamPixels; ++i) {
            const float fi = static_cast<float>(i);
            const float fN = static_cast<float>(kBeamPixels);
            const float offset = (fi / fN) * beam_len;
            const PointF p = pos_on_perim(beam_phase_ * perim - offset);

            // alpha 头 1.0 -> 尾 0, 平方衰减让尾巴更柔
            const float fade = 1.0F - fi / fN;
            const float alpha = fade * fade;
            const Color glow{glow_base.r, glow_base.g, glow_base.b, alpha};

            context.fill_rect(
                RectF{p.x - kPixSize * 0.5F, p.y - kPixSize * 0.5F,
                      kPixSize, kPixSize},
                glow);
        }
    }

    // 内容排版：leading icon + text + trailing icon，整体居中。
    // 优先级：leading_codicon_（codicon 字体）> leading_icon_（字面值）。
    // icon-only 圆按钮（无 text_）布局退化：图标居中显示。
    const bool has_lead_codi  = !leading_codicon_.empty();
    const bool has_trail_codi = !trailing_codicon_.empty();
    const bool has_leading  = has_lead_codi  || !leading_icon_.empty();
    const bool has_trailing = has_trail_codi || !trailing_icon_.empty();
    const bool has_text     = !text_.empty();

    // v1.20.1: font_size override 优先于 ButtonSize 推算。size <= 0 时禁用文字。
    const bool font_size_disabled = font_size_overridden_ && font_size_ <= 0.0F;
    const float font_px  = font_size_overridden_ ? std::max(font_size_, 0.0F) : sz.font_size;
    const bool  bold_txt = font_size_overridden_ ? font_bold_ : true;

    const float icon_col_w = font_px + 4.0F;

    // icon-only（has_leading && !has_text）：居中画图标，跳过 text/trailing 复杂排版
    if (has_leading && !has_text && !has_trailing) {
        // icon-only 圆按钮的图标尺寸：BrandButton 历史用 height*0.42，jtui::Button v1.20
        // 用 font_size+2。这两套都接受，icon-only 路径下用 height-based 让圆按钮里图标
        // 在 24/36/44/56 各种 frame 高度下都视觉适中。
        const float icon_size = std::max(frame().height * 0.42F, 12.0F);
        if (has_lead_codi) {
            const auto cp = foundation::codicon_codepoint_for(leading_codicon_);
            if (cp.has_value()) {
                std::string utf8;
                encode_codicon_utf8(*cp, utf8);
                context.draw_text_with_font(
                    frame(), std::move(utf8),
                    std::string(foundation::kCodiconFontFamily),
                    pal.foreground, TextAlignment::Center, icon_size, false);
            }
        } else {
            context.draw_text(frame(), leading_icon_, pal.foreground,
                              TextAlignment::Center, icon_size, true);
        }
        return;
    }

    if (has_leading) {
        const RectF lead_rect{
            frame().x + sz.padding_x, frame().y, icon_col_w, frame().height};
        if (has_lead_codi) {
            const auto cp = foundation::codicon_codepoint_for(leading_codicon_);
            if (cp.has_value()) {
                std::string utf8;
                encode_codicon_utf8(*cp, utf8);
                context.draw_text_with_font(
                    lead_rect, std::move(utf8),
                    std::string(foundation::kCodiconFontFamily),
                    pal.foreground, TextAlignment::Center, font_px, false);
            }
        } else {
            context.draw_text(lead_rect, leading_icon_, pal.foreground,
                              TextAlignment::Center, font_px, true);
        }
    }
    if (has_trailing) {
        const RectF trail_rect{
            frame().x + frame().width - sz.padding_x - icon_col_w, frame().y,
            icon_col_w, frame().height};
        if (has_trail_codi) {
            const auto cp = foundation::codicon_codepoint_for(trailing_codicon_);
            if (cp.has_value()) {
                std::string utf8;
                encode_codicon_utf8(*cp, utf8);
                context.draw_text_with_font(
                    trail_rect, std::move(utf8),
                    std::string(foundation::kCodiconFontFamily),
                    pal.foreground, TextAlignment::Center, font_px, false);
            }
        } else {
            context.draw_text(trail_rect, trailing_icon_, pal.foreground,
                              TextAlignment::Center, font_px, true);
        }
    }

    // 文本占剩余中间区域（没 text_ 或 font_size 被显式禁用则跳过）
    if (has_text && !font_size_disabled) {
        const float text_left = frame().x + sz.padding_x
                              + (has_leading ? icon_col_w + sz.icon_gap : 0.0F);
        const float text_right = frame().x + frame().width - sz.padding_x
                               - (has_trailing ? icon_col_w + sz.icon_gap : 0.0F);
        const RectF text_rect{text_left, frame().y, text_right - text_left, frame().height};
        context.draw_text(text_rect, text_, pal.foreground, TextAlignment::Center, font_px, bold_txt);
    }
}

} // namespace hui
