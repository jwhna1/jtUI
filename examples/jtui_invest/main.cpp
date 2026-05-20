// jtui_invest —— jtUI Invest 金融投资子品牌落地页（视觉范式借鉴 桌面UI.pdf p20）。
//
// 本 example 是 example 集里最复杂的一个，刻意压上三个框架新能力：
//   1. 毛玻璃 NavBar —— fixed 顶栏，set_backdrop_blur 把背后滚动内容真高斯
//      模糊透出（D2D 1.1 CLSID_D2D1GaussianBlur）。
//   2. count-up 数字动画 + 折线 draw-in + 柱状 grow-in —— 见 animated_widgets.hpp。
//   3. ScrollView 长页面 —— hero + dashboard 一整页可滚动。
//
// 关键路径埋了 "invest" / "anim" / "backdrop" tag 的 log，方便定位。
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
#include "hui/foundation/log.hpp"

#include "brand_tokens.hpp"
#include "i18n_catalog.hpp"
#include "animated_widgets.hpp"
#include "_shared/about_dialog.hpp"

namespace {

constexpr float kWinW       = 1280.0F;
constexpr float kWinH       = 800.0F;
constexpr float kTitleBarH  = 36.0F;
constexpr float kNavBarH    = 64.0F;
constexpr float kPad        = 80.0F;
constexpr float kPageH      = 1460.0F;  // ScrollView content 总高

namespace iv = jtui_invest;
namespace br = jtui_invest::brand;
using jtui::i18n::tr;
using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// 切主题 / i18n 走整树 rebuild，但 ScrollView 滚动位置 + 动画进度不该被重置。
// InvestState 跨 rebuild 持有这些；rebuild 前存滚动位置，build 后恢复。
struct InvestState {
    bool   first_build {true};
    float  scroll_y {0.0F};
    jtui::ScrollView* scroll {nullptr};
};

// build_root 开头按 state.first_build 设置：true 时动画 widget 正常播放，
// false（rebuild）时动画 widget 创建后立即 finish_now() 跳到终值不重播。
bool g_play_anim = true;

// ───────── 小 helper：涨跌 chip（圆角小标签）──────────────────────

void add_chip(hui::Panel& root, jtui::RectF frame, const std::string& text,
              jtui::Color bg, jtui::Color fg) {
    auto chip = std::make_unique<jtui::Panel>();
    chip->set_role(jtui::PanelRole::Base);
    chip->set_background_color(bg);
    chip->set_corner_radius(frame.height * 0.42F);
    chip->set_frame(frame);
    root.append_child(std::move(chip));

    auto t = std::make_unique<jtui::Text>(text);
    t->set_font_size(11.0F);
    t->set_bold(true);
    t->set_color(fg);
    t->set_alignment(jtui::TextAlignment::Center);
    t->set_frame(frame);
    root.append_child(std::move(t));
}

// 小圆点 logo 占位（公司图标）
void add_dot(hui::Panel& root, float cx, float cy, float r, jtui::Color c) {
    auto dot = std::make_unique<jtui::Panel>();
    dot->set_role(jtui::PanelRole::Base);
    dot->set_background_color(c);
    dot->set_corner_radius(r);
    dot->set_frame(jtui::RectF{cx - r, cy - r, r * 2.0F, r * 2.0F});
    root.append_child(std::move(dot));
}

// 普通文本
void add_text(hui::Panel& root, jtui::RectF frame, const std::string& text,
              float size, bool bold, jtui::Color color,
              jtui::TextAlignment align = jtui::TextAlignment::Leading) {
    auto t = std::make_unique<jtui::Text>(text);
    t->set_font_size(size);
    t->set_bold(bold);
    t->set_color(color);
    t->set_alignment(align);
    t->set_frame(frame);
    root.append_child(std::move(t));
}

// 子卡片底（深色圆角 + 1px 描边）
void add_card_bg(hui::Panel& root, jtui::RectF frame) {
    const auto& p = br::active();
    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_border(p.border, 1.0F);
    card->set_frame(frame);
    root.append_child(std::move(card));
}

// ───────── 毛玻璃 NavBar（fixed）─────────────────────────────────────

void build_navbar(hui::Panel& root, const RebuildFn& rebuild) {
    const auto& p = br::active();
    const float top = kTitleBarH;

    // 毛玻璃底：backdrop blur 把背后滚动内容模糊透出 + glass_tint 半透明叠色。
    // 关键：必须显式 set_background_color 全透明 —— 否则 Panel::resolve_background
    // 在没 override 时返回不透明 bg_base，会在 backdrop blur 之后再盖一层实色，
    // 把毛玻璃彻底糊死（这正是上一版"透明都没出来"的 bug）。
    auto glass = std::make_unique<jtui::Panel>();
    glass->set_role(jtui::PanelRole::Base);
    glass->set_corner_radius(0.0F);
    glass->set_background_color(jtui::Color{0.0F, 0.0F, 0.0F, 0.0F});
    glass->set_backdrop_blur(64.0F, p.glass_tint);
    glass->set_frame(jtui::RectF{0.0F, top, kWinW, kNavBarH});
    root.append_child(std::move(glass));
    HUI_LOG_D("invest", "navbar glass set: blur=18 frame=0," +
                        std::to_string(static_cast<int>(top)) + ",1280,64");

    // 底部 1px 分隔线
    auto sep = std::make_unique<jtui::Panel>();
    sep->set_role(jtui::PanelRole::Base);
    sep->set_background_color(p.border);
    sep->set_corner_radius(0.0F);
    sep->set_frame(jtui::RectF{0.0F, top + kNavBarH, kWinW, 1.0F});
    root.append_child(std::move(sep));

    // 左：圆形 logo 球 + "jtUI" + accent "Invest"（jtUI 主品牌 + 金融子品牌）
    add_dot(root, kPad + 16.0F, top + kNavBarH * 0.5F, 16.0F, p.accent);
    add_text(root, jtui::RectF{kPad + 42.0F, top, 50.0F, kNavBarH},
             "jtUI", 17.0F, true, p.text_strong);
    add_text(root, jtui::RectF{kPad + 86.0F, top, 100.0F, kNavBarH},
             "Invest", 17.0F, true, p.accent);

    // 右：globe(i18n) + color-mode(theme) 圆按钮 + START FOR FREE 白 pill + 汉堡
    {
        constexpr float kBtnW   = 180.0F;
        constexpr float kBtnH   = 42.0F;
        constexpr float kIconSz = 40.0F;
        constexpr float kGap    = 12.0F;
        const float btn_y  = top + (kNavBarH - kBtnH) * 0.5F;
        const float icon_y = top + (kNavBarH - kIconSz) * 0.5F;
        const float menu_sz = 40.0F;
        const float right_x = kWinW - kPad;

        // 汉堡菜单（最右，圆角方 + 3 横线）
        const float menu_x = right_x - menu_sz;
        auto menu = std::make_unique<jtui::Panel>();
        menu->set_role(jtui::PanelRole::Base);
        menu->set_background_color(p.bg_card);
        menu->set_corner_radius(br::radius_xs);
        menu->set_border(p.border, 1.0F);
        menu->set_frame(jtui::RectF{menu_x, top + (kNavBarH - menu_sz) * 0.5F,
                                    menu_sz, menu_sz});
        root.append_child(std::move(menu));
        for (int i = 0; i < 3; ++i) {
            auto bar = std::make_unique<jtui::Panel>();
            bar->set_role(jtui::PanelRole::Base);
            bar->set_background_color(p.text_secondary);
            bar->set_corner_radius(1.0F);
            bar->set_frame(jtui::RectF{
                menu_x + 11.0F,
                top + (kNavBarH - menu_sz) * 0.5F + 14.0F +
                    static_cast<float>(i) * 6.0F,
                18.0F, 2.0F});
            root.append_child(std::move(bar));
        }

        // START FOR FREE 白 pill（纯 CTA，不再绑主题切换）
        const float cta_x = menu_x - kGap - kBtnW;
        auto cta = std::make_unique<jtui::Button>(tr("nav.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(jtui::Color::from_hex("#FFFFFF"),
                        jtui::Color::from_hex("#ECECEC"),
                        jtui::Color::from_hex("#DADADA"),
                        jtui::Color::from_hex("#0A0B0D"));
        cta->set_font_size(12.0F, true);
        cta->set_frame(jtui::RectF{cta_x, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));

        // color-mode（主题切换）圆按钮
        const float theme_x = cta_x - kGap - kIconSz;
        auto theme_btn = std::make_unique<jtui::Button>("");
        theme_btn->set_shape(jtui::ButtonShape::Circle);
        theme_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field,
                              p.text_primary);
        theme_btn->set_border(p.border_strong, 1.0F);
        theme_btn->set_leading_codicon("color-mode");
        theme_btn->set_frame(jtui::RectF{theme_x, icon_y, kIconSz, kIconSz});
        theme_btn->on_clicked().connect([rebuild]() {
            const auto cur = jtui::theme::Theme::mode();
            jtui::theme::Theme::set(cur == jtui::theme::ThemeMode::Dark
                ? jtui::theme::ThemeMode::Light
                : jtui::theme::ThemeMode::Dark);
            HUI_LOG_D("invest", "theme toggled");
            rebuild();
        });
        root.append_child(std::move(theme_btn));

        // About 按钮：info 圆形，hover bg_card，位于 lang button 左侧
        const float lang_x  = theme_x - kGap + 4.0F - kIconSz;
        const float about_x = lang_x  - kGap + 4.0F - kIconSz;
        auto about_btn = std::make_unique<jtui::Button>("");
        about_btn->set_shape(jtui::ButtonShape::Circle);
        about_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.accent);
        about_btn->set_border(p.border_strong, 1.0F);
        about_btn->set_leading_codicon("info");
        about_btn->set_frame(jtui::RectF{about_x, icon_y, kIconSz, kIconSz});
        about_btn->on_clicked().connect([rebuild]() {
            s_about_open = true;
            rebuild();
        });
        root.append_child(std::move(about_btn));

        // globe（i18n 切换）圆按钮
        auto lang_btn = std::make_unique<jtui::Button>("");
        lang_btn->set_shape(jtui::ButtonShape::Circle);
        lang_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field,
                             p.text_primary);
        lang_btn->set_border(p.border_strong, 1.0F);
        lang_btn->set_leading_codicon("globe");
        lang_btn->set_frame(jtui::RectF{lang_x, icon_y, kIconSz, kIconSz});
        lang_btn->on_clicked().connect([rebuild]() {
            const auto cur = jtui::i18n::current_locale();
            jtui::i18n::set_locale(cur == jtui::i18n::Locale::En
                ? jtui::i18n::Locale::Zh
                : jtui::i18n::Locale::En);
            HUI_LOG_D("invest", "locale toggled");
            rebuild();
        });
        root.append_child(std::move(lang_btn));
    }
}

// ───────── Hero ─────────────────────────────────────────────────────

void build_hero(hui::Panel& root) {
    const auto& p = br::active();
    const float hero_top = kTitleBarH + kNavBarH + 48.0F;
    const float col_w = kWinW - 2.0F * kPad;

    // 大标题两行，第一行 "financial data" 段酸橙绿 + 下划线
    {
        constexpr float kFont = 50.0F;
        auto line1 = std::make_unique<jtui::Text>();
        line1->set_font_size(kFont);
        line1->set_bold(true);
        line1->set_color(p.text_strong);
        line1->set_alignment(jtui::TextAlignment::Leading);
        line1->set_runs({
            jtui::TextRun{tr("hero.l1.pre")},
            jtui::TextRun{tr("hero.l1.mark"), p.accent},
            jtui::TextRun{tr("hero.l1.post")},
        });
        line1->set_frame(jtui::RectF{kPad, hero_top, col_w, 62.0F});
        root.append_child(std::move(line1));

        add_text(root, jtui::RectF{kPad, hero_top + 62.0F, col_w, 62.0F},
                 tr("hero.l2"), kFont, true, p.text_strong);

        // "financial data" 段下方下划线（酸橙绿）。Text::paint 多段排版用
        // char × 0.55 × font 估宽，这里下划线沿用同一估算，跟段位对齐。
        const float pre_w = static_cast<float>(
            std::string(tr("hero.l1.pre")).size()) * kFont * 0.55F;
        const float mark_w = static_cast<float>(
            std::string(tr("hero.l1.mark")).size()) * kFont * 0.55F;
        auto underline = std::make_unique<jtui::Panel>();
        underline->set_role(jtui::PanelRole::Base);
        underline->set_background_color(p.accent);
        underline->set_corner_radius(2.0F);
        underline->set_frame(jtui::RectF{kPad + pre_w, hero_top + 56.0F,
                                         mark_w, 3.0F});
        root.append_child(std::move(underline));
    }

    // 副文两行
    add_text(root, jtui::RectF{kPad, hero_top + 150.0F, 1080.0F, 24.0F},
             tr("hero.sub.l1"), 16.0F, false, p.text_secondary);
    add_text(root, jtui::RectF{kPad, hero_top + 176.0F, 1080.0F, 24.0F},
             tr("hero.sub.l2"), 16.0F, false, p.text_secondary);

    // CTA pill（酸橙绿）
    {
        auto cta = std::make_unique<jtui::Button>(tr("hero.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        cta->set_font_size(15.0F, true);
        cta->set_frame(jtui::RectF{kPad, hero_top + 224.0F, 236.0F, 58.0F});
        root.append_child(std::move(cta));
    }

    // 3 个 ✓ feature
    {
        const float feat_y = hero_top + 318.0F;
        const std::vector<std::string> feats = {
            tr("hero.feat.1"), tr("hero.feat.2"), tr("hero.feat.3")};
        float x = kPad;
        for (const auto& s : feats) {
            auto chk = std::make_unique<jtui::CodiconIcon>("check");
            chk->set_color(p.accent);
            chk->set_size_px(15.0F);
            chk->set_frame(jtui::RectF{x, feat_y + 3.0F, 15.0F, 15.0F});
            root.append_child(std::move(chk));

            const float tw =
                static_cast<float>(s.size()) * 7.6F + 10.0F;
            add_text(root, jtui::RectF{x + 24.0F, feat_y, tw, 22.0F},
                     s, 13.0F, false, p.text_primary);
            x += 24.0F + tw + 28.0F;
        }
    }
}

// ───────── Dashboard：PORTFOLIO 卡片 ────────────────────────────────

void build_portfolio_card(hui::Panel& root, jtui::RectF f) {
    const auto& p = br::active();
    add_card_bg(root, f);

    add_text(root, jtui::RectF{f.x + 22.0F, f.y + 20.0F, 200.0F, 18.0F},
             tr("portfolio.label"), 12.0F, true, p.text_muted);
    // 右上三点菜单
    add_text(root, jtui::RectF{f.x + f.width - 36.0F, f.y + 16.0F, 20.0F, 20.0F},
             "...", 16.0F, true, p.text_muted);

    // 大金额 count-up
    auto amount = std::make_unique<iv::CountUpText>();
    amount->configure(489451.48, 2, "$", 38.0F, true, p.text_strong);
    amount->set_duration(1.4F);
    amount->set_frame(jtui::RectF{f.x + 22.0F, f.y + 44.0F, f.width - 44.0F, 50.0F});
    if (!g_play_anim) amount->finish_now();
    root.append_child(std::move(amount));

    // 分隔线
    auto div = std::make_unique<jtui::Panel>();
    div->set_role(jtui::PanelRole::Base);
    div->set_background_color(p.divider);
    div->set_corner_radius(0.0F);
    div->set_frame(jtui::RectF{f.x + 22.0F, f.y + 110.0F, f.width - 44.0F, 1.0F});
    root.append_child(std::move(div));

    // INVESTED / RETURN 分栏
    add_text(root, jtui::RectF{f.x + 22.0F, f.y + 124.0F, 160.0F, 16.0F},
             tr("portfolio.invested"), 10.0F, true, p.text_muted);
    add_text(root, jtui::RectF{f.x + 22.0F, f.y + 144.0F, 160.0F, 22.0F},
             "$350,789.14", 15.0F, true, p.text_primary);

    add_text(root, jtui::RectF{f.x + 190.0F, f.y + 124.0F, 160.0F, 16.0F},
             tr("portfolio.return"), 10.0F, true, p.text_muted);
    add_text(root, jtui::RectF{f.x + 190.0F, f.y + 144.0F, 160.0F, 22.0F},
             "+$138,662.34", 15.0F, true, p.up);

    add_chip(root, jtui::RectF{f.x + 22.0F, f.y + 174.0F, 92.0F, 22.0F},
             "+28.33%", p.up_soft, p.up);
}

// ───────── Dashboard：QUARTERLY GAINS 卡片（柱状图）─────────────────

void build_gains_card(hui::Panel& root, jtui::RectF f) {
    const auto& p = br::active();
    add_card_bg(root, f);

    add_text(root, jtui::RectF{f.x + 22.0F, f.y + 20.0F, 220.0F, 18.0F},
             tr("gains.label"), 12.0F, true, p.text_muted);
    // 右上设置齿轮
    auto gear = std::make_unique<jtui::CodiconIcon>("settings-gear");
    gear->set_color(p.text_muted);
    gear->set_size_px(15.0F);
    gear->set_frame(jtui::RectF{f.x + f.width - 36.0F, f.y + 19.0F, 15.0F, 15.0F});
    root.append_child(std::move(gear));

    auto chart = std::make_unique<iv::BarChart>();
    chart->set_bars({
        {"Q1", "+15.28%",  15.28F, true},
        {"Q2", "-10.89%", -10.89F, false},
        {"Q3", "+41.06%",  41.06F, true},
        {"Q4", "+28.33%",  28.33F, true},
    });
    chart->set_palette(p.up, p.up_soft, p.down, p.down_soft,
                       p.text_secondary, p.border_strong);
    chart->set_duration(1.0F);
    chart->set_frame(jtui::RectF{f.x + 20.0F, f.y + 48.0F,
                                 f.width - 40.0F, f.height - 68.0F});
    if (!g_play_anim) chart->finish_now();
    root.append_child(std::move(chart));
}

// ───────── Dashboard：WATCHED COMPANIES 卡片（表格）─────────────────

void build_watched_card(hui::Panel& root, jtui::RectF f) {
    const auto& p = br::active();
    add_card_bg(root, f);

    add_text(root, jtui::RectF{f.x + 22.0F, f.y + 20.0F, 240.0F, 18.0F},
             tr("watched.label"), 12.0F, true, p.text_muted);
    auto gear = std::make_unique<jtui::CodiconIcon>("settings-gear");
    gear->set_color(p.text_muted);
    gear->set_size_px(15.0F);
    gear->set_frame(jtui::RectF{f.x + f.width - 36.0F, f.y + 19.0F, 15.0F, 15.0F});
    root.append_child(std::move(gear));

    // 表头
    const float col1 = f.x + 22.0F;
    const float col2 = f.x + 170.0F;
    const float col3 = f.x + 270.0F;
    const float head_y = f.y + 52.0F;
    add_text(root, jtui::RectF{col1, head_y, 140.0F, 16.0F},
             tr("watched.col.company"), 11.0F, false, p.text_muted);
    add_text(root, jtui::RectF{col2, head_y, 100.0F, 16.0F},
             tr("watched.col.price"), 11.0F, false, p.text_muted);
    add_text(root, jtui::RectF{col3, head_y, 100.0F, 16.0F},
             tr("watched.col.val"), 11.0F, false, p.text_muted);

    // 2 行数据
    struct Row {
        const char* name; jtui::Color dot; const char* price;
        bool price_up; const char* val;
    };
    const Row rows[2] = {
        {"Airbnb", jtui::Color::from_hex("#FF5A5F"), "$140.55", false, "$90B"},
        {"Shopify", jtui::Color::from_hex("#7AB55C"), "$72.50", true,  "$93B"},
    };
    for (int i = 0; i < 2; ++i) {
        const float row_y = f.y + 84.0F + static_cast<float>(i) * 44.0F;
        add_dot(root, col1 + 9.0F, row_y + 11.0F, 9.0F, rows[i].dot);
        add_text(root, jtui::RectF{col1 + 26.0F, row_y, 130.0F, 22.0F},
                 rows[i].name, 13.0F, true, p.text_primary);
        add_text(root, jtui::RectF{col2, row_y, 96.0F, 22.0F},
                 rows[i].price, 12.0F, false,
                 rows[i].price_up ? p.up : p.down);
        add_text(root, jtui::RectF{col3, row_y, 96.0F, 22.0F},
                 rows[i].val, 12.0F, false, p.text_secondary);
    }
}

// ───────── Dashboard：Spotify 折线图大卡片 ──────────────────────────

void build_spotify_card(hui::Panel& root, jtui::RectF f) {
    const auto& p = br::active();
    add_card_bg(root, f);

    // 顶部：Spotify logo + 名称 + 下拉箭头
    add_dot(root, f.x + 38.0F, f.y + 38.0F, 18.0F,
            jtui::Color::from_hex("#1DB954"));
    add_text(root, jtui::RectF{f.x + 66.0F, f.y + 24.0F, 160.0F, 28.0F},
             "Spotify", 19.0F, true, p.text_strong);
    add_text(root, jtui::RectF{f.x + 156.0F, f.y + 24.0F, 24.0F, 28.0F},
             "v", 13.0F, false, p.text_muted);

    // 右上：当前值 count-up + 涨幅 chip
    auto val = std::make_unique<iv::CountUpText>();
    val->configure(16478.69, 2, "$", 22.0F, true, p.text_strong);
    val->set_duration(1.4F);
    val->set_alignment(jtui::TextAlignment::Trailing);
    val->set_frame(jtui::RectF{f.x + f.width - 280.0F, f.y + 26.0F,
                               180.0F, 28.0F});
    if (!g_play_anim) val->finish_now();
    root.append_child(std::move(val));
    add_chip(root, jtui::RectF{f.x + f.width - 90.0F, f.y + 28.0F, 74.0F, 22.0F},
             "+11.75%", p.up_soft, p.up);

    // 折线图本体
    const float chart_x = f.x + 30.0F;
    const float chart_y = f.y + 80.0F;
    const float chart_w = f.width - 130.0F;   // 右侧留 Y 轴刻度位
    const float chart_h = f.height - 160.0F;  // 底部留时间 tab 位

    auto chart = std::make_unique<iv::AnimatedLineChart>();
    chart->set_data({
        0.58F, 0.52F, 0.60F, 0.48F, 0.42F, 0.38F, 0.45F, 0.35F,
        0.30F, 0.33F, 0.28F, 0.40F, 0.38F, 0.30F, 0.25F, 0.28F,
        0.32F, 0.30F, 0.35F, 0.33F, 0.30F, 0.28F, 0.32F, 0.38F,
        0.42F, 0.40F, 0.48F, 0.52F, 0.58F, 0.55F, 0.62F, 0.68F,
        0.65F, 0.72F, 0.78F, 0.75F, 0.82F, 0.88F, 0.85F, 0.95F});
    chart->set_palette(p.chart_line, p.chart_glow, p.chart_grid);
    chart->set_duration(1.8F);
    chart->set_frame(jtui::RectF{chart_x, chart_y, chart_w, chart_h});
    if (!g_play_anim) chart->finish_now();
    root.append_child(std::move(chart));

    // 右侧 Y 轴刻度
    const char* y_labels[5] = {"$20,000", "$15,000", "$10,000", "$5,000", "$0"};
    for (int i = 0; i < 5; ++i) {
        const float ly = chart_y + chart_h * static_cast<float>(i) / 4.0F - 8.0F;
        add_text(root, jtui::RectF{chart_x + chart_w + 12.0F, ly, 80.0F, 16.0F},
                 y_labels[i], 10.0F, false, p.text_muted);
    }

    // 底部时间范围 tab
    {
        const std::vector<std::string> tabs = {
            "1D", "1W", "1M", "3M", "1Y", "MAX"};
        const float tab_y = f.y + f.height - 44.0F;
        float tx = f.x + 30.0F;
        for (std::size_t i = 0; i < tabs.size(); ++i) {
            const bool active = (i == 3);  // 3M 高亮
            const float tw = 46.0F;
            if (active) {
                auto bg = std::make_unique<jtui::Panel>();
                bg->set_role(jtui::PanelRole::Base);
                bg->set_background_color(p.accent_soft);
                bg->set_corner_radius(br::radius_xs);
                bg->set_frame(jtui::RectF{tx, tab_y, tw, 28.0F});
                root.append_child(std::move(bg));
            }
            add_text(root, jtui::RectF{tx, tab_y + 4.0F, tw, 20.0F},
                     tabs[i], 12.0F, active, active ? p.accent : p.text_muted,
                     jtui::TextAlignment::Center);
            tx += tw + 6.0F;
        }

        // 右下 Chart Analysis 按钮
        auto analysis = std::make_unique<jtui::Button>(tr("chart.analysis"));
        analysis->set_shape(jtui::ButtonShape::Pill);
        analysis->set_colors(p.bg_field, p.bg_card_hover, p.bg_field,
                             p.text_primary);
        analysis->set_border(p.border_strong, 1.0F);
        analysis->set_leading_codicon("graph-line");
        analysis->set_font_size(12.0F, false);
        analysis->set_frame(jtui::RectF{f.x + f.width - 168.0F, tab_y - 2.0F,
                                        152.0F, 32.0F});
        root.append_child(std::move(analysis));
    }

    // tooltip 浮窗（折线中段某点的固定 tooltip）
    {
        const float tip_w = 168.0F;
        const float tip_h = 78.0F;
        const float tip_x = chart_x + chart_w * 0.52F;
        const float tip_y = chart_y + chart_h * 0.42F;
        auto tip = std::make_unique<jtui::Panel>();
        tip->set_role(jtui::PanelRole::Base);
        tip->set_background_color(p.bg_card_hover);
        tip->set_corner_radius(br::radius_sm);
        tip->set_border(p.border_strong, 1.0F);
        tip->set_shadow(jtui::theme::elevation().level_3);
        tip->set_frame(jtui::RectF{tip_x, tip_y, tip_w, tip_h});
        root.append_child(std::move(tip));

        add_dot(root, tip_x + 18.0F, tip_y + 18.0F, 7.0F,
                jtui::Color::from_hex("#1DB954"));
        add_text(root, jtui::RectF{tip_x + 32.0F, tip_y + 9.0F, 70.0F, 18.0F},
                 "Spotify", 12.0F, true, p.text_primary);
        add_text(root, jtui::RectF{tip_x + tip_w - 84.0F, tip_y + 10.0F,
                                   72.0F, 16.0F},
                 tr("chart.tooltip.date"), 9.0F, false, p.text_muted,
                 jtui::TextAlignment::Trailing);
        add_text(root, jtui::RectF{tip_x + 14.0F, tip_y + 34.0F, 60.0F, 16.0F},
                 tr("chart.tooltip.value"), 9.0F, false, p.text_muted);
        add_text(root, jtui::RectF{tip_x + tip_w - 96.0F, tip_y + 34.0F,
                                   84.0F, 16.0F},
                 "$5,128.32", 11.0F, true, p.text_primary,
                 jtui::TextAlignment::Trailing);
        add_text(root, jtui::RectF{tip_x + 14.0F, tip_y + 54.0F, 60.0F, 16.0F},
                 tr("chart.tooltip.return"), 9.0F, false, p.text_muted);
        add_text(root, jtui::RectF{tip_x + tip_w - 96.0F, tip_y + 54.0F,
                                   84.0F, 16.0F},
                 "+$58.78", 11.0F, true, p.up, jtui::TextAlignment::Trailing);
    }
}

// ───────── Dashboard：QUARTERLY TOP GAINERS（3 公司卡片）────────────

void build_top_gainers(hui::Panel& root, jtui::RectF area) {
    const auto& p = br::active();

    add_text(root, jtui::RectF{area.x + 4.0F, area.y, 320.0F, 22.0F},
             tr("gainers.label"), 13.0F, true, p.text_primary);
    auto filter = std::make_unique<jtui::CodiconIcon>("filter");
    filter->set_color(p.text_muted);
    filter->set_size_px(15.0F);
    filter->set_frame(jtui::RectF{area.x + area.width - 20.0F, area.y + 2.0F,
                                  15.0F, 15.0F});
    root.append_child(std::move(filter));

    struct Gainer {
        const char* name; jtui::Color dot; const char* price;
        const char* shares; const char* gain; const char* pct;
    };
    const Gainer gs[3] = {
        {"Airtable",  jtui::Color::from_hex("#FCB400"), "$42,875.69",
         "157K", "$4,890.78", "+15.97%"},
        {"Canva",     jtui::Color::from_hex("#00C4CC"), "$38,148.78",
         "100K", "$3,789.15", "+14.35%"},
        {"Microsoft", jtui::Color::from_hex("#00A4EF"), "$31,258.70",
         "50K",  "$3,490.05", "+13.87%"},
    };
    const float gap = 16.0F;
    const float card_w = (area.width - 2.0F * gap) / 3.0F;
    const float card_y = area.y + 34.0F;
    const float card_h = area.height - 34.0F;
    for (int i = 0; i < 3; ++i) {
        const float cx = area.x + static_cast<float>(i) * (card_w + gap);
        add_card_bg(root, jtui::RectF{cx, card_y, card_w, card_h});

        add_dot(root, cx + 26.0F, card_y + 28.0F, 12.0F, gs[i].dot);
        add_text(root, jtui::RectF{cx + 46.0F, card_y + 16.0F, 130.0F, 24.0F},
                 gs[i].name, 14.0F, true, p.text_strong);
        add_text(root, jtui::RectF{cx + card_w - 110.0F, card_y + 16.0F,
                                   96.0F, 24.0F},
                 gs[i].price, 13.0F, true, p.text_primary,
                 jtui::TextAlignment::Trailing);

        add_text(root, jtui::RectF{cx + 16.0F, card_y + 52.0F, 120.0F, 18.0F},
                 std::string(gs[i].shares) + " " + tr("gainers.shares"),
                 11.0F, false, p.text_muted);
        add_text(root, jtui::RectF{cx + card_w - 170.0F, card_y + 52.0F,
                                   96.0F, 18.0F},
                 gs[i].gain, 11.0F, true, p.up, jtui::TextAlignment::Trailing);
        add_chip(root, jtui::RectF{cx + card_w - 66.0F, card_y + 51.0F,
                                   54.0F, 20.0F},
                 gs[i].pct, p.up_soft, p.up);
    }
}

// ───────── Dashboard 大面板 ─────────────────────────────────────────

void build_dashboard(hui::Panel& root) {
    const auto& p = br::active();
    const float panel_x = 40.0F;
    const float panel_y = kTitleBarH + kNavBarH + 48.0F + 430.0F;  // hero 之下
    const float panel_w = kWinW - 2.0F * panel_x;
    const float panel_h = 828.0F;

    // 大面板底
    auto panel = std::make_unique<jtui::Panel>();
    panel->set_role(jtui::PanelRole::Base);
    panel->set_background_color(p.bg_panel);
    panel->set_corner_radius(br::radius_lg);
    panel->set_border(p.border, 1.0F);
    panel->set_frame(jtui::RectF{panel_x, panel_y, panel_w, panel_h});
    root.append_child(std::move(panel));

    // 标题行
    add_text(root, jtui::RectF{panel_x + 28.0F, panel_y + 26.0F, 320.0F, 32.0F},
             tr("dash.title"), 25.0F, true, p.text_strong);
    add_text(root, jtui::RectF{panel_x + 28.0F, panel_y + 62.0F, 420.0F, 20.0F},
             tr("dash.sub"), 13.0F, false, p.text_secondary);

    // 右上：通知铃铛 + Add New
    {
        const float bell_x = panel_x + panel_w - 220.0F;
        auto bell_bg = std::make_unique<jtui::Panel>();
        bell_bg->set_role(jtui::PanelRole::Base);
        bell_bg->set_background_color(p.bg_card);
        bell_bg->set_corner_radius(br::radius_xs);
        bell_bg->set_border(p.border, 1.0F);
        bell_bg->set_frame(jtui::RectF{bell_x, panel_y + 28.0F, 40.0F, 40.0F});
        root.append_child(std::move(bell_bg));
        auto bell = std::make_unique<jtui::CodiconIcon>("bell");
        bell->set_color(p.text_secondary);
        bell->set_size_px(17.0F);
        bell->set_frame(jtui::RectF{bell_x + 11.0F, panel_y + 39.0F,
                                    17.0F, 17.0F});
        root.append_child(std::move(bell));
        // badge
        add_dot(root, bell_x + 32.0F, panel_y + 32.0F, 8.0F, p.accent);
        add_text(root, jtui::RectF{bell_x + 24.0F, panel_y + 25.0F, 16.0F, 14.0F},
                 "2", 9.0F, true, p.accent_fg, jtui::TextAlignment::Center);

        auto add_btn = std::make_unique<jtui::Button>(tr("dash.add"));
        add_btn->set_shape(jtui::ButtonShape::Pill);
        add_btn->set_colors(p.bg_card_hover, p.bg_field, p.bg_card,
                            p.text_primary);
        add_btn->set_border(p.border_strong, 1.0F);
        add_btn->set_trailing_codicon("add");
        add_btn->set_font_size(13.0F, true);
        add_btn->set_frame(jtui::RectF{bell_x + 52.0F, panel_y + 28.0F,
                                       128.0F, 40.0F});
        root.append_child(std::move(add_btn));
    }

    // 内容区
    const float content_top = panel_y + 104.0F;
    const float left_x = panel_x + 28.0F;
    const float left_w = 372.0F;
    const float right_x = left_x + left_w + 20.0F;
    const float right_w = panel_x + panel_w - 28.0F - right_x;

    // 左栏 3 卡片
    build_portfolio_card(root,
        jtui::RectF{left_x, content_top, left_w, 208.0F});
    build_gains_card(root,
        jtui::RectF{left_x, content_top + 224.0F, left_w, 236.0F});
    build_watched_card(root,
        jtui::RectF{left_x, content_top + 476.0F, left_w, 216.0F});

    // 右栏：Spotify chart + TOP GAINERS
    build_spotify_card(root,
        jtui::RectF{right_x, content_top, right_w, 496.0F});
    build_top_gainers(root,
        jtui::RectF{right_x, content_top + 512.0F, right_w, 180.0F});
}

// ───────── Footer ───────────────────────────────────────────────────

void build_footer(hui::Panel& root) {
    const auto& p = br::active();
    add_text(root, jtui::RectF{0.0F, kTitleBarH + kPageH - 38.0F, kWinW, 18.0F},
             tr("footer.author"), 12.0F, false, p.text_muted,
             jtui::TextAlignment::Center);
}

// ───────── 整树 + rebuild ───────────────────────────────────────────

std::unique_ptr<jtui::Panel> build_root(const RebuildFn& rebuild,
                                        InvestState& state) {
    g_play_anim = state.first_build;
    const auto& p = br::active();
    HUI_LOG_I("invest", "build_root start, first_build=" +
        std::string(state.first_build ? "1" : "0") + " theme=" +
        std::string(jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark
                    ? "dark" : "light"));

    auto root = std::make_unique<jtui::Panel>();
    root->set_role(jtui::PanelRole::Base);
    root->set_background_color(p.bg_window);
    root->set_corner_radius(0.0F);
    root->set_frame(jtui::RectF{0.0F, 0.0F, kWinW, kWinH});

    auto window_frame = std::make_unique<jtui::WindowFrame>();
    window_frame->set_frameless(true);
    window_frame->set_frame(jtui::RectF{0.0F, 0.0F, kWinW, kWinH});
    root->append_child(std::move(window_frame));

    // ScrollView 长页面内容
    auto content = std::make_unique<jtui::Panel>();
    content->set_role(jtui::PanelRole::Base);
    content->set_background_color(p.bg_window);
    content->set_corner_radius(0.0F);
    content->set_frame(jtui::RectF{0.0F, kTitleBarH, kWinW, kPageH});
    build_hero(*content);
    build_dashboard(*content);
    build_footer(*content);

    auto scroll = std::make_unique<jtui::ScrollView>();
    scroll->set_frame(jtui::RectF{0.0F, kTitleBarH, kWinW, kWinH - kTitleBarH});
    scroll->set_content(std::move(content));
    jtui::ScrollView* scroll_raw = scroll.get();
    root->append_child(std::move(scroll));
    // root 是普通 Panel，window 的 layout pass 只对 root 调一次 arrange 且
    // Panel::arrange 不递归子节点 → ScrollView::arrange 不会被自动触发 →
    // content_size_ 停在 0 → clamp_offset 把滚动 offset 钳死在 0，整页滚不动。
    // 这里手动 arrange 一次初始化 content_size_；之后滚轮 set_scroll_offset
    // 内部会自己 re-arrange。
    scroll_raw->arrange(scroll_raw->frame());
    // 切主题 / i18n rebuild 后恢复滚动位置，不回顶。
    if (!state.first_build && state.scroll_y > 0.0F) {
        scroll_raw->set_scroll_offset(jtui::PointF{0.0F, state.scroll_y});
        HUI_LOG_D("invest", "scroll restored to y=" +
                            std::to_string(static_cast<int>(state.scroll_y)));
    }
    state.scroll = scroll_raw;
    HUI_LOG_D("invest", "scrollview arranged, content height=" +
                        std::to_string(static_cast<int>(kPageH)) +
                        " viewport=" +
                        std::to_string(static_cast<int>(kWinH - kTitleBarH)));

    // 毛玻璃 NavBar：append 在 ScrollView 之后 → 画在内容之上 → backdrop
    // blur 模糊的是 ScrollView 当前滚动位置的内容。
    build_navbar(*root, rebuild);

    // TitleBar
    auto title_bar = std::make_unique<jtui::TitleBar>(tr("window.title"));
    title_bar->set_frame(jtui::RectF{0.0F, 0.0F, kWinW, kTitleBarH});
    root->append_child(std::move(title_bar));

    // About modal —— 在 ScrollView/navbar/titlebar 全部 append 之后再加，
    // z-order 在所有内容之上。
    jtui_about::build_about_modal(
        *root, s_about_open,
        jtui_about::palette_to_about(br::active()),
        [rebuild]() { s_about_open = false; rebuild(); },
        kWinW, kWinH);

    HUI_LOG_I("invest", "build_root done");
    return root;
}

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "jtUI Invest";
    options.frameless = true;
    options.size      = {kWinW, kWinH};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    iv::register_strings();
    jtui_about::register_about_strings();
    HUI_LOG_I("invest", "jtui_invest starting");

    auto state = std::make_shared<InvestState>();
    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, state, rebuild_holder]() {
        // rebuild 前存当前滚动位置，build 后在 build_root 内恢复。
        if (state->scroll != nullptr) {
            state->scroll_y = state->scroll->scroll_offset().y;
        }
        state->first_build = false;
        window.set_content(build_root(*rebuild_holder, *state));
    };

    window.set_content(build_root(*rebuild_holder, *state));
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
