// jtui_live —— jtUI Live 子产品 hero example。
// 钢青 accent + "Edit Code. Reload in Seconds." 调性。
//
// 视觉范式借鉴 桌面UI.pdf p4：左半屏文字 hero + 右半屏大装饰图 + 右下角浮卡。
// 装饰图改为 mock 代码编辑器（VS Code 风），浮卡改为"已重载 47ms"状态卡，
// 整体表达 hot reload 工程师工具的工程感。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "jtui/jtui.hpp"

#include "brand_tokens.hpp"
#include "i18n_catalog.hpp"
#include "_shared/about_dialog.hpp"

namespace {

constexpr float kWindowWidth  = 1280.0F;
constexpr float kWindowHeight = 800.0F;
constexpr float kTitleBarH    = 36.0F;
constexpr float kNavBarH      = 60.0F;
constexpr float kSidePad      = 80.0F;

namespace jl = jtui_live;
namespace br = jtui_live::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// ───────── 装饰：钢青径向高光（很淡，仅夯底"工程冷感"）──────────

void build_glow_decoration(hui::Panel& root) {
    const auto& p = br::active();
    // 右上角一片极淡的钢青云团，dark 下若隐若现，light 下基本不可见
    const float gw = 520.0F;
    const float gh = 320.0F;
    auto glow = std::make_unique<jtui::Panel>();
    glow->set_role(jtui::PanelRole::Base);
    glow->set_background_color(jtui::Color{
        p.accent.r, p.accent.g, p.accent.b, 0.06F});
    glow->set_corner_radius(gh * 0.5F);
    glow->set_frame(jtui::RectF{
        kWindowWidth - gw - 40.0F, kTitleBarH + kNavBarH + 20.0F, gw, gh});
    root.append_child(std::move(glow));
}

// ───────── NavBar ──────────────────────────────────────────────────

void build_navbar(hui::Panel& root, const RebuildFn& rebuild) {
    const auto& p = br::active();
    const float top = kTitleBarH;

    {
        auto sep = std::make_unique<jtui::Panel>();
        sep->set_role(jtui::PanelRole::Base);
        sep->set_background_color(p.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(jtui::RectF{0.0F, top + kNavBarH, kWindowWidth, 1.0F});
        root.append_child(std::move(sep));
    }

    // 左：jtUI Live logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_live = std::make_unique<jtui::Text>("Live");
        logo_live->set_font_size(18.0F);
        logo_live->set_bold(true);
        logo_live->set_color(p.accent);
        logo_live->set_frame(jtui::RectF{kSidePad + 44.0F, top, 60.0F, kNavBarH});
        root.append_child(std::move(logo_live));
    }

    // 中：DOCS / SAMPLES / API
    {
        const std::vector<std::string> link_keys = {
            "nav.docs", "nav.samples", "nav.api"};
        constexpr float kLinkW = 100.0F;
        constexpr float kLinkGap = 8.0F;
        const float total_w = kLinkW * static_cast<float>(link_keys.size())
                            + kLinkGap * static_cast<float>(link_keys.size() - 1);
        const float start_x = (kWindowWidth - total_w) * 0.5F;

        for (std::size_t i = 0; i < link_keys.size(); ++i) {
            auto t = std::make_unique<jtui::Text>(tr(link_keys[i]));
            t->set_font_size(13.0F);
            t->set_bold(true);
            t->set_color(p.text_secondary);
            t->set_alignment(jtui::TextAlignment::Center);
            t->set_frame(jtui::RectF{
                start_x + (kLinkW + kLinkGap) * static_cast<float>(i),
                top, kLinkW, kNavBarH});
            root.append_child(std::move(t));
        }
    }

    // 右：lang / theme + Get Live Now CTA
    {
        constexpr float kBtnW       = 150.0F;
        constexpr float kBtnH       = 38.0F;
        constexpr float kIconBtnSz  = 36.0F;
        constexpr float kIconBtnGap = 8.0F;

        const float right_x = kWindowWidth - kSidePad;
        const float btn_y   = top + (kNavBarH - kBtnH) * 0.5F;
        const float icon_y  = top + (kNavBarH - kIconBtnSz) * 0.5F;

        auto cta = std::make_unique<jtui::Button>(tr("nav.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        cta->set_font_size(13.0F, true);
        cta->set_frame(jtui::RectF{right_x - kBtnW, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));

        auto theme_btn = std::make_unique<jtui::Button>("");
        theme_btn->set_shape(jtui::ButtonShape::Circle);
        theme_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
        theme_btn->set_border(p.border_strong, 1.0F);
        theme_btn->set_leading_codicon("color-mode");
        theme_btn->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap - kIconBtnSz, icon_y, kIconBtnSz, kIconBtnSz});
        theme_btn->on_clicked().connect([rebuild]() {
            const auto cur = jtui::theme::Theme::mode();
            jtui::theme::Theme::set(cur == jtui::theme::ThemeMode::Dark
                ? jtui::theme::ThemeMode::Light
                : jtui::theme::ThemeMode::Dark);
            rebuild();
        });
        root.append_child(std::move(theme_btn));

        // About 按钮：info 圆形，hover bg_card
        auto about_btn = std::make_unique<jtui::Button>("");
        about_btn->set_shape(jtui::ButtonShape::Circle);
        about_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.accent);
        about_btn->set_border(p.border_strong, 1.0F);
        about_btn->set_leading_codicon("info");
        about_btn->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 3.0F - kIconBtnSz * 3.0F,
            icon_y, kIconBtnSz, kIconBtnSz});
        about_btn->on_clicked().connect([rebuild]() {
            s_about_open = true;
            rebuild();
        });
        root.append_child(std::move(about_btn));

        auto lang_btn = std::make_unique<jtui::Button>("");
        lang_btn->set_shape(jtui::ButtonShape::Circle);
        lang_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
        lang_btn->set_border(p.border_strong, 1.0F);
        lang_btn->set_leading_codicon("globe");
        lang_btn->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 2.0F - kIconBtnSz * 2.0F,
            icon_y, kIconBtnSz, kIconBtnSz});
        lang_btn->on_clicked().connect([rebuild]() {
            const auto cur = jtui::i18n::current_locale();
            jtui::i18n::set_locale(cur == jtui::i18n::Locale::En
                ? jtui::i18n::Locale::Zh
                : jtui::i18n::Locale::En);
            rebuild();
        });
        root.append_child(std::move(lang_btn));
    }
}

// ───────── 左半屏 Hero 文字 ────────────────────────────────────────

void build_hero_left(hui::Panel& root) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH;
    const float left_x = kSidePad;
    constexpr float kColW = 520.0F;

    // pill
    {
        constexpr float kPillW = 240.0F;
        constexpr float kPillH = 32.0F;
        const float pill_y = top + 56.0F;

        auto pill = std::make_unique<jtui::Panel>();
        pill->set_role(jtui::PanelRole::Base);
        pill->set_background_color(p.bg_card);
        pill->set_corner_radius(kPillH * 0.5F);
        pill->set_border(p.border, 1.0F);
        pill->set_frame(jtui::RectF{left_x, pill_y, kPillW, kPillH});
        root.append_child(std::move(pill));

        constexpr float kTagW = 92.0F;
        auto tag_bg = std::make_unique<jtui::Panel>();
        tag_bg->set_role(jtui::PanelRole::Base);
        tag_bg->set_background_color(p.accent_soft);
        tag_bg->set_corner_radius((kPillH - 6.0F) * 0.5F);
        tag_bg->set_frame(jtui::RectF{left_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag_bg));

        auto tag = std::make_unique<jtui::Text>(tr("hero.pill.tag"));
        tag->set_font_size(11.0F);
        tag->set_bold(true);
        tag->set_color(p.accent);
        tag->set_alignment(jtui::TextAlignment::Center);
        tag->set_frame(jtui::RectF{left_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag));

        auto pill_text = std::make_unique<jtui::Text>(tr("hero.pill.text"));
        pill_text->set_font_size(12.0F);
        pill_text->set_color(p.text_primary);
        pill_text->set_alignment(jtui::TextAlignment::Center);
        pill_text->set_frame(jtui::RectF{
            left_x + kTagW, pill_y, kPillW - kTagW - 6.0F, kPillH});
        root.append_child(std::move(pill_text));
    }

    // 大字标题（左对齐 + 中段染钢青）
    {
        const float title_y = top + 116.0F;
        constexpr float kLineH = 64.0F;
        constexpr float kFont  = 48.0F;

        auto line1 = std::make_unique<jtui::Text>();
        line1->set_font_size(kFont);
        line1->set_bold(true);
        line1->set_color(p.text_strong);
        line1->set_alignment(jtui::TextAlignment::Leading);
        line1->set_runs({
            jtui::TextRun{tr("hero.title.l1.prefix")},
            jtui::TextRun{tr("hero.title.l1.highlight"), p.accent},
            jtui::TextRun{tr("hero.title.l1.suffix")},
        });
        line1->set_frame(jtui::RectF{left_x, title_y, kColW, kLineH});
        root.append_child(std::move(line1));

        auto line2 = std::make_unique<jtui::Text>();
        line2->set_font_size(kFont);
        line2->set_bold(true);
        line2->set_color(p.text_strong);
        line2->set_alignment(jtui::TextAlignment::Leading);
        line2->set_runs({
            jtui::TextRun{tr("hero.title.l2.prefix")},
            jtui::TextRun{tr("hero.title.l2.highlight"), p.accent},
            jtui::TextRun{tr("hero.title.l2.suffix")},
        });
        line2->set_frame(jtui::RectF{left_x, title_y + kLineH, kColW, kLineH});
        root.append_child(std::move(line2));
    }

    // 副文（左对齐两行）
    {
        const float sub_y = top + 264.0F;
        constexpr float kSubH = 24.0F;

        auto sub1 = std::make_unique<jtui::Text>(tr("hero.sub.line1"));
        sub1->set_font_size(15.0F);
        sub1->set_color(p.text_secondary);
        sub1->set_alignment(jtui::TextAlignment::Leading);
        sub1->set_frame(jtui::RectF{left_x, sub_y, kColW, kSubH});
        root.append_child(std::move(sub1));

        auto sub2 = std::make_unique<jtui::Text>(tr("hero.sub.line2"));
        sub2->set_font_size(15.0F);
        sub2->set_color(p.text_secondary);
        sub2->set_alignment(jtui::TextAlignment::Leading);
        sub2->set_frame(jtui::RectF{left_x, sub_y + kSubH + 4.0F, kColW, kSubH});
        root.append_child(std::move(sub2));
    }

    // CTA 主 + 次
    {
        const float btn_y = top + 340.0F;
        constexpr float kBtnH = 48.0F;
        constexpr float kPrimaryW   = 180.0F;
        constexpr float kSecondaryW = 180.0F;
        constexpr float kBtnGap = 16.0F;

        auto primary = std::make_unique<jtui::Button>(tr("hero.cta.primary"));
        primary->set_shape(jtui::ButtonShape::Pill);
        primary->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        primary->set_font_size(14.0F, true);
        primary->set_frame(jtui::RectF{left_x, btn_y, kPrimaryW, kBtnH});
        root.append_child(std::move(primary));

        auto secondary = std::make_unique<jtui::Button>(tr("hero.cta.secondary"));
        secondary->set_shape(jtui::ButtonShape::Pill);
        secondary->set_colors(
            jtui::Color{0.0F, 0.0F, 0.0F, 0.0F},
            p.bg_card,
            p.bg_card_hover,
            p.text_primary);
        secondary->set_border(p.border_strong, 1.0F);
        secondary->set_font_size(14.0F, true);
        secondary->set_frame(jtui::RectF{
            left_x + kPrimaryW + kBtnGap, btn_y, kSecondaryW, kBtnH});
        root.append_child(std::move(secondary));
    }

    // 三个 check feature（左对齐一行）
    {
        const float feat_y = top + 416.0F;
        constexpr float kRowH = 22.0F;
        constexpr float kCheckSize = 14.0F;
        constexpr float kItemGap = 28.0F;
        constexpr float kCheckTextGap = 8.0F;
        constexpr float kCharW = 7.5F;
        const std::vector<std::string> feats = {
            tr("hero.feat.1"), tr("hero.feat.2"), tr("hero.feat.3")};

        float x = left_x;
        for (const auto& s : feats) {
            const float tw = static_cast<float>(s.size()) * kCharW;

            auto chk = std::make_unique<jtui::CodiconIcon>("check");
            chk->set_color(p.accent);
            chk->set_size_px(kCheckSize);
            chk->set_frame(jtui::RectF{x, feat_y + (kRowH - kCheckSize) * 0.5F,
                                       kCheckSize, kCheckSize});
            root.append_child(std::move(chk));

            auto txt = std::make_unique<jtui::Text>(s);
            txt->set_font_size(13.0F);
            txt->set_color(p.text_secondary);
            txt->set_frame(jtui::RectF{
                x + kCheckSize + kCheckTextGap, feat_y, tw, kRowH});
            root.append_child(std::move(txt));

            x += kCheckSize + kCheckTextGap + tw + kItemGap;
        }
    }

    // 底部署名
    {
        auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
        footer->set_font_size(12.0F);
        footer->set_color(p.text_muted);
        footer->set_alignment(jtui::TextAlignment::Leading);
        footer->set_frame(jtui::RectF{left_x, kWindowHeight - 32.0F, kColW, 18.0F});
        root.append_child(std::move(footer));
    }
}

// ───────── 右半屏 Mock 代码编辑器 ──────────────────────────────────

void build_editor_mock(hui::Panel& root) {
    const auto& p = br::active();
    constexpr float kEditorW = 520.0F;
    constexpr float kEditorH = 360.0F;
    const float ex = kWindowWidth - kSidePad - kEditorW;
    const float ey = kTitleBarH + kNavBarH + 96.0F;

    // 编辑器外壳
    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_border(p.border_strong, 1.0F);
    card->set_frame(jtui::RectF{ex, ey, kEditorW, kEditorH});
    root.append_child(std::move(card));

    // 顶部栏：mac 三圆点 + 文件名
    {
        const std::vector<jtui::Color> dots = {
            jtui::Color::from_hex("#FF5F57"),
            jtui::Color::from_hex("#FEBC2E"),
            jtui::Color::from_hex("#28C840"),
        };
        for (std::size_t i = 0; i < dots.size(); ++i) {
            auto dot = std::make_unique<jtui::Panel>();
            dot->set_role(jtui::PanelRole::Base);
            dot->set_background_color(dots[i]);
            dot->set_corner_radius(6.0F);
            dot->set_frame(jtui::RectF{
                ex + 16.0F + static_cast<float>(i) * 20.0F,
                ey + 16.0F, 12.0F, 12.0F});
            root.append_child(std::move(dot));
        }

        auto title = std::make_unique<jtui::Text>(tr("editor.title"));
        title->set_font_size(12.0F);
        title->set_color(p.text_secondary);
        title->set_alignment(jtui::TextAlignment::Center);
        title->set_frame(jtui::RectF{ex, ey + 14.0F, kEditorW, 18.0F});
        root.append_child(std::move(title));

        // 顶部分割线
        auto sep = std::make_unique<jtui::Panel>();
        sep->set_role(jtui::PanelRole::Base);
        sep->set_background_color(p.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(jtui::RectF{ex + 1.0F, ey + 44.0F, kEditorW - 2.0F, 1.0F});
        root.append_child(std::move(sep));
    }

    // 代码区域
    constexpr float kCodeFont = 13.0F;
    constexpr float kLineH    = 24.0F;
    const float code_x = ex + 56.0F;       // 留 40px 给行号
    const float code_w = kEditorW - 56.0F - 16.0F;
    const float code_top = ey + 64.0F;
    const float gutter_x = ex + 14.0F;

    struct CodeLine {
        std::vector<jtui::TextRun> runs;
    };

    // 用 lambda 构造一行（捕获 p 引用）
    auto kw = [&](std::string s) { return jtui::TextRun{std::move(s), p.code_keyword}; };
    auto str = [&](std::string s) { return jtui::TextRun{std::move(s), p.code_string}; };
    auto txt = [&](std::string s) { return jtui::TextRun{std::move(s), p.text_secondary}; };
    auto cmt = [&](std::string s) { return jtui::TextRun{std::move(s), p.code_comment}; };

    const std::vector<CodeLine> lines = {
        {{txt("auto card = "), kw("std::make_unique"), txt("<"), kw("jtui::Panel"), txt(">();")}},
        {{txt("card->set_background_color("), kw("br::accent"), txt(");")}},
        {{txt("card->set_corner_radius("), str("12.0F"), txt(");")}},
        {{txt("card->set_frame({0, 0, "), str("260"), txt(", "), str("80"), txt("});")}},
        {{cmt(tr("editor.comment"))}},
    };

    for (std::size_t i = 0; i < lines.size(); ++i) {
        // 行号
        auto ln = std::make_unique<jtui::Text>(std::to_string(i + 1));
        ln->set_font_size(kCodeFont);
        ln->set_color(p.text_muted);
        ln->set_alignment(jtui::TextAlignment::Trailing);
        ln->set_frame(jtui::RectF{
            gutter_x, code_top + static_cast<float>(i) * kLineH, 28.0F, kLineH});
        root.append_child(std::move(ln));

        // 代码
        auto code = std::make_unique<jtui::Text>();
        code->set_font_size(kCodeFont);
        code->set_color(p.text_primary);
        code->set_alignment(jtui::TextAlignment::Leading);
        code->set_runs(lines[i].runs);
        code->set_frame(jtui::RectF{
            code_x, code_top + static_cast<float>(i) * kLineH, code_w, kLineH});
        root.append_child(std::move(code));
    }

    // 底部 cursor 闪烁（静态横条暗示焦点）
    {
        auto cursor = std::make_unique<jtui::Panel>();
        cursor->set_role(jtui::PanelRole::Base);
        cursor->set_background_color(p.accent);
        cursor->set_corner_radius(1.0F);
        cursor->set_frame(jtui::RectF{
            code_x + 0.0F,
            code_top + static_cast<float>(lines.size()) * kLineH + 4.0F,
            2.0F, 16.0F});
        root.append_child(std::move(cursor));
    }
}

// ───────── 右下角"已重载"浮卡 ────────────────────────────────────

void build_reload_card(hui::Panel& root) {
    const auto& p = br::active();
    constexpr float kCardW = 280.0F;
    constexpr float kCardH = 96.0F;
    const float cx = kWindowWidth - kSidePad - kCardW + 60.0F;  // 微微向右偏出编辑器
    const float cy = kWindowHeight - 220.0F;

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_border(p.border_strong, 1.0F);
    card->set_frame(jtui::RectF{cx, cy, kCardW, kCardH});
    root.append_child(std::move(card));

    // 左侧 success 圆点指示
    {
        constexpr float kDot = 8.0F;
        auto dot = std::make_unique<jtui::Panel>();
        dot->set_role(jtui::PanelRole::Base);
        dot->set_background_color(p.success);
        dot->set_corner_radius(kDot * 0.5F);
        dot->set_frame(jtui::RectF{cx + 16.0F, cy + 18.0F, kDot, kDot});
        root.append_child(std::move(dot));

        auto title = std::make_unique<jtui::Text>(tr("reload.title"));
        title->set_font_size(13.0F);
        title->set_bold(true);
        title->set_color(p.text_primary);
        title->set_frame(jtui::RectF{cx + 32.0F, cy + 12.0F, 100.0F, 20.0F});
        root.append_child(std::move(title));

        auto dur = std::make_unique<jtui::Text>(tr("reload.duration"));
        dur->set_font_size(13.0F);
        dur->set_bold(true);
        dur->set_color(p.success);
        dur->set_alignment(jtui::TextAlignment::Trailing);
        dur->set_frame(jtui::RectF{cx + kCardW - 80.0F, cy + 12.0F, 64.0F, 20.0F});
        root.append_child(std::move(dur));
    }

    // 文件名 + diff 链接
    {
        auto file = std::make_unique<jtui::Text>(tr("reload.file"));
        file->set_font_size(12.0F);
        file->set_color(p.text_muted);
        file->set_frame(jtui::RectF{cx + 16.0F, cy + 40.0F, kCardW - 32.0F, 18.0F});
        root.append_child(std::move(file));
    }

    // 底部进度条（mock 满）
    {
        auto bar_bg = std::make_unique<jtui::Panel>();
        bar_bg->set_role(jtui::PanelRole::Base);
        bar_bg->set_background_color(p.bg_field);
        bar_bg->set_corner_radius(2.0F);
        bar_bg->set_frame(jtui::RectF{cx + 16.0F, cy + 70.0F, kCardW - 100.0F, 4.0F});
        root.append_child(std::move(bar_bg));

        auto bar_fg = std::make_unique<jtui::Panel>();
        bar_fg->set_role(jtui::PanelRole::Base);
        bar_fg->set_background_color(p.accent);
        bar_fg->set_corner_radius(2.0F);
        bar_fg->set_frame(jtui::RectF{cx + 16.0F, cy + 70.0F, kCardW - 100.0F, 4.0F});
        root.append_child(std::move(bar_fg));

        auto diff = std::make_unique<jtui::Text>(tr("reload.diff"));
        diff->set_font_size(11.0F);
        diff->set_bold(true);
        diff->set_color(p.accent);
        diff->set_alignment(jtui::TextAlignment::Trailing);
        diff->set_frame(jtui::RectF{cx + kCardW - 80.0F, cy + 64.0F, 64.0F, 18.0F});
        root.append_child(std::move(diff));
    }
}

// ───────── 整树 + rebuild 自指 ─────────────────────────────────────

std::unique_ptr<jtui::Panel> build_root(const RebuildFn& rebuild) {
    auto root = std::make_unique<jtui::Panel>();
    const auto& p = br::active();

    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(p.bg_window);
    root->set_corner_radius(0.0F);
    root->set_frame(jtui::RectF{0.0F, 0.0F, kWindowWidth, kWindowHeight});

    auto window_frame = std::make_unique<jtui::WindowFrame>();
    window_frame->set_frameless(true);
    window_frame->set_frame(jtui::RectF{0.0F, 0.0F, kWindowWidth, kWindowHeight});
    root->append_child(std::move(window_frame));

    auto title_bar = std::make_unique<jtui::TitleBar>(tr("window.title"));
    title_bar->set_frame(jtui::RectF{0.0F, 0.0F, kWindowWidth, kTitleBarH});
    root->append_child(std::move(title_bar));

    build_glow_decoration(*root);
    build_navbar(*root, rebuild);
    build_hero_left(*root);
    build_editor_mock(*root);
    build_reload_card(*root);

    jtui_about::build_about_modal(
        *root, s_about_open,
        jtui_about::palette_to_about(br::active()),
        [rebuild]() { s_about_open = false; rebuild(); },
        kWindowWidth, kWindowHeight);

    return root;
}

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "jtUI Live";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    jl::register_strings();
    jtui_about::register_about_strings();

    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, rebuild_holder]() {
        window.set_content(build_root(*rebuild_holder));
    };

    window.set_content(build_root(*rebuild_holder));
    return app.run();
}

}  // namespace

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    return run_app();
}
#else
int main() {
    return run_app();
}
#endif
