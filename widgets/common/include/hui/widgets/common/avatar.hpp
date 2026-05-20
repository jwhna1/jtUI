#pragma once

// Avatar / AvatarGroup —— 圆形头像。v1 不做图片加载（jtUI 没有图片资源管线），
// 用 initials + 哈希背景色 + 可选 status dot。需要真实图片可后续接 PixelBuffer。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

enum class AvatarShape {
    Circle,
    Squircle,  // 圆角方块（用于品牌 logo / app 图标场景）
};

enum class AvatarStatus {
    None,
    Online,    // 绿点
    Away,      // 黄点
    Busy,      // 红点
    Offline,   // 灰点
};

namespace avatar_detail {

// 8 个预设柔和色，按 initials 哈希取一个。配色避开蓝紫渐变，
// 用饱和度较低的"工程师友好"色。
inline const std::vector<Color>& palette() noexcept {
    static const std::vector<Color> p = {
        Color::from_hex("#5C6B7A"),  // slate
        Color::from_hex("#4F8C73"),  // sage
        Color::from_hex("#A0612A"),  // amber
        Color::from_hex("#8C4F73"),  // mauve
        Color::from_hex("#3F7384"),  // teal
        Color::from_hex("#7B5A2E"),  // bronze
        Color::from_hex("#6B6B3A"),  // olive
        Color::from_hex("#5A4F8C"),  // indigo（不是品牌主色，单纯 hash 槽位）
    };
    return p;
}

inline std::size_t hash_text(std::string_view s) noexcept {
    std::size_t h = 1469598103934665603ULL;  // FNV offset
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

// 从 full name 提取最多两个首字母大写。"Olivia Rhye" → "OR", "ali" → "A".
inline std::string make_initials(std::string_view full) noexcept {
    std::string out;
    bool at_word_start = true;
    for (char c : full) {
        if (c == ' ' || c == '\t' || c == '_' || c == '-') {
            at_word_start = true;
            continue;
        }
        if (at_word_start && out.size() < 2) {
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            at_word_start = false;
        }
    }
    return out.empty() ? std::string{"?"} : out;
}

inline Color status_color(AvatarStatus s) noexcept {
    switch (s) {
    case AvatarStatus::Online:  return Color::from_hex("#3DD68C");
    case AvatarStatus::Away:    return Color::from_hex("#F5C13D");
    case AvatarStatus::Busy:    return Color::from_hex("#F56565");
    case AvatarStatus::Offline: return Color::from_hex("#6A6A6A");
    case AvatarStatus::None:    break;
    }
    return Color{};
}

}  // namespace avatar_detail

class Avatar : public Widget {
public:
    Avatar() = default;
    explicit Avatar(std::string name)
        : name_(std::move(name)),
          initials_(avatar_detail::make_initials(name_)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override { return "Avatar"; }

    // 用 full name 自动派生 initials + 背景色 hash。
    void set_name(std::string name) {
        name_ = std::move(name);
        if (!initials_override_) {
            initials_ = avatar_detail::make_initials(name_);
        }
        mark_dirty(DirtyFlags::Paint);
    }

    // 显式指定 initials（覆盖 name 自动派生）
    void set_initials(std::string s) {
        initials_ = std::move(s);
        initials_override_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    // 显式指定背景色（覆盖 hash 派生色）
    void set_background_color(Color c) noexcept {
        bg_override_ = c;
        bg_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_text_color(Color c) noexcept {
        text_color_ = c;
        text_color_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_shape(AvatarShape shape) noexcept {
        if (shape_ == shape) return;
        shape_ = shape;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_status(AvatarStatus status) noexcept {
        if (status_ == status) return;
        status_ = status;
        mark_dirty(DirtyFlags::Paint);
    }

    // ring 用于 AvatarGroup 重叠场景，画一圈和容器底色一致的描边把头像"切"开。
    // alpha=0 视作不画。
    void set_ring(Color color, float thickness = 2.0F) noexcept {
        ring_color_ = color;
        ring_thickness_ = thickness;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] const std::string& initials() const noexcept { return initials_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void paint(PaintContext& context) const override {
        const RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const Color bg = resolve_bg();
        const float radius = (shape_ == AvatarShape::Circle) ? f.height * 0.5F : f.height * 0.28F;

        if (shape_ == AvatarShape::Circle) {
            context.fill_ellipse(f, bg);
        } else {
            context.fill_rounded_rect(f, bg, radius);
        }

        const Color fg = text_color_overridden_
                             ? text_color_
                             : Color::from_hex("#FFFFFF");
        const float font_size = f.height * 0.42F;
        context.draw_text(f, initials_, fg, TextAlignment::Center, font_size, true);

        if (ring_thickness_ > 0.0F && ring_color_.a > 0.001F) {
            if (shape_ == AvatarShape::Circle) {
                context.stroke_ellipse(f, ring_color_, ring_thickness_);
            } else {
                context.stroke_rounded_rect(f, ring_color_, radius, ring_thickness_);
            }
        }

        if (status_ != AvatarStatus::None) {
            const float dot = f.height * 0.28F;
            const RectF dot_rect{
                f.x + f.width - dot,
                f.y + f.height - dot,
                dot,
                dot,
            };
            // 先画外圈（和 ring 同色或主背景色），再画状态色，模拟"挖洞"效果
            const Color outer = (ring_color_.a > 0.001F) ? ring_color_ : Color::from_hex("#0A0A0A");
            context.fill_ellipse(dot_rect, outer);
            const RectF inner{
                dot_rect.x + 2.0F,
                dot_rect.y + 2.0F,
                dot_rect.width - 4.0F,
                dot_rect.height - 4.0F,
            };
            context.fill_ellipse(inner, avatar_detail::status_color(status_));
        }
    }

private:
    [[nodiscard]] Color resolve_bg() const noexcept {
        if (bg_overridden_) return bg_override_;
        const auto& p = avatar_detail::palette();
        const std::string& key = !name_.empty() ? name_ : initials_;
        return p[avatar_detail::hash_text(key) % p.size()];
    }

    std::string name_ {};
    std::string initials_ {"?"};
    bool initials_override_ {false};
    AvatarShape shape_ {AvatarShape::Circle};
    AvatarStatus status_ {AvatarStatus::None};
    Color bg_override_ {};
    bool bg_overridden_ {false};
    Color text_color_ {};
    bool text_color_overridden_ {false};
    Color ring_color_ {};
    float ring_thickness_ {0.0F};
};

// 多个头像横向重叠展示，max 之外用 "+N" 圆显示溢出。
// 不持有 Avatar widget —— 直接 paint 一组圆，省一层 widget 树开销。
class AvatarGroup : public Widget {
public:
    struct Entry {
        std::string name;       // 用于派生 initials & 哈希色
        AvatarStatus status {AvatarStatus::None};
    };

    [[nodiscard]] std::string_view type_name() const noexcept override { return "AvatarGroup"; }

    void set_entries(std::vector<Entry> entries) {
        entries_ = std::move(entries);
        mark_dirty(DirtyFlags::Paint);
    }

    void set_max_visible(std::size_t max) noexcept {
        if (max_visible_ == max) return;
        max_visible_ = max;
        mark_dirty(DirtyFlags::Paint);
    }

    // 重叠比例：0 = 完全不重叠，0.5 = 重叠半个直径。默认 0.32。
    void set_overlap(float ratio) noexcept {
        overlap_ = ratio;
        mark_dirty(DirtyFlags::Paint);
    }

    // ring 同 Avatar，给重叠头像之间画一圈底色描边
    void set_ring(Color color, float thickness = 2.0F) noexcept {
        ring_color_ = color;
        ring_thickness_ = thickness;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_extra_color(Color bg, Color fg) noexcept {
        extra_bg_ = bg;
        extra_fg_ = fg;
        extra_color_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void paint(PaintContext& context) const override {
        if (entries_.empty()) return;
        const RectF f = frame();
        const float size = f.height;
        if (size <= 0.0F) return;

        const std::size_t total = entries_.size();
        const std::size_t shown = (total > max_visible_) ? max_visible_ : total;
        const std::size_t extra = total - shown;
        const float step = size * (1.0F - overlap_);

        for (std::size_t i = 0; i < shown; ++i) {
            const RectF cell{f.x + step * static_cast<float>(i), f.y, size, size};
            const auto& e = entries_[i];
            const std::string init = avatar_detail::make_initials(e.name);
            const auto& palette = avatar_detail::palette();
            const Color bg = palette[avatar_detail::hash_text(e.name) % palette.size()];

            context.fill_ellipse(cell, bg);
            context.draw_text(cell, init, Color::from_hex("#FFFFFF"),
                               TextAlignment::Center, size * 0.42F, true);

            if (ring_thickness_ > 0.0F && ring_color_.a > 0.001F) {
                context.stroke_ellipse(cell, ring_color_, ring_thickness_);
            }

            if (e.status != AvatarStatus::None) {
                const float dot = size * 0.28F;
                const RectF dot_rect{cell.x + cell.width - dot, cell.y + cell.height - dot, dot, dot};
                const Color outer = (ring_color_.a > 0.001F) ? ring_color_ : Color::from_hex("#0A0A0A");
                context.fill_ellipse(dot_rect, outer);
                context.fill_ellipse(
                    RectF{dot_rect.x + 2.0F, dot_rect.y + 2.0F, dot_rect.width - 4.0F, dot_rect.height - 4.0F},
                    avatar_detail::status_color(e.status));
            }
        }

        if (extra > 0) {
            const RectF cell{f.x + step * static_cast<float>(shown), f.y, size, size};
            const Color bg = extra_color_overridden_ ? extra_bg_ : Color::from_hex("#262626");
            const Color fg = extra_color_overridden_ ? extra_fg_ : Color::from_hex("#E8E8E8");
            context.fill_ellipse(cell, bg);
            std::string label = "+" + std::to_string(extra);
            context.draw_text(cell, label, fg, TextAlignment::Center, size * 0.36F, true);
            if (ring_thickness_ > 0.0F && ring_color_.a > 0.001F) {
                context.stroke_ellipse(cell, ring_color_, ring_thickness_);
            }
        }
    }

    // 计算给定单个头像 size 时整组渲染所需的总宽度，便于父级布局预留位置。
    [[nodiscard]] float intrinsic_width(float per_size) const noexcept {
        if (entries_.empty()) return 0.0F;
        const std::size_t total = entries_.size();
        const std::size_t shown = (total > max_visible_) ? max_visible_ : total;
        const std::size_t cells = shown + (total > max_visible_ ? 1U : 0U);
        if (cells == 0) return 0.0F;
        const float step = per_size * (1.0F - overlap_);
        return per_size + step * static_cast<float>(cells - 1U);
    }

private:
    std::vector<Entry> entries_ {};
    std::size_t max_visible_ {3};
    float overlap_ {0.32F};
    Color ring_color_ {};
    float ring_thickness_ {0.0F};
    Color extra_bg_ {};
    Color extra_fg_ {};
    bool extra_color_overridden_ {false};
};

}  // namespace hui
