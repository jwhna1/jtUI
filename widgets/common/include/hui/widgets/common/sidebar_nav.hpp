#pragma once

// SidebarNav —— 完整侧边导航栏：用户区 + 搜索 + 主菜单 + 辅助菜单 + 底部信息卡。
// 结构（自上而下）：
//   ┌─────────────────────────┐
//   │ ◉  Olivia Rhye    ⇅      │  user_area
//   │ [🔍 Search          F ]  │  search
//   │                         │
//   │ ◐ My Folders            │  main_items (active 高亮)
//   │ ☆ Favorite              │
//   │ ⏳ History               │
//   │ ＋ Invite                │
//   │                         │
//   │ ⓘ Support               │  secondary_items
//   │ ⚙ Settings              │
//   │                         │
//   │ ┌─────────────────────┐ │  storage_card
//   │ │ Used space          │ │
//   │ │ 80% of plan         │ │
//   │ │ ▓▓▓▓▓▓▓▓░░          │ │
//   │ │ Upgrade plan        │ │
//   │ └─────────────────────┘ │
//   └─────────────────────────┘
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <cstddef>
#include <memory>
#include <optional>
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
#include "hui/theme/theme.hpp"
#include "hui/widgets/common/avatar.hpp"
#include "hui/widgets/common/progress_bar.hpp"
#include "hui/widgets/common/search_input.hpp"

namespace hui {

namespace sidebar_internal {

// 内部 nav item 行：icon + label + 可选右侧 chevron。Widget 自管 paint，无子 widget。
class NavItemButton : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override { return "NavItemButton"; }

    void set_icon(std::string name) {
        icon_ = std::move(name);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_label(std::string label) {
        label_ = std::move(label);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_active(bool active) noexcept {
        if (active_ == active) return;
        active_ = active;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_colors(Color text_normal, Color text_active, Color icon_normal, Color icon_active,
                    Color active_bg, Color hover_bg, Color accent_bar) noexcept {
        text_normal_ = text_normal;
        text_active_ = text_active;
        icon_normal_ = icon_normal;
        icon_active_ = icon_active;
        active_bg_ = active_bg;
        hover_bg_ = hover_bg;
        accent_bar_ = accent_bar;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Signal<>& on_clicked() noexcept { return clicked_; }

    void on_click(PointF) override {
        if (enabled()) clicked_.emit();
    }

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 行底色
        // v1.19 (2026-05-14): 旧实现 `Color bg = Color{}` 默认 alpha=1，inactive
        // 非 hover 时 bg=不透明黑，反复画出黑底。在浅色主题下尤其刺眼（黑底压
        // 浅色 sidebar）。改成 alpha=0 透明色作 fallback，让透明态正确跳过 fill。
        Color bg{0.0F, 0.0F, 0.0F, 0.0F};
        if (active_) {
            bg = active_bg_;
        } else if (hovered()) {
            bg = hover_bg_;
        }
        if (bg.a > 0.001F) {
            context.fill_rounded_rect(f, bg, 8.0F);
        }

        // active 左侧 accent 条
        if (active_ && accent_bar_.a > 0.001F) {
            const float bar_h = f.height * 0.5F;
            const float bar_w = 3.0F;
            context.fill_rounded_rect(
                RectF{f.x, f.y + (f.height - bar_h) * 0.5F, bar_w, bar_h},
                accent_bar_, bar_w * 0.5F);
        }

        // 图标
        const float pad_left = 14.0F;
        const float icon_size = 16.0F;
        const float icon_x = f.x + pad_left;
        const float icon_y = f.y + (f.height - icon_size) * 0.5F;
        const Color icon_color = active_ ? icon_active_ : icon_normal_;
        const auto cp = foundation::codicon_codepoint_for(icon_);
        if (cp.has_value()) {
            std::string utf8;
            encode_codicon_(*cp, utf8);
            context.draw_text_with_font(
                RectF{icon_x, icon_y, icon_size, icon_size},
                std::move(utf8),
                std::string(foundation::kCodiconFontFamily),
                icon_color, TextAlignment::Center, icon_size, false);
        } else {
            // 未知图标：占位描边方
            context.stroke_rect(RectF{icon_x, icon_y, icon_size, icon_size}, icon_color, 1.0F);
        }

        // 标签
        const float label_x = icon_x + icon_size + 12.0F;
        const float label_w = f.x + f.width - 12.0F - label_x;
        const Color text_color = active_ ? text_active_ : text_normal_;
        context.draw_text(
            RectF{label_x, f.y, label_w, f.height},
            label_, text_color, TextAlignment::Leading, 14.0F, active_);
    }

private:
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

    std::string icon_ {};
    std::string label_ {};
    bool active_ {false};

    Color text_normal_ {};
    Color text_active_ {};
    Color icon_normal_ {};
    Color icon_active_ {};
    Color active_bg_ {};
    Color hover_bg_ {};
    Color accent_bar_ {};

    Signal<> clicked_ {};
};

}  // namespace sidebar_internal

class SidebarNav : public Widget {
public:
    struct Item {
        std::string label;
        std::string icon;  // codicon 名
    };

    SidebarNav() {
        // 头像
        auto avatar = std::make_unique<Avatar>();
        avatar_ = avatar.get();
        avatar_->set_status(AvatarStatus::Online);
        append_child(std::move(avatar));

        // 搜索框
        auto search = std::make_unique<SearchInput>("Search");
        search->set_shortcut_label("F");
        search_ = search.get();
        append_child(std::move(search));

        // 进度条
        auto progress = std::make_unique<ProgressBar>();
        progress->set_thickness(6.0F);
        progress->set_value(0.8F);
        progress_ = progress.get();
        append_child(std::move(progress));
    }

    [[nodiscard]] std::string_view type_name() const noexcept override { return "SidebarNav"; }

    // ───── 配置 ─────────────────────────────────────────────────────

    void set_user(std::string name) {
        user_name_ = std::move(name);
        avatar_->set_name(user_name_);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_user_status(AvatarStatus s) noexcept {
        user_status_ = s;
        avatar_->set_status(s);
    }

    // 主菜单。set 后 active_index 自动归零。
    void set_main_items(std::vector<Item> items) {
        main_items_ = std::move(items);
        rebuild_main_buttons_();
        set_active(0);
    }

    void set_secondary_items(std::vector<Item> items) {
        secondary_items_ = std::move(items);
        rebuild_secondary_buttons_();
    }

    // 高亮指定主菜单项。
    void set_active(std::size_t idx) noexcept {
        if (active_ == idx) return;
        active_ = idx;
        for (std::size_t i = 0; i < main_buttons_.size(); ++i) {
            main_buttons_[i]->set_active(i == active_);
        }
    }

    [[nodiscard]] std::size_t active_index() const noexcept { return active_; }

    void set_storage_label(std::string title, std::string detail) {
        storage_title_ = std::move(title);
        storage_detail_ = std::move(detail);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_storage_used(float ratio) noexcept {
        progress_->set_value(ratio);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_upgrade_label(std::string s) {
        upgrade_label_ = std::move(s);
        mark_dirty(DirtyFlags::Paint);
    }

    // ───── 主题色（可由业务从 brand_tokens 注入） ────────────

    void set_palette(Color bg, Color border, Color text_primary, Color text_secondary,
                     Color text_muted, Color accent, Color accent_soft, Color accent_fg,
                     Color hover_bg, Color storage_card_bg) noexcept {
        bg_ = bg;
        border_ = border;
        text_primary_ = text_primary;
        text_secondary_ = text_secondary;
        text_muted_ = text_muted;
        accent_ = accent;
        accent_soft_ = accent_soft;
        accent_fg_ = accent_fg;
        hover_bg_ = hover_bg;
        storage_card_bg_ = storage_card_bg;
        // 应用到所有按钮
        for (auto* b : main_buttons_) {
            b->set_colors(text_secondary_, accent_, text_muted_, accent_,
                          accent_soft_, hover_bg_, accent_);
        }
        for (auto* b : secondary_buttons_) {
            b->set_colors(text_secondary_, accent_, text_muted_, accent_,
                          accent_soft_, hover_bg_, accent_);
        }
        // 进度条
        // (ProgressBar 用 theme tone，颜色靠 set_palette 间接走，这里设个轨道色)
        mark_dirty(DirtyFlags::Paint);
    }

    // 让 SearchInput 用品牌色
    void set_search_palette(Color bg, Color border, Color icon, Color badge_bg, Color badge_fg) noexcept {
        search_->set_background_color(bg);
        search_->set_border(border, 1.0F);
        search_->set_icon_color(icon);
        search_->set_shortcut_badge_color(badge_bg, badge_fg);
    }

    // ───── 信号 ─────────────────────────────────────────────────────

    [[nodiscard]] Signal<std::size_t>& on_main_clicked() noexcept { return main_clicked_; }
    [[nodiscard]] Signal<std::size_t>& on_secondary_clicked() noexcept { return secondary_clicked_; }
    [[nodiscard]] Signal<>& on_upgrade_clicked() noexcept { return upgrade_clicked_; }
    [[nodiscard]] SearchInput* search_input() noexcept { return search_; }

    // ───── 布局 ─────────────────────────────────────────────────────

    // jtUI 惯例：业务 set_frame 后调一次同步全部子 widget。
    // 布局策略：自上而下流式堆叠（用户/搜索/主菜单/辅助菜单/存储卡），不靠底对齐。
    // 这样空间紧张时不会出现"主菜单与辅助菜单互相挤压"的视觉错乱。
    void relayout() {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        constexpr float kSidePad = 16.0F;
        constexpr float kAvatarSize = 36.0F;
        constexpr float kSearchH = 36.0F;
        constexpr float kItemH = 36.0F;
        constexpr float kItemGap = 2.0F;
        constexpr float kStorageH = 110.0F;
        constexpr float kSectionGap = 18.0F;

        const float content_w = f.width - kSidePad * 2.0F;
        float y = f.y + 20.0F;

        // 用户区：avatar
        avatar_->set_frame(RectF{f.x + kSidePad, y, kAvatarSize, kAvatarSize});
        user_name_y_ = y;
        user_name_x_ = f.x + kSidePad + kAvatarSize + 12.0F;
        user_name_w_ = content_w - kAvatarSize - 12.0F;
        user_name_h_ = kAvatarSize;
        y += kAvatarSize + 16.0F;

        // 搜索
        search_->set_frame(RectF{f.x + kSidePad, y, content_w, kSearchH});
        search_->relayout();
        y += kSearchH + kSectionGap;

        // 主菜单
        for (auto* b : main_buttons_) {
            b->set_frame(RectF{f.x + kSidePad, y, content_w, kItemH});
            y += kItemH + kItemGap;
        }

        // 辅助菜单（紧跟主菜单，留 section gap）
        if (!secondary_buttons_.empty()) {
            y += kSectionGap - kItemGap;
            for (auto* b : secondary_buttons_) {
                b->set_frame(RectF{f.x + kSidePad, y, content_w, kItemH});
                y += kItemH + kItemGap;
            }
        }

        // 存储卡（紧跟辅助菜单，留 section gap）
        if (!storage_title_.empty()) {
            y += kSectionGap;
            storage_rect_ = RectF{f.x + kSidePad, y, content_w, kStorageH};
            const float pb_y = y + 64.0F;
            progress_->set_frame(RectF{storage_rect_.x + 14.0F, pb_y,
                                       storage_rect_.width - 28.0F, 6.0F});
        } else {
            storage_rect_ = RectF{0, 0, 0, 0};
            progress_->set_frame(RectF{0, 0, 0, 0});
        }
    }

    // ───── 绘制 ─────────────────────────────────────────────────────

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 整体底色 + 右侧 1px 分割线
        if (bg_.a > 0.001F) {
            context.fill_rect(f, bg_);
        }
        if (border_.a > 0.001F) {
            context.line(PointF{f.x + f.width - 1.0F, f.y},
                         PointF{f.x + f.width - 1.0F, f.y + f.height},
                         border_, 1.0F);
        }

        // 用户名 + 状态
        const Color name_color = text_primary_.a > 0.001F ? text_primary_ : Color{0.92F, 0.92F, 0.92F, 1.0F};
        context.draw_text(
            RectF{user_name_x_, user_name_y_, user_name_w_, user_name_h_ * 0.6F},
            user_name_, name_color, TextAlignment::Leading, 13.0F, true);
        const Color sub_color = text_muted_.a > 0.001F ? text_muted_ : Color{0.55F, 0.55F, 0.55F, 1.0F};
        context.draw_text(
            RectF{user_name_x_, user_name_y_ + user_name_h_ * 0.55F, user_name_w_, user_name_h_ * 0.45F},
            "Personal", sub_color, TextAlignment::Leading, 11.0F, false);

        // 存储卡背景
        if (storage_card_bg_.a > 0.001F) {
            context.fill_rounded_rect(storage_rect_, storage_card_bg_, 12.0F);
        }
        if (border_.a > 0.001F) {
            context.stroke_rounded_rect(storage_rect_, border_, 12.0F, 1.0F);
        }
        // 存储卡内文字
        const Color title_color = text_primary_.a > 0.001F ? text_primary_ : Color{0.92F, 0.92F, 0.92F, 1.0F};
        context.draw_text(
            RectF{storage_rect_.x + 16.0F, storage_rect_.y + 14.0F, storage_rect_.width - 32.0F, 18.0F},
            storage_title_, title_color, TextAlignment::Leading, 13.0F, true);
        const Color detail_color = text_secondary_.a > 0.001F ? text_secondary_ : Color{0.65F, 0.65F, 0.65F, 1.0F};
        context.draw_text(
            RectF{storage_rect_.x + 16.0F, storage_rect_.y + 32.0F, storage_rect_.width - 32.0F, 32.0F},
            storage_detail_, detail_color, TextAlignment::Leading, 11.0F, false);

        // Upgrade 链接
        const Color up_color = accent_.a > 0.001F ? accent_ : Color{0.39F, 0.96F, 0.55F, 1.0F};
        context.draw_text(
            RectF{storage_rect_.x + 16.0F, storage_rect_.y + storage_rect_.height - 28.0F,
                  storage_rect_.width - 32.0F, 18.0F},
            upgrade_label_, up_color, TextAlignment::Leading, 12.0F, true);
    }

private:
    void rebuild_main_buttons_() {
        // 移除旧主按钮（必须保留 avatar/search/progress 等其它子）
        // 简单做法：clear 全部子，再按顺序重新 append。
        // 由于 jtUI children 是 unique_ptr 容器，clear_children 会销毁所有子。
        // 我们重建 avatar/search/progress 太麻烦 —— 改方案：维护 main_buttons_/secondary_buttons_
        // 列表，rebuild 时只销毁这些 raw pointer 对应的 child。
        // 但 Node 没暴露 erase_child(idx)（v0.3 才会有）。
        // 折衷：full clear + rebuild 全部。
        full_rebuild_();
    }

    void rebuild_secondary_buttons_() {
        full_rebuild_();
    }

    void full_rebuild_() {
        // 把 search/progress 的可变状态备份（avatar/status 已存在 SidebarNav 字段中）
        const std::string saved_search_text = search_ ? search_->text() : std::string{};
        const std::string saved_search_placeholder =
            search_ ? search_->text_input()->placeholder() : std::string{"Search"};
        const std::string saved_search_shortcut =
            search_ ? search_->shortcut_label() : std::string{"F"};
        const float saved_progress = progress_ ? progress_->value() : 0.8F;

        clear_children();
        main_buttons_.clear();
        secondary_buttons_.clear();

        auto avatar = std::make_unique<Avatar>(user_name_);
        avatar_ = avatar.get();
        avatar_->set_status(user_status_);
        append_child(std::move(avatar));

        auto search = std::make_unique<SearchInput>(saved_search_placeholder);
        search->set_shortcut_label(saved_search_shortcut);
        search->set_text(saved_search_text);
        search_ = search.get();
        append_child(std::move(search));

        // 主菜单按钮
        for (std::size_t i = 0; i < main_items_.size(); ++i) {
            auto btn = std::make_unique<sidebar_internal::NavItemButton>();
            btn->set_label(main_items_[i].label);
            btn->set_icon(main_items_[i].icon);
            btn->set_active(i == active_);
            btn->set_colors(text_secondary_, accent_, text_muted_, accent_,
                            accent_soft_, hover_bg_, accent_);
            const std::size_t idx = i;
            btn->on_clicked().connect([this, idx]() {
                set_active(idx);
                main_clicked_.emit(idx);
            });
            main_buttons_.push_back(btn.get());
            append_child(std::move(btn));
        }

        // 辅助菜单按钮
        for (std::size_t i = 0; i < secondary_items_.size(); ++i) {
            auto btn = std::make_unique<sidebar_internal::NavItemButton>();
            btn->set_label(secondary_items_[i].label);
            btn->set_icon(secondary_items_[i].icon);
            btn->set_colors(text_secondary_, accent_, text_muted_, accent_,
                            accent_soft_, hover_bg_, accent_);
            const std::size_t idx = i;
            btn->on_clicked().connect([this, idx]() {
                secondary_clicked_.emit(idx);
            });
            secondary_buttons_.push_back(btn.get());
            append_child(std::move(btn));
        }

        // 进度条
        auto progress = std::make_unique<ProgressBar>();
        progress->set_thickness(6.0F);
        progress->set_value(saved_progress);
        progress_ = progress.get();
        append_child(std::move(progress));

        mark_dirty(DirtyFlags::Layout);
    }

    // children
    Avatar* avatar_ {nullptr};
    SearchInput* search_ {nullptr};
    ProgressBar* progress_ {nullptr};
    std::vector<sidebar_internal::NavItemButton*> main_buttons_ {};
    std::vector<sidebar_internal::NavItemButton*> secondary_buttons_ {};

    // data
    std::string user_name_ {};
    AvatarStatus user_status_ {AvatarStatus::Online};
    std::vector<Item> main_items_ {};
    std::vector<Item> secondary_items_ {};
    std::size_t active_ {0};

    std::string storage_title_ {"Used space"};
    std::string storage_detail_ {"You have used 80% of your available space."};
    std::string upgrade_label_ {"Upgrade plan"};

    // palette
    Color bg_ {};
    Color border_ {};
    Color text_primary_ {};
    Color text_secondary_ {};
    Color text_muted_ {};
    Color accent_ {};
    Color accent_soft_ {};
    Color accent_fg_ {};
    Color hover_bg_ {};
    Color storage_card_bg_ {};

    // layout cache
    float user_name_x_ {0.0F};
    float user_name_y_ {0.0F};
    float user_name_w_ {0.0F};
    float user_name_h_ {0.0F};
    RectF storage_rect_ {};

    // signals
    Signal<std::size_t> main_clicked_ {};
    Signal<std::size_t> secondary_clicked_ {};
    Signal<> upgrade_clicked_ {};
};

}  // namespace hui
