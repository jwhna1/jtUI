#pragma once

// jtUI v1.4 框架级 About 卡片 —— 由 examples/_shared/about_dialog.hpp 升级而来。
//
// 设计：业务侧把自己 brand palette 的颜色映射到 AboutColors，调用
//   hui::install_about_card(root, open, colors, on_close, w, h)
// 即可。返回 Panel* container raw ptr，container.set_visible(open) 切换显隐。
// i18n 文案统一从 register_about_i18n() 一次性注入到主 catalog。
//
// 内容结构（modal 600x720 居中）：
//   header  : jtUI logo + by jtai team 角标
//   title   : "About jtUI" + 2 行副标题（反 Electron 力量派文案）
//   section : 9 BRAND EXAMPLES · ONE ENGINE（9 个 example 名 + 一行描述）
//   section : CRAFTED WITH（8 个技术栈 chip）
//   meta    : Started / Team / Lead / Version
//   vision  : "Build native. Ship fast. As simple as Vue."（accent 强调）
//   close   : 底部居中 pill 按钮
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hui/foundation/color.hpp"
#include "hui/foundation/i18n.hpp"
#include "hui/object/widget.hpp"
#include "hui/theme/elevation_tokens.hpp"
#include "hui/widgets/basic/panel.hpp"
#include "hui/widgets/basic/text.hpp"
#include "hui/widgets/common/button.hpp"

namespace hui {

struct AboutColors {
    Color scrim;             // 全屏蒙层色（半透黑/白）
    Color bg_panel;          // modal 卡片底
    Color border;            // 分割线
    Color border_strong;     // 卡片描边
    Color text_strong;       // 标题
    Color text_primary;      // value 文本
    Color text_secondary;    // 副标题
    Color text_muted;        // label / 注释
    Color accent;            // 主品牌色（每个 example 不同）
    Color accent_hover;
    Color accent_pressed;
    Color accent_soft;       // chip / tag bg
    Color accent_fg;         // accent 上文字色
};

// 通用：把任意 brand Palette（duck-typed，要求字段 bg_panel/border/border_strong/
// text_*/accent_*）映射成 AboutColors。每个 example 一行调用：
//     auto c = hui::palette_to_about(brand::active());
template <typename PaletteT>
[[nodiscard]] inline AboutColors palette_to_about(const PaletteT& p) {
    AboutColors c{};
    c.scrim          = Color{0.0F, 0.0F, 0.0F, 0.62F};
    c.bg_panel       = p.bg_panel;
    c.border         = p.border;
    c.border_strong  = p.border_strong;
    c.text_strong    = p.text_strong;
    c.text_primary   = p.text_primary;
    c.text_secondary = p.text_secondary;
    c.text_muted     = p.text_muted;
    c.accent         = p.accent;
    c.accent_hover   = p.accent_hover;
    c.accent_pressed = p.accent_pressed;
    c.accent_soft    = p.accent_soft;
    c.accent_fg      = p.accent_fg;
    return c;
}

// 一次性向 hui::i18n 主 catalog 注入 about.* + nav.about 全套条目。
// 多 example 重复调安全（register_entries 用 map[] 覆盖，幂等）。
inline void register_about_i18n() {
    i18n::register_entries({
        {"nav.about",               "ABOUT",                    "关于"},
        {"about.title",             "About jtUI",               "关于 jtUI"},
        {"about.subtitle.l1",
         "A C++ desktop UI framework with Vue-like simplicity.",
         "一款追求 Vue 般简单的 C++ 桌面 UI 框架。"},
        {"about.subtitle.l2",
         "Pure C++. Real frames. No Electron, no JS bridge.",
         "纯 C++，原生帧渲染。无 Electron，无 JS 桥。"},

        // 9 examples section
        {"about.examples.label",
         "9 BRAND EXAMPLES  ·  ONE ENGINE",
         "9 套品牌示例  ·  同一引擎"},
        {"about.ex.hero.desc",
         "Native hero — power statement",
         "原生 hero — 力量派品牌"},
        {"about.ex.cinema.desc",
         "Video carousel — cinematic UI",
         "视频轮播 — 原生影像 UI"},
        {"about.ex.studio.desc",
         "Media studio — VideoPlayer + AudioPlayer",
         "媒体工作台 — 视频 + 音频"},
        {"about.ex.invest.desc",
         "Financial landing — glass + animation",
         "金融落地 — 毛玻璃 + 动画"},
        {"about.ex.atlas.desc",
         "KPI sparklines — chart cards",
         "KPI 微图 — 图表卡片"},
        {"about.ex.live.desc",
         "Live editor — code mock-up",
         "直播编辑器 — 代码原型"},
        {"about.ex.pro.desc",
         "Pro studio — stream lines",
         "Pro 工作室 — 流线背景"},
        {"about.ex.pulse.desc",
         "Live chart background — pulse",
         "动态图表背景 — pulse"},
        {"about.ex.folders.desc",
         "File manager (jtUI Folders style)",
         "文件管理（jtUI Folders 风格）"},

        // stack chips section
        {"about.stack.label",       "CRAFTED WITH",             "技术栈"},

        // meta rows
        {"about.history.label",     "Started",                  "起始"},
        {"about.history.value",     "March 2025",               "2025 年 3 月"},
        {"about.team.label",        "Team",                     "团队"},
        {"about.team.value",        "jtai",                     "jtai 团队"},
        {"about.lead.label",        "Lead",                     "主导"},
        {"about.lead.value",        "曾能混 · jwhna1@gmail.com",
                                    "曾能混 · jwhna1@gmail.com"},
        {"about.version.label",     "Version",                  "版本"},
        {"about.version.value",     "v0.4 · Pre-release",       "v0.4 · 预览版"},

        // vision
        {"about.vision.value",
         "Build native. Ship fast. As simple as Vue.",
         "原生构建，极速出货，像 Vue 一样简单。"},

        {"about.close",             "Close",                    "关闭"},
    });
}

// 业务侧调用入口。所有内部 widget 装入一个 hui::Panel container（覆盖全窗
// 透明），container.set_visible(open) 控制整体显隐。返回 container raw ptr：
//   - rebuild 模式（brand example）：忽略返回值，靠下次 rebuild 重建
//   - 单次 install 模式（gallery）：拿到 raw ptr 后业务侧 set_visible 切换
//
// scrim / close button click → 先 container.set_visible(false) 再调用 user
// 提供的 on_close（rebuild example 在 on_close 里调 rebuild；gallery 传 noop）。
inline Panel* install_about_card(
    Panel& root,
    bool open,
    const AboutColors& c,
    std::function<void()> on_close,
    float window_w,
    float window_h)
{
    using i18n::tr;

    // ─── 0) Container：包住所有 modal widget，set_visible 整体控制 ─
    auto container_owned = std::make_unique<Panel>();
    container_owned->set_role(PanelRole::Base);
    container_owned->set_background_color(Color{0.0F, 0.0F, 0.0F, 0.0F});
    container_owned->set_corner_radius(0.0F);
    container_owned->set_frame(RectF{0.0F, 0.0F, window_w, window_h});
    container_owned->set_visible(open);
    Panel* container_raw = container_owned.get();

    auto inner_close = [container_raw, user_close = std::move(on_close)]() {
        container_raw->set_visible(false);
        if (user_close) user_close();
    };

    auto& panel = *container_raw;
    root.append_child(std::move(container_owned));

    // ─── 1) Scrim ─────────────────────────────────────────────
    {
        auto scrim = std::make_unique<Button>("");
        scrim->set_shape(ButtonShape::Default);
        scrim->set_corner_radius(0.0F);
        scrim->set_colors(c.scrim, c.scrim, c.scrim, Color{1.0F, 1.0F, 1.0F, 0.0F});
        scrim->set_frame(RectF{0.0F, 0.0F, window_w, window_h});
        scrim->on_clicked().connect(inner_close);
        panel.append_child(std::move(scrim));
    }

    // ─── 2) Modal Card 居中布局 ───────────────────────────────
    constexpr float kModalW = 600.0F;
    constexpr float kModalH = 720.0F;
    constexpr float kPad    = 32.0F;
    const float card_x = (window_w - kModalW) * 0.5F;
    const float card_y = (window_h - kModalH) * 0.5F;

    {
        auto card = std::make_unique<Panel>();
        card->set_role(PanelRole::Base);
        card->set_background_color(c.bg_panel);
        card->set_corner_radius(28.0F);
        card->set_border(c.border_strong, 1.0F);
        card->set_shadow(theme::elevation().level_4);
        card->set_frame(RectF{card_x, card_y, kModalW, kModalH});
        panel.append_child(std::move(card));
    }

    float y = card_y + kPad;

    // ─── 3) Header：jtUI logo + by jtai team 角标 ───────────────
    {
        auto jt = std::make_unique<Text>("jtUI");
        jt->set_font_size(28.0F);
        jt->set_bold(true);
        jt->set_color(c.accent);
        jt->set_frame(RectF{card_x + kPad, y, 80.0F, 36.0F});
        panel.append_child(std::move(jt));

        constexpr float kTagW = 100.0F, kTagH = 22.0F;
        auto tag_bg = std::make_unique<Panel>();
        tag_bg->set_role(PanelRole::Base);
        tag_bg->set_background_color(c.accent_soft);
        tag_bg->set_corner_radius(kTagH * 0.5F);
        tag_bg->set_frame(RectF{card_x + kPad + 80.0F, y + 8.0F, kTagW, kTagH});
        panel.append_child(std::move(tag_bg));

        auto tag = std::make_unique<Text>("by jtai team");
        tag->set_font_size(11.0F);
        tag->set_bold(true);
        tag->set_color(c.accent);
        tag->set_alignment(TextAlignment::Center);
        tag->set_frame(RectF{card_x + kPad + 80.0F, y + 8.0F, kTagW, kTagH});
        panel.append_child(std::move(tag));
    }
    y += 48.0F;

    // ─── 4) Title + Subtitle 2 lines ───────────────────────────
    {
        auto t = std::make_unique<Text>(tr("about.title"));
        t->set_font_size(22.0F);
        t->set_bold(true);
        t->set_color(c.text_strong);
        t->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 30.0F});
        panel.append_child(std::move(t));
    }
    y += 32.0F;
    {
        auto t1 = std::make_unique<Text>(tr("about.subtitle.l1"));
        t1->set_font_size(14.0F);
        t1->set_color(c.text_secondary);
        t1->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 20.0F});
        panel.append_child(std::move(t1));
    }
    y += 22.0F;
    {
        auto t2 = std::make_unique<Text>(tr("about.subtitle.l2"));
        t2->set_font_size(13.0F);
        t2->set_color(c.text_muted);
        t2->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 20.0F});
        panel.append_child(std::move(t2));
    }
    y += 26.0F;

    auto add_sep = [&](float& yy) {
        auto sep = std::make_unique<Panel>();
        sep->set_role(PanelRole::Base);
        sep->set_background_color(c.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(RectF{card_x + kPad, yy, kModalW - kPad * 2.0F, 1.0F});
        panel.append_child(std::move(sep));
        yy += 14.0F;
    };

    add_sep(y);

    // ─── 5) Examples section ──────────────────────────────────
    {
        auto label = std::make_unique<Text>(tr("about.examples.label"));
        label->set_font_size(11.0F);
        label->set_bold(true);
        label->set_color(c.text_muted);
        label->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 16.0F});
        panel.append_child(std::move(label));
    }
    y += 22.0F;

    struct Example { const char* name; const char* desc_key; };
    const Example examples[] = {
        {"jtui_hero",    "about.ex.hero.desc"},
        {"jtui_cinema",  "about.ex.cinema.desc"},
        {"jtui_studio",  "about.ex.studio.desc"},
        {"jtui_invest",  "about.ex.invest.desc"},
        {"jtui_atlas",   "about.ex.atlas.desc"},
        {"jtui_live",    "about.ex.live.desc"},
        {"jtui_pro",     "about.ex.pro.desc"},
        {"jtui_pulse",   "about.ex.pulse.desc"},
        {"folders_app",  "about.ex.folders.desc"},
    };
    constexpr float kExRowH = 21.0F;
    constexpr float kExNameW = 110.0F;
    for (const auto& ex : examples) {
        auto name = std::make_unique<Text>(ex.name);
        name->set_font_size(12.0F);
        name->set_bold(true);
        name->set_color(c.accent);
        name->set_frame(RectF{card_x + kPad, y, kExNameW, kExRowH});
        panel.append_child(std::move(name));

        auto desc = std::make_unique<Text>(tr(ex.desc_key));
        desc->set_font_size(12.0F);
        desc->set_color(c.text_secondary);
        desc->set_frame(RectF{
            card_x + kPad + kExNameW, y,
            kModalW - kPad * 2.0F - kExNameW, kExRowH});
        panel.append_child(std::move(desc));

        y += kExRowH;
    }
    y += 8.0F;

    add_sep(y);

    // ─── 6) Stack chips section ───────────────────────────────
    {
        auto label = std::make_unique<Text>(tr("about.stack.label"));
        label->set_font_size(11.0F);
        label->set_bold(true);
        label->set_color(c.text_muted);
        label->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 16.0F});
        panel.append_child(std::move(label));
    }
    y += 22.0F;

    const char* chips[] = {
        "Pure C++20", "Direct2D 1.1", "WASAPI",
        "Media Foundation", "Backdrop Blur", "Box Shadow",
        "Bezier / Polygon", "i18n + Dual Theme",
    };
    constexpr float kChipH    = 22.0F;
    constexpr float kChipGap  = 6.0F;
    constexpr float kChipFont = 11.0F;
    constexpr float kCharPx   = 6.5F;
    float chip_x = card_x + kPad;
    float chip_y = y;
    const float chip_x_max = card_x + kModalW - kPad;
    for (const char* chip : chips) {
        const float text_w = static_cast<float>(std::string{chip}.length()) * kCharPx;
        const float chip_w = text_w + 18.0F;
        if (chip_x + chip_w > chip_x_max) {
            chip_x = card_x + kPad;
            chip_y += kChipH + kChipGap;
        }
        auto bg = std::make_unique<Panel>();
        bg->set_role(PanelRole::Base);
        bg->set_background_color(c.accent_soft);
        bg->set_corner_radius(kChipH * 0.5F);
        bg->set_frame(RectF{chip_x, chip_y, chip_w, kChipH});
        panel.append_child(std::move(bg));

        auto t = std::make_unique<Text>(chip);
        t->set_font_size(kChipFont);
        t->set_bold(true);
        t->set_color(c.accent);
        t->set_alignment(TextAlignment::Center);
        t->set_frame(RectF{chip_x, chip_y, chip_w, kChipH});
        panel.append_child(std::move(t));

        chip_x += chip_w + kChipGap;
    }
    y = chip_y + kChipH + 12.0F;

    add_sep(y);

    // ─── 7) Meta rows: Started / Team / Lead / Version ────────
    struct MetaRow { const char* label_key; const char* value_key; };
    const MetaRow meta[] = {
        {"about.history.label", "about.history.value"},
        {"about.team.label",    "about.team.value"},
        {"about.lead.label",    "about.lead.value"},
        {"about.version.label", "about.version.value"},
    };
    constexpr float kMetaRowH = 22.0F;
    constexpr float kMetaLabelW = 90.0F;
    for (const auto& m : meta) {
        auto label = std::make_unique<Text>(tr(m.label_key));
        label->set_font_size(12.0F);
        label->set_bold(true);
        label->set_color(c.text_muted);
        label->set_frame(RectF{card_x + kPad, y, kMetaLabelW, kMetaRowH});
        panel.append_child(std::move(label));

        auto value = std::make_unique<Text>(tr(m.value_key));
        value->set_font_size(13.0F);
        value->set_color(c.text_primary);
        value->set_frame(RectF{
            card_x + kPad + kMetaLabelW, y,
            kModalW - kPad * 2.0F - kMetaLabelW, kMetaRowH});
        panel.append_child(std::move(value));

        y += kMetaRowH;
    }
    y += 8.0F;

    // ─── 8) Vision（accent 强调，居中） ───────────────────────
    {
        auto vision = std::make_unique<Text>(tr("about.vision.value"));
        vision->set_font_size(14.0F);
        vision->set_bold(true);
        vision->set_color(c.accent);
        vision->set_alignment(TextAlignment::Center);
        vision->set_frame(RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 24.0F});
        panel.append_child(std::move(vision));
    }

    // ─── 9) Close 按钮 ────────────────────────────────────────
    {
        constexpr float kBtnW = 140.0F;
        constexpr float kBtnH = 40.0F;
        auto close = std::make_unique<Button>(tr("about.close"));
        close->set_shape(ButtonShape::Pill);
        close->set_colors(c.accent, c.accent_hover, c.accent_pressed, c.accent_fg);
        close->set_font_size(13.0F, true);
        close->set_frame(RectF{
            card_x + (kModalW - kBtnW) * 0.5F,
            card_y + kModalH - kPad - kBtnH,
            kBtnW, kBtnH});
        close->on_clicked().connect(inner_close);
        panel.append_child(std::move(close));
    }

    return container_raw;
}

}  // namespace hui
