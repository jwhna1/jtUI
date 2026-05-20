// Package: jtUI gallery
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// Button Styles section -- 展示 5 套现代设计语言的按钮风格 (v2 风, 2026-05-14).
//
// 行布局:
//   Row 1: Pill 胶囊         5 个 (Filled + Pill shape, 全 tone)
//   Row 2: Tonal 柔和         5 个 (Tonal variant, 全 tone)
//   Row 3: Gradient 渐变      5 个 (Gradient variant 同色明暗, 全 tone)
//   Row 4: 黑金 + 流光         3 个 (GradientBlackGold + border beam)
//   Row 5: Icon-only         8 个 (Circle 4 + Square 4, 配 codicon)
//   Row 6: Group 分段控制      3 个连续按钮 (视图切换 demo)
//
// 实际是 6 行 ("5 套" 中 Group 是组合用法, 单独成行展示)。
// Black Gold 行只有 1-2 个按钮开 border beam, 避免每帧多个 widget 一起 dirty。
#include "sections/button_styles.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

namespace {

constexpr std::array<jtui::theme::Tone, 5> kAllTones = {
    jtui::theme::Tone::Accent,
    jtui::theme::Tone::Success,
    jtui::theme::Tone::Warning,
    jtui::theme::Tone::Danger,
    jtui::theme::Tone::Info,
};

// 5 个 tone 在 EN/ZH 下的标签
const std::array<std::pair<const char*, const char*>, 5>& tone_labels() {
    static const std::array<std::pair<const char*, const char*>, 5> kLabels = {{
        {"Accent",  "主色"},
        {"Success", "成功"},
        {"Warning", "警告"},
        {"Danger",  "危险"},
        {"Info",    "信息"},
    }};
    return kLabels;
}

}  // namespace

struct ButtonStylesSection::Impl {
    // 行标题指针
    std::array<jtui::Text*, 6> row_titles{};

    // Row 1 Pill: 5 button (一 tone 一个)
    std::array<jtui::Button*, 5> pill_buttons{};

    // Row 2 Tonal: 5 button
    std::array<jtui::Button*, 5> tonal_buttons{};

    // Row 3 Gradient: 5 button
    std::array<jtui::Button*, 5> gradient_buttons{};

    // Row 4 Black Gold: 3 button (左到右 medium + medium beam + large beam)
    std::array<jtui::Button*, 3> black_gold_buttons{};

    // Row 5 Icon-only: 4 Circle + 4 Square. 用 CodiconIcon 作为 button 内容
    // 当前 Button widget 不能直接持有 CodiconIcon, 用 leading_icon 字符串 + codicon
    // codepoint UTF-8 编码绕过 (Button 自己用 draw_text 显示)。
    // 但是 Button 默认 font 不是 codicon, 所以这里改用独立 Panel 嵌套 CodiconIcon
    // + Button + 透明背景 hack 太重, 改用最简单方案: 用现成 ButtonVariant::Filled
    // 矩形/圆形, 上层覆盖一个 CodiconIcon widget 显示 glyph。
    std::array<jtui::Button*, 8> icon_button_bases{};
    std::array<jtui::CodiconIcon*, 8> icon_button_glyphs{};

    // Row 6 Group: 3 connected button (左 / 中 / 右)
    std::array<jtui::Button*, 3> group_buttons{};

    // 缓存 area for layout
    jtui::RectF area{};
};

ButtonStylesSection::ButtonStylesSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);

    auto add_row_title = [this](std::size_t idx) {
        auto title = std::make_unique<jtui::Text>();
        title->set_preset(jtui::TextStylePreset::Label);
        title->set_role(jtui::TextRole::Secondary);
        impl_->row_titles[idx] = title.get();
        root_->append_child(std::move(title));
    };
    for (std::size_t i = 0; i < 6; ++i) {
        add_row_title(i);
    }

    // Row 1: Pill 胶囊 (Filled + Pill shape)
    for (std::size_t i = 0; i < kAllTones.size(); ++i) {
        auto btn = std::make_unique<jtui::Button>();
        btn->set_variant(jtui::ButtonVariant::Filled);
        btn->set_shape(jtui::ButtonShape::Pill);
        btn->set_tone(kAllTones[i]);
        impl_->pill_buttons[i] = btn.get();
        root_->append_child(std::move(btn));
    }

    // Row 2: Tonal 柔和
    for (std::size_t i = 0; i < kAllTones.size(); ++i) {
        auto btn = std::make_unique<jtui::Button>();
        btn->set_variant(jtui::ButtonVariant::Tonal);
        btn->set_tone(kAllTones[i]);
        impl_->tonal_buttons[i] = btn.get();
        root_->append_child(std::move(btn));
    }

    // Row 3: Gradient 渐变 (同色明暗)
    for (std::size_t i = 0; i < kAllTones.size(); ++i) {
        auto btn = std::make_unique<jtui::Button>();
        btn->set_variant(jtui::ButtonVariant::Gradient);
        btn->set_tone(kAllTones[i]);
        impl_->gradient_buttons[i] = btn.get();
        root_->append_child(std::move(btn));
    }

    // Row 4: 黑金 + 流光 (3 个, 第 2/3 开 border beam)
    for (std::size_t i = 0; i < impl_->black_gold_buttons.size(); ++i) {
        auto btn = std::make_unique<jtui::Button>();
        btn->set_variant(jtui::ButtonVariant::GradientBlackGold);
        btn->set_size(i == 2 ? jtui::ButtonSize::Large : jtui::ButtonSize::Medium);
        if (i >= 1) {
            btn->set_border_beam(true);  // 第 2/3 开流光
        }
        impl_->black_gold_buttons[i] = btn.get();
        root_->append_child(std::move(btn));
    }

    // Row 5: Icon-only. 8 个 button: 4 Circle + 4 Square
    // 用 Button widget 做底色 (Ghost/Tonal/Filled), 上层覆盖 CodiconIcon 显示图标
    // Circle 用 Tonal, Square 用 Outlined, 视觉对比更丰富
    static const std::array<const char*, 8> kIconNames = {
        // Circle 4: search / settings-gear / heart / bell
        "search", "settings-gear", "heart", "bell",
        // Square 4: play / pause / debug-start / refresh
        "play", "debug-pause", "debug-start", "refresh",
    };
    for (std::size_t i = 0; i < 8; ++i) {
        auto btn = std::make_unique<jtui::Button>();
        if (i < 4) {
            btn->set_variant(jtui::ButtonVariant::Tonal);
            btn->set_shape(jtui::ButtonShape::Circle);
        } else {
            btn->set_variant(jtui::ButtonVariant::Outlined);
            btn->set_shape(jtui::ButtonShape::Square);
        }
        btn->set_tone(jtui::theme::Tone::Accent);
        btn->set_text("");  // 不要文字, 让位给 codicon
        impl_->icon_button_bases[i] = btn.get();
        root_->append_child(std::move(btn));

        auto glyph = std::make_unique<jtui::CodiconIcon>(std::string{kIconNames[i]});
        glyph->set_size_px(18.0F);
        glyph->set_color(jtui::theme::colors().accent);
        impl_->icon_button_glyphs[i] = glyph.get();
        root_->append_child(std::move(glyph));
    }

    // Row 6: Group 分段控制 (3 个连续按钮: 第 1 个 Filled 表示 selected, 其余 Outlined)
    for (std::size_t i = 0; i < impl_->group_buttons.size(); ++i) {
        auto btn = std::make_unique<jtui::Button>();
        btn->set_variant(i == 0 ? jtui::ButtonVariant::Filled
                                : jtui::ButtonVariant::Outlined);
        btn->set_tone(jtui::theme::Tone::Accent);
        impl_->group_buttons[i] = btn.get();
        root_->append_child(std::move(btn));
    }

    // 主题切换时 icon-only 行的 CodiconIcon 颜色跟着 accent 走
    Impl* impl_raw = impl_.get();
    jtui::theme::Theme::on_changed().connect([impl_raw](jtui::theme::ThemeMode) {
        const auto& c = jtui::theme::colors();
        for (jtui::CodiconIcon* g : impl_raw->icon_button_glyphs) {
            g->set_color(c.accent);
        }
    });
}

ButtonStylesSection::~ButtonStylesSection() = default;

std::unique_ptr<jtui::Panel> ButtonStylesSection::take_root() {
    return std::move(root_);
}

void ButtonStylesSection::apply_layout(const SectionLayout& area) {
    impl_->area = jtui::RectF{area.x, area.y, area.width, area.height};

    constexpr float row_h = 56.0F;
    constexpr float row_gap = 18.0F;
    constexpr float title_w = 120.0F;
    constexpr float title_x_off = 0.0F;
    constexpr float content_x_off = title_w + 16.0F;
    constexpr float btn_gap = 12.0F;

    // 各 variant 行通用尺寸: medium button h=36, w 按 label 长度估
    constexpr float btn_h = 36.0F;
    constexpr float btn_w_pill_tonal = 96.0F;
    constexpr float btn_w_gradient = 110.0F;

    auto place_title = [&](std::size_t row_idx) {
        const float y = area.y + 6.0F + static_cast<float>(row_idx) * (row_h + row_gap);
        impl_->row_titles[row_idx]->set_frame(
            jtui::RectF{area.x + title_x_off, y + (row_h - 20.0F) * 0.5F,
                        title_w, 20.0F});
        return y;
    };

    // Row 1: Pill 胶囊
    {
        const float y = place_title(0);
        float x = area.x + content_x_off;
        for (jtui::Button* btn : impl_->pill_buttons) {
            btn->set_frame(jtui::RectF{x, y + (row_h - btn_h) * 0.5F,
                                       btn_w_pill_tonal, btn_h});
            x += btn_w_pill_tonal + btn_gap;
        }
    }

    // Row 2: Tonal
    {
        const float y = place_title(1);
        float x = area.x + content_x_off;
        for (jtui::Button* btn : impl_->tonal_buttons) {
            btn->set_frame(jtui::RectF{x, y + (row_h - btn_h) * 0.5F,
                                       btn_w_pill_tonal, btn_h});
            x += btn_w_pill_tonal + btn_gap;
        }
    }

    // Row 3: Gradient
    {
        const float y = place_title(2);
        float x = area.x + content_x_off;
        for (jtui::Button* btn : impl_->gradient_buttons) {
            btn->set_frame(jtui::RectF{x, y + (row_h - btn_h) * 0.5F,
                                       btn_w_gradient, btn_h});
            x += btn_w_gradient + btn_gap;
        }
    }

    // Row 4: 黑金 (medium + medium beam + large beam)
    {
        const float y = place_title(3);
        float x = area.x + content_x_off;
        constexpr float bg_w_md = 130.0F;
        constexpr float bg_w_lg = 160.0F;
        constexpr float bg_h_md = 38.0F;
        constexpr float bg_h_lg = 46.0F;
        for (std::size_t i = 0; i < impl_->black_gold_buttons.size(); ++i) {
            const float w = (i == 2) ? bg_w_lg : bg_w_md;
            const float h = (i == 2) ? bg_h_lg : bg_h_md;
            impl_->black_gold_buttons[i]->set_frame(
                jtui::RectF{x, y + (row_h - h) * 0.5F, w, h});
            x += w + btn_gap;
        }
    }

    // Row 5: Icon-only
    {
        const float y = place_title(4);
        float x = area.x + content_x_off;
        constexpr float icon_btn_side = 40.0F;
        constexpr float glyph_size = 18.0F;
        for (std::size_t i = 0; i < 8; ++i) {
            const jtui::RectF btn_rect{x, y + (row_h - icon_btn_side) * 0.5F,
                                       icon_btn_side, icon_btn_side};
            impl_->icon_button_bases[i]->set_frame(btn_rect);
            // CodiconIcon 居中覆盖
            impl_->icon_button_glyphs[i]->set_frame(jtui::RectF{
                btn_rect.x + (icon_btn_side - glyph_size) * 0.5F,
                btn_rect.y + (icon_btn_side - glyph_size) * 0.5F,
                glyph_size, glyph_size});
            x += icon_btn_side + 10.0F;
            // 4 个 Circle 与 4 个 Square 之间额外 16px 间距分组
            if (i == 3) x += 16.0F;
        }
    }

    // Row 6: Group 分段 -- 三段独立按钮 + 正常 6px 间距, 不再边框紧贴 (用户反馈太挤).
    // 真实生产里"分段控制器"应该是专用 SegmentedControl widget (无 gap + 中间分隔线 +
    // selected 状态高亮); 这里只作为多按钮组合用法的演示。
    {
        const float y = place_title(5);
        const float x_start = area.x + content_x_off;
        constexpr float seg_w = 100.0F;
        constexpr float seg_h = 36.0F;
        constexpr float seg_gap = 6.0F;
        for (std::size_t i = 0; i < impl_->group_buttons.size(); ++i) {
            impl_->group_buttons[i]->set_frame(jtui::RectF{
                x_start + static_cast<float>(i) * (seg_w + seg_gap),
                y + (row_h - seg_h) * 0.5F, seg_w, seg_h});
        }
    }
}

void ButtonStylesSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;

    // 行标题
    static const std::array<std::pair<const char*, const char*>, 6> kRowTitles = {{
        {"Pill",       "胶囊"},
        {"Tonal",      "柔和"},
        {"Gradient",   "同色渐变"},
        {"Black Gold", "黑金流光"},
        {"Icon-only",  "图标按钮"},
        {"Group",      "分段控制"},
    }};
    for (std::size_t i = 0; i < 6; ++i) {
        impl_->row_titles[i]->set_content(zh ? kRowTitles[i].second
                                              : kRowTitles[i].first);
    }

    // Row 1 / 2 / 3: 5 个 tone 标签复用
    const auto& tones = tone_labels();
    for (std::size_t i = 0; i < 5; ++i) {
        const std::string lbl = zh ? tones[i].second : tones[i].first;
        impl_->pill_buttons[i]->set_text(lbl);
        impl_->tonal_buttons[i]->set_text(lbl);
        impl_->gradient_buttons[i]->set_text(lbl);
    }

    // Row 4: 黑金
    impl_->black_gold_buttons[0]->set_text(zh ? "黑金" : "Black Gold");
    impl_->black_gold_buttons[1]->set_text(zh ? "流光" : "Beam");
    impl_->black_gold_buttons[2]->set_text(zh ? "立即升级" : "Upgrade Now");

    // Row 5: Icon-only -- 不设 text (空字符串)
    for (jtui::Button* btn : impl_->icon_button_bases) {
        btn->set_text("");
    }

    // Row 6: Group 分段
    static const std::array<std::pair<const char*, const char*>, 3> kSegLabels = {{
        {"List",   "列表"},
        {"Grid",   "网格"},
        {"Detail", "详情"},
    }};
    for (std::size_t i = 0; i < 3; ++i) {
        impl_->group_buttons[i]->set_text(zh ? kSegLabels[i].second
                                              : kSegLabels[i].first);
    }
}

}  // namespace jtui::gallery
