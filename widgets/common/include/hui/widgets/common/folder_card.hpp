#pragma once

// FolderCard —— 文件夹宫格中的单个文件夹卡。
// 视觉结构：
//   ┌─────────────────┐
//   │ ┌─┐  ┌─┐        │  ← 顶部叠放两张"封面"片（heading）
//   │ │  thumbnail    │  ← 主体大圆角矩形 + 中央 codicon
//   │ └───────────────┘
//   │  Title          │
//   │  15 photos      │  ← 副信息 + 右下角 lock/avatar group
//   └─────────────────┘
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hui/foundation/codicon_font.hpp"
#include "hui/foundation/codicon_lookup.hpp"
#include "hui/foundation/color.hpp"
#include "hui/foundation/geometry.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/widgets/common/avatar.hpp"

namespace hui {

class FolderCard : public Widget {
public:
    FolderCard() {
        auto group = std::make_unique<AvatarGroup>();
        group->set_max_visible(3);
        group->set_overlap(0.36F);
        avatar_group_ = group.get();
        append_child(std::move(group));
    }

    [[nodiscard]] std::string_view type_name() const noexcept override { return "FolderCard"; }

    // ───── 内容 ─────

    void set_title(std::string s) {
        title_ = std::move(s);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_subtitle(std::string s) {
        subtitle_ = std::move(s);
        mark_dirty(DirtyFlags::Paint);
    }

    // codicon 名，画在缩略图中央。空串 = 不画。
    void set_thumb_icon(std::string codicon_name) {
        thumb_icon_ = std::move(codicon_name);
        mark_dirty(DirtyFlags::Paint);
    }

    // 缩略图色调 hint（0..N）—— 用 set_thumb_palette 传入的颜色列表里取一个。
    void set_color_hint(int hint) noexcept {
        color_hint_ = hint;
        mark_dirty(DirtyFlags::Paint);
    }

    // 右下角小锁
    void set_locked(bool locked) noexcept {
        locked_ = locked;
        mark_dirty(DirtyFlags::Paint);
    }

    // 右下角共享头像（≤3 个直接显示，超出 +N）
    void set_shared_avatars(std::vector<AvatarGroup::Entry> entries) {
        shared_ = std::move(entries);
        avatar_group_->set_entries(shared_);
        mark_dirty(DirtyFlags::Paint);
    }

    // ───── 交互 ─────

    void set_clickable(bool clickable) noexcept { clickable_ = clickable; }

    [[nodiscard]] Signal<>& on_clicked() noexcept { return clicked_; }

    void on_click(PointF) override {
        if (enabled() && clickable_) clicked_.emit();
    }

    // ───── 配色 ─────

    void set_palette(Color card_bg, Color card_hover_bg, Color border, Color thumb_body,
                     Color thumb_tab, Color thumb_layer, Color text_primary, Color text_muted,
                     Color icon_color, Color lock_color, Color avatar_ring) noexcept {
        card_bg_ = card_bg;
        card_hover_bg_ = card_hover_bg;
        border_ = border;
        thumb_body_ = thumb_body;
        thumb_tab_ = thumb_tab;
        thumb_layer_ = thumb_layer;
        text_primary_ = text_primary;
        text_muted_ = text_muted;
        icon_color_ = icon_color;
        lock_color_ = lock_color;
        avatar_ring_color_ = avatar_ring;
        avatar_group_->set_ring(avatar_ring, 2.0F);
        mark_dirty(DirtyFlags::Paint);
    }

    // 缩略图色调候选（按 color_hint 取 mod）。给一组"略带颜色"的封面色，
    // 让相邻文件夹卡之间不至于全是黑灰一个调。
    void set_thumb_color_hints(std::vector<Color> hints) {
        thumb_hints_ = std::move(hints);
        mark_dirty(DirtyFlags::Paint);
    }

    // ───── 布局 ─────

    void relayout() {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 共享头像组：右下角，每个 18px，总宽自动算
        // 距右/下边距与卡片 paint() 中的 kPad 一致（12px）
        constexpr float kAvSize = 18.0F;
        constexpr float kEdge = 12.0F;
        const float total_w = avatar_group_->intrinsic_width(kAvSize);
        if (total_w > 0.0F) {
            avatar_group_->set_frame(RectF{
                f.x + f.width - kEdge - total_w,
                f.y + f.height - kEdge - kAvSize,
                total_w,
                kAvSize,
            });
        } else {
            avatar_group_->set_frame(RectF{0, 0, 0, 0});
        }
    }

    // ───── 绘制 ─────

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 卡片底
        const Color bg = (clickable_ && hovered()) ? card_hover_bg_ : card_bg_;
        if (bg.a > 0.001F) {
            context.fill_rounded_rect(f, bg, 14.0F);
        }
        if (border_.a > 0.001F) {
            context.stroke_rounded_rect(f, border_, 14.0F, 1.0F);
        }

        // ── 缩略图区域：占上半（约 66%）────
        constexpr float kPad = 12.0F;
        const float thumb_top = f.y + kPad;
        const float thumb_bottom = f.y + f.height * 0.66F;
        const float thumb_h = thumb_bottom - thumb_top;
        const float thumb_w = f.width - kPad * 2.0F;
        const RectF thumb_body{f.x + kPad, thumb_top, thumb_w, thumb_h};

        const float r = 8.0F;

        // 1) 主体（先画，作为底）—— 颜色可被 color_hint 微调，让相邻卡有差异
        const Color body_tinted = mix_(thumb_body_, pick_thumb_color_(), 0.18F);
        context.fill_rounded_rect(thumb_body, body_tinted, r);

        // 2) 文件夹小标签（后画 = 在 body 之上）：两层错落，
        //    完全落在 thumb 区域内，不超出卡片 padding；颜色统一来自 thumb_tab_ /
        //    thumb_layer_，不再受 color_hint 影响。
        //    每层加 1px 描边（用 thumb_layer_ / 卡片 border 色），让浅色主题下
        //    tab 与 body 不再融化（dark 下也无害，对比度刚好不抢戏）。
        constexpr float kTabH = 14.0F;
        constexpr float kTabR = 5.0F;
        const RectF tab_back{
            thumb_body.x + thumb_w * 0.08F,
            thumb_top + 8.0F,
            thumb_w * 0.34F,
            kTabH,
        };
        const RectF tab_front{
            thumb_body.x + thumb_w * 0.16F,
            thumb_top + 4.0F,
            thumb_w * 0.36F,
            kTabH,
        };
        // 后层（左偏，深一点）
        context.fill_rounded_rect(tab_back, thumb_layer_, kTabR);
        if (border_.a > 0.001F) {
            context.stroke_rounded_rect(tab_back, border_, kTabR, 1.0F);
        }
        // 前层（右偏，浅一点）
        context.fill_rounded_rect(tab_front, thumb_tab_, kTabR);
        if (border_.a > 0.001F) {
            context.stroke_rounded_rect(tab_front, border_, kTabR, 1.0F);
        }

        // 3) 中央 codicon（如果有）—— 字号收一点避免抢占 tab 视觉
        if (!thumb_icon_.empty()) {
            const auto cp = foundation::codicon_codepoint_for(thumb_icon_);
            if (cp.has_value()) {
                std::string utf8;
                encode_codicon_(*cp, utf8);
                const float size = std::min(thumb_h, thumb_w) * 0.26F;
                context.draw_text_with_font(
                    thumb_body, std::move(utf8),
                    std::string(foundation::kCodiconFontFamily),
                    icon_color_, TextAlignment::Center, size, false);
            }
        }

        // ── 标题 + 副文 ────
        const float text_top = thumb_bottom + 12.0F;
        const float text_w = f.width - kPad * 2.0F;
        context.draw_text(
            RectF{f.x + kPad, text_top, text_w, 20.0F},
            title_, text_primary_, TextAlignment::Leading, 14.0F, true);
        context.draw_text(
            RectF{f.x + kPad, text_top + 22.0F, text_w * 0.55F, 16.0F},
            subtitle_, text_muted_, TextAlignment::Leading, 12.0F, false);

        // ── 右下角小锁 ────
        if (locked_) {
            const float icon_size = 14.0F;
            const auto cp = foundation::codicon_codepoint_for("lock");
            if (cp.has_value()) {
                std::string utf8;
                encode_codicon_(*cp, utf8);
                context.draw_text_with_font(
                    RectF{f.x + f.width - kPad - icon_size,
                          f.y + f.height - kPad - icon_size,
                          icon_size, icon_size},
                    std::move(utf8),
                    std::string(foundation::kCodiconFontFamily),
                    lock_color_, TextAlignment::Center, icon_size, false);
            }
        }

        // ── 右上角 "..." 按钮（仅 hover 显示）────
        if (clickable_ && hovered()) {
            const float dot_r = 1.5F;
            const float dot_y = f.y + kPad + 4.0F;
            for (int i = 0; i < 3; ++i) {
                const float dot_x = f.x + f.width - kPad - 14.0F + static_cast<float>(i) * 5.0F;
                context.fill_ellipse(
                    RectF{dot_x, dot_y, dot_r * 2.0F, dot_r * 2.0F},
                    text_muted_);
            }
        }
    }

private:
    [[nodiscard]] Color pick_thumb_color_() const noexcept {
        if (thumb_hints_.empty()) return thumb_layer_;
        const std::size_t idx = static_cast<std::size_t>(
            std::max(0, color_hint_)) % thumb_hints_.size();
        return thumb_hints_[idx];
    }

    static Color mix_(Color a, Color b, float t) noexcept {
        const float k = std::clamp(t, 0.0F, 1.0F);
        return Color{
            a.r * (1.0F - k) + b.r * k,
            a.g * (1.0F - k) + b.g * k,
            a.b * (1.0F - k) + b.b * k,
            a.a,
        };
    }

    static void encode_codicon_(char32_t code, std::string& out) {
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

    AvatarGroup* avatar_group_ {nullptr};

    std::string title_ {"Untitled"};
    std::string subtitle_ {};
    std::string thumb_icon_ {};
    int color_hint_ {0};
    bool locked_ {false};
    bool clickable_ {true};
    std::vector<AvatarGroup::Entry> shared_ {};

    Color card_bg_ {};
    Color card_hover_bg_ {};
    Color border_ {};
    Color thumb_body_ {};
    Color thumb_tab_ {};
    Color thumb_layer_ {};
    Color text_primary_ {};
    Color text_muted_ {};
    Color icon_color_ {};
    Color lock_color_ {};
    Color avatar_ring_color_ {};
    std::vector<Color> thumb_hints_ {};

    Signal<> clicked_ {};
};

}  // namespace hui
