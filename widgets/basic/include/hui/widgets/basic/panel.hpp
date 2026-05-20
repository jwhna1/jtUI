#pragma once

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/elevation_tokens.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// Panel 背景来自 theme.bg_surface。要做整窗背景、卡片、二级面板的时候
// 用不同的 Role 区分层级。
//
// 坐标模型:Panel 不参与布局调度 — children 用 *绝对* 坐标(set_frame 传整窗
// 坐标系下的 RectF)而不是相对父 panel。Panel::set_frame 只改自己的 frame,
// 不会自动平移 children。要把整棵 subtree 移到新位置,用 Element::shift_subtree
// (dx, dy)。这是有意识的设计 — 简化 paint(D2D 直接拿绝对坐标),允许任意
// 自由定位;代价是手动 shift。需要"父 frame 改 → 子自动跟着"的语义请用
// FlexBox / GridBox / ListView 等真容器(它们重写了 arrange 主动 set 子 frame)。
enum class PanelRole {
    Base,        // 根面板，铺整窗 → theme.bg_base
    Surface,     // 一级面板 → theme.bg_surface
    SurfaceAlt,  // 二级面板 / 卡片区 → theme.bg_surface_alt
    Raised,      // 卡片 / dialog 背景 → theme.bg_raised
};

class Panel : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Panel";
    }

    void set_role(PanelRole role) noexcept {
        if (role_ == role) {
            return;
        }
        role_ = role;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] PanelRole role() const noexcept {
        return role_;
    }

    void paint(PaintContext& context) const override {
        const float radius = resolve_corner_radius();
        const Color bg = resolve_background();
        // v1.22 (2026-05-15): 阴影画在底色之前 —— 让阴影位于卡片"下方"。
        // 业务侧调 set_shadow(theme::elevation().level_2) 后这里自动生效；
        // shadow_enabled_=false 时不发出命令、零开销。
        if (shadow_enabled_ && shadow_.color.a > 0.001F) {
            context.fill_shadow(frame(), radius,
                                PointF{shadow_.offset_x, shadow_.offset_y},
                                shadow_.blur, shadow_.spread, shadow_.color);
        }
        // v1.23 (2026-05-15): 毛玻璃 backdrop blur —— 在 shadow 之后、bg 填充
        // 之前画。用了 backdrop blur 的 Panel 通常不再 set_background_color
        // 不透明色（否则盖掉模糊层），tint 由 set_backdrop_blur 的第二参数给。
        if (backdrop_enabled_) {
            context.fill_backdrop_blur(frame(), radius,
                                       backdrop_blur_, backdrop_tint_);
        }
        // bg.a < 0.001 视为完全透明: 跳过填充让背后内容直接透出 (业务做"半透
        // 玻璃感"卡片时配合 set_background_color({...,0}) 完全不画底)。
        if (bg.a > 0.001F) {
            if (radius <= 0.0F) {
                context.fill_rect(frame(), bg);
            } else {
                context.fill_rounded_rect(frame(), bg, radius);
            }
        }
        // v1.16 (2026-05-03): 可选 1px 描边, 让半透明卡片在背景上仍有清晰边界。
        if (border_color_.a > 0.001F && border_thickness_ > 0.0F) {
            if (radius <= 0.0F) {
                context.stroke_rect(frame(), border_color_, border_thickness_);
            } else {
                context.stroke_rounded_rect(frame(), border_color_, radius,
                                             border_thickness_);
            }
        }
    }

    // v1.1 (2026-04-27): 按 role 默认小圆角. Surface/SurfaceAlt/Raised 用作
    // 卡片背板时自动 10px 圆角, 视觉与 qml 设计对齐 (整页 ScrollView 内的
    // 各分区卡片都是小圆角矩形)。Base 仍保持矩形作为整窗 / content 背景。
    //
    // 应用层可以通过 set_corner_radius 覆盖默认值; 传 0 等于强制矩形, 传 -1
    // 用 role 默认 (init 值)。
    void set_corner_radius(float radius) noexcept {
        if (corner_radius_override_ == radius) return;
        corner_radius_override_ = radius;
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.16 (2026-05-03): 覆盖 theme 默认背景色。alpha < 1 实现半透明卡片
    // (透出背后主 UI), alpha = 0 完全透明 (Panel 不画底色, 仅留 border / 子树)。
    // 不调或 reset 时 paint 走 resolve_background 用 theme 默认色。
    // 业务用法: card->set_background_color({c.bg_surface.r, c.bg_surface.g,
    //                                         c.bg_surface.b, 0.55F});
    void set_background_color(Color color) noexcept {
        background_override_ = color;
        background_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }
    void clear_background_override() noexcept {
        if (!background_overridden_) return;
        background_overridden_ = false;
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.16 (2026-05-03): 可选边框, 业务画半透明卡片常配合 1px 描边突出边界。
    // alpha = 0 等于不画。thickness 默认 1px。
    void set_border(Color color, float thickness = 1.0F) noexcept {
        border_color_ = color;
        border_thickness_ = thickness;
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.22 (2026-05-15): 高斯模糊投影。业务想做"靠阴影分割" 的现代卡片视觉
    // 时用 set_shadow(theme::elevation().level_2) 替代 set_border 即可。后端
    // 走 D2D 1.1 CLSID_D2D1Shadow Effect 做真高斯模糊。
    //
    // 用法：
    //   card->set_shadow(jtui::theme::elevation().level_2);
    //   // 不再 set_border —— 阴影直接给视觉分隔感，不再要 1px 描边。
    void set_shadow(theme::Shadow shadow) noexcept {
        shadow_ = shadow;
        shadow_enabled_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void clear_shadow() noexcept {
        if (!shadow_enabled_) return;
        shadow_enabled_ = false;
        mark_dirty(DirtyFlags::Paint);
    }

    // v1.23 (2026-05-15): 毛玻璃 / backdrop blur。把本 Panel frame 区域背后
    // 已绘制的内容做高斯模糊，再叠 tint —— 经典磨砂导航条。
    // blur 是模糊半径（10~30 视觉较好）；tint 是叠在模糊层之上的半透明色
    // （品牌底色低 alpha 版，alpha 太高会糊掉背景质感、太低则可读性差，
    // 0.5~0.75 区间通常合适）。
    //
    // 重要：用了 backdrop blur 的 Panel 不要再 set_background_color 不透明
    // 色，否则把模糊层完全盖掉；要分隔感用 set_border 画 1px 描边即可。
    void set_backdrop_blur(float blur, Color tint) noexcept {
        backdrop_blur_ = blur;
        backdrop_tint_ = tint;
        backdrop_enabled_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void clear_backdrop_blur() noexcept {
        if (!backdrop_enabled_) return;
        backdrop_enabled_ = false;
        mark_dirty(DirtyFlags::Paint);
    }

private:
    [[nodiscard]] Color resolve_background() const noexcept {
        if (background_overridden_) return background_override_;
        const auto& c = theme::colors();
        switch (role_) {
        case PanelRole::Base:       return c.bg_base;
        case PanelRole::Surface:    return c.bg_surface;
        case PanelRole::SurfaceAlt: return c.bg_surface_alt;
        case PanelRole::Raised:     return c.bg_raised;
        }
        return c.bg_surface;
    }

    [[nodiscard]] float resolve_corner_radius() const noexcept {
        if (corner_radius_override_ >= 0.0F) return corner_radius_override_;
        switch (role_) {
        case PanelRole::Base:       return 0.0F;
        case PanelRole::Surface:    return 10.0F;
        case PanelRole::SurfaceAlt: return 10.0F;
        case PanelRole::Raised:     return 14.0F;
        }
        return 0.0F;
    }

    PanelRole role_ {PanelRole::Base};
    float corner_radius_override_ {-1.0F};
    // v1.16 (2026-05-03): 半透明卡片 / 自定义底色支持。
    Color background_override_ {};
    bool  background_overridden_ {false};
    Color border_color_ {};
    float border_thickness_ {0.0F};
    // v1.22 (2026-05-15): 卡片投影 token。shadow_enabled_=false 时 paint 跳过
    // 阴影命令，零开销；启用后 paint 在 fill_rounded_rect 之前先 fill_shadow。
    theme::Shadow shadow_ {};
    bool shadow_enabled_ {false};
    // v1.23 (2026-05-15): 毛玻璃 backdrop blur 参数。
    float backdrop_blur_ {0.0F};
    Color backdrop_tint_ {};
    bool  backdrop_enabled_ {false};
};

}  // namespace hui
