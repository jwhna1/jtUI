// jtui_hero —— jtUI 自身品牌的 hero example。
// 力量派 + 暖橘 accent。"Build native. Ship now. No bullshit."
//
// 本轮新增（2026-05-14）：
//   - 双主题 dark / light（点击 nav 右侧 sun/moon 切换）
//   - 中英双语 EN / 中（点击 nav 右侧 globe 切换）
//   - 切换走"整树 rebuild"：root.set_content(...) 直接替换。
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

namespace jh = jtui_hero;
namespace br = jtui_hero::brand;
using jtui::i18n::tr;

// 切换回调：lang / theme 改完后从 main 注入，让按钮回调能调它。
using RebuildFn = std::function<void()>;

// About modal 开关 —— 全局 static 状态，跨 rebuild 自然保活。
// hero 本来没有 state struct（只用全局 theme/i18n），为最小侵入接入 About。
static bool s_about_open = false;

// ───────── 装饰层 ──────────────────────────────────────────────────

void build_grid_decoration(hui::Panel& root) {
    const auto& p = br::active();
    constexpr float kCellW = 64.0F;
    constexpr float kCellH = 64.0F;

    for (float x = 0.0F; x < kWindowWidth; x += kCellW) {
        auto vl = std::make_unique<jtui::Panel>();
        vl->set_role(jtui::PanelRole::Base);
        vl->set_background_color(p.grid_line);
        vl->set_corner_radius(0.0F);
        vl->set_frame(jtui::RectF{x, kTitleBarH + kNavBarH, 1.0F,
                                  kWindowHeight - kTitleBarH - kNavBarH});
        root.append_child(std::move(vl));
    }
    for (float y = kTitleBarH + kNavBarH; y < kWindowHeight; y += kCellH) {
        auto hl = std::make_unique<jtui::Panel>();
        hl->set_role(jtui::PanelRole::Base);
        hl->set_background_color(p.grid_line);
        hl->set_corner_radius(0.0F);
        hl->set_frame(jtui::RectF{0.0F, y, kWindowWidth, 1.0F});
        root.append_child(std::move(hl));
    }
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

    // 左：jtUI logo + 版本号（不翻译）
    {
        auto logo = std::make_unique<jtui::Text>("jtUI");
        logo->set_font_size(18.0F);
        logo->set_bold(true);
        logo->set_color(p.text_strong);
        logo->set_frame(jtui::RectF{kSidePad, top, 80.0F, kNavBarH});
        root.append_child(std::move(logo));

        auto ver = std::make_unique<jtui::Text>("v0.2");
        ver->set_font_size(12.0F);
        ver->set_color(p.text_muted);
        ver->set_frame(jtui::RectF{kSidePad + 50.0F, top, 50.0F, kNavBarH});
        root.append_child(std::move(ver));
    }

    // 中：DOCS / SAMPLES / WIDGETS（i18n）
    {
        const std::vector<std::string> link_keys = {
            "nav.docs", "nav.samples", "nav.widgets"};
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

    // 右：lang 切换 + theme 切换 + Get the SDK 主 CTA
    {
        constexpr float kBtnW       = 150.0F;
        constexpr float kBtnH       = 38.0F;
        constexpr float kIconBtnSz  = 36.0F;
        constexpr float kIconBtnGap = 8.0F;

        const float right_x = kWindowWidth - kSidePad;
        const float btn_y   = top + (kNavBarH - kBtnH) * 0.5F;
        const float icon_y  = top + (kNavBarH - kIconBtnSz) * 0.5F;

        // 主 CTA
        auto cta = std::make_unique<jtui::Button>(tr("nav.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        cta->set_font_size(13.0F, true);
        cta->set_frame(jtui::RectF{right_x - kBtnW, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));

        // theme 切换：codicon 字典没有 sun/moon，用 color-mode（半亮半暗的圆）
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

        // About 按钮：info 圆形，hover 显示 bg_card。X 在 lang button 左侧 + 一个 icon 间距
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

        // lang 切换：globe
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

// ───────── Hero 主体 ───────────────────────────────────────────────

void build_hero(hui::Panel& root) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH;
    const float center_x = kWindowWidth * 0.5F;

    // 1) pill
    {
        constexpr float kPillW = 320.0F;
        constexpr float kPillH = 32.0F;
        const float pill_x = center_x - kPillW * 0.5F;
        const float pill_y = top + 64.0F;

        auto pill = std::make_unique<jtui::Panel>();
        pill->set_role(jtui::PanelRole::Base);
        pill->set_background_color(p.bg_card);
        pill->set_corner_radius(kPillH * 0.5F);
        pill->set_border(p.border, 1.0F);
        pill->set_frame(jtui::RectF{pill_x, pill_y, kPillW, kPillH});
        root.append_child(std::move(pill));

        constexpr float kTagW = 100.0F;
        auto tag_bg = std::make_unique<jtui::Panel>();
        tag_bg->set_role(jtui::PanelRole::Base);
        tag_bg->set_background_color(p.accent_soft);
        tag_bg->set_corner_radius((kPillH - 6.0F) * 0.5F);
        tag_bg->set_frame(jtui::RectF{pill_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag_bg));

        auto tag_label = std::make_unique<jtui::Text>(tr("hero.pill.tag"));
        tag_label->set_font_size(11.0F);
        tag_label->set_bold(true);
        tag_label->set_color(p.accent);
        tag_label->set_alignment(jtui::TextAlignment::Center);
        tag_label->set_frame(jtui::RectF{pill_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag_label));

        auto pill_text = std::make_unique<jtui::Text>(tr("hero.pill.text"));
        pill_text->set_font_size(12.0F);
        pill_text->set_color(p.text_primary);
        pill_text->set_alignment(jtui::TextAlignment::Center);
        pill_text->set_frame(jtui::RectF{
            pill_x + kTagW, pill_y, kPillW - kTagW - 6.0F, kPillH});
        root.append_child(std::move(pill_text));
    }

    // 2) 大字标题（双语，中段染暖橘）
    {
        const float title_y = top + 124.0F;
        constexpr float kLineH = 70.0F;
        constexpr float kFont  = 56.0F;

        auto line1 = std::make_unique<jtui::Text>();
        line1->set_font_size(kFont);
        line1->set_bold(true);
        line1->set_color(p.text_strong);
        line1->set_alignment(jtui::TextAlignment::Center);
        line1->set_runs({
            jtui::TextRun{tr("hero.title.l1.prefix")},
            jtui::TextRun{tr("hero.title.l1.highlight"), p.accent},
            jtui::TextRun{tr("hero.title.l1.suffix")},
        });
        line1->set_frame(jtui::RectF{0.0F, title_y, kWindowWidth, kLineH});
        root.append_child(std::move(line1));

        auto line2 = std::make_unique<jtui::Text>();
        line2->set_font_size(kFont);
        line2->set_bold(true);
        line2->set_color(p.text_strong);
        line2->set_alignment(jtui::TextAlignment::Center);
        line2->set_runs({
            jtui::TextRun{tr("hero.title.l2.prefix")},
            jtui::TextRun{tr("hero.title.l2.highlight"), p.accent},
            jtui::TextRun{tr("hero.title.l2.suffix")},
        });
        line2->set_frame(jtui::RectF{0.0F, title_y + kLineH, kWindowWidth, kLineH});
        root.append_child(std::move(line2));
    }

    // 3) 副文（双行）
    {
        const float sub_y = top + 296.0F;
        constexpr float kSubH = 24.0F;

        auto sub1 = std::make_unique<jtui::Text>(tr("hero.sub.line1"));
        sub1->set_font_size(16.0F);
        sub1->set_color(p.text_secondary);
        sub1->set_alignment(jtui::TextAlignment::Center);
        sub1->set_frame(jtui::RectF{0.0F, sub_y, kWindowWidth, kSubH});
        root.append_child(std::move(sub1));

        auto sub2 = std::make_unique<jtui::Text>(tr("hero.sub.line2"));
        sub2->set_font_size(16.0F);
        sub2->set_color(p.text_secondary);
        sub2->set_alignment(jtui::TextAlignment::Center);
        sub2->set_frame(jtui::RectF{0.0F, sub_y + kSubH + 4.0F, kWindowWidth, kSubH});
        root.append_child(std::move(sub2));
    }

    // 4) CTA
    {
        const float btn_y = top + 372.0F;
        constexpr float kBtnH = 48.0F;
        constexpr float kPrimaryW   = 200.0F;
        constexpr float kSecondaryW = 180.0F;
        constexpr float kBtnGap = 16.0F;
        const float total_w = kPrimaryW + kBtnGap + kSecondaryW;
        const float start_x = center_x - total_w * 0.5F;

        auto primary = std::make_unique<jtui::Button>(tr("hero.cta.primary"));
        primary->set_shape(jtui::ButtonShape::Pill);
        primary->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        primary->set_font_size(14.0F, true);
        primary->set_frame(jtui::RectF{start_x, btn_y, kPrimaryW, kBtnH});
        root.append_child(std::move(primary));

        auto secondary = std::make_unique<jtui::Button>(tr("hero.cta.secondary"));
        secondary->set_shape(jtui::ButtonShape::Pill);
        secondary->set_colors(
            jtui::Color{0.0F, 0.0F, 0.0F, 0.0F},  // 透明底
            p.bg_card,
            p.bg_card_hover,
            p.text_primary);
        secondary->set_border(p.border_strong, 1.0F);
        secondary->set_font_size(14.0F, true);
        secondary->set_frame(jtui::RectF{
            start_x + kPrimaryW + kBtnGap, btn_y, kSecondaryW, kBtnH});
        root.append_child(std::move(secondary));
    }

    // 5) 底部署名
    {
        auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
        footer->set_font_size(12.0F);
        footer->set_color(p.text_muted);
        footer->set_alignment(jtui::TextAlignment::Center);
        footer->set_frame(jtui::RectF{0.0F, kWindowHeight - 32.0F, kWindowWidth, 18.0F});
        root.append_child(std::move(footer));
    }
}

// ───────── 右下角"代码浮卡" ──────────────────────────────────────

void build_code_card(hui::Panel& root) {
    const auto& p = br::active();
    constexpr float kCardW = 360.0F;
    constexpr float kCardH = 156.0F;
    const float card_x = kWindowWidth - kSidePad - kCardW;
    const float card_y = kWindowHeight - 200.0F;

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_border(p.border_strong, 1.0F);
    card->set_frame(jtui::RectF{card_x, card_y, kCardW, kCardH});
    root.append_child(std::move(card));

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
                card_x + 14.0F + static_cast<float>(i) * 18.0F,
                card_y + 14.0F, 12.0F, 12.0F});
            root.append_child(std::move(dot));
        }

        auto title = std::make_unique<jtui::Text>(tr("code.title"));
        title->set_font_size(11.0F);
        title->set_color(p.text_muted);
        title->set_alignment(jtui::TextAlignment::Center);
        title->set_frame(jtui::RectF{card_x, card_y + 12.0F, kCardW, 16.0F});
        root.append_child(std::move(title));
    }

    const float code_y = card_y + 50.0F;
    constexpr float kLineH = 20.0F;
    constexpr float kCodeFont = 13.0F;
    const float code_x = card_x + 18.0F;
    const float code_w = kCardW - 36.0F;

    {
        auto l1 = std::make_unique<jtui::Text>();
        l1->set_font_size(kCodeFont);
        l1->set_color(p.text_secondary);
        l1->set_alignment(jtui::TextAlignment::Leading);
        l1->set_runs({
            jtui::TextRun{"auto btn = std::make_unique<"},
            jtui::TextRun{"jtui::Button", p.accent},
            jtui::TextRun{">();"},
        });
        l1->set_frame(jtui::RectF{code_x, code_y, code_w, kLineH});
        root.append_child(std::move(l1));

        auto l2 = std::make_unique<jtui::Text>();
        l2->set_font_size(kCodeFont);
        l2->set_color(p.text_secondary);
        l2->set_alignment(jtui::TextAlignment::Leading);
        l2->set_runs({
            jtui::TextRun{"btn->set_text("},
            jtui::TextRun{"\"Ship now.\"", p.code_string},
            jtui::TextRun{");"},
        });
        l2->set_frame(jtui::RectF{code_x, code_y + kLineH, code_w, kLineH});
        root.append_child(std::move(l2));

        auto l3 = std::make_unique<jtui::Text>();
        l3->set_font_size(kCodeFont);
        l3->set_color(p.text_secondary);
        l3->set_alignment(jtui::TextAlignment::Leading);
        l3->set_runs({
            jtui::TextRun{"window.set_content("},
            jtui::TextRun{"std::move", p.accent},
            jtui::TextRun{"(btn));"},
        });
        l3->set_frame(jtui::RectF{code_x, code_y + kLineH * 2.0F, code_w, kLineH});
        root.append_child(std::move(l3));

        auto l4 = std::make_unique<jtui::Text>(tr("code.comment"));
        l4->set_font_size(kCodeFont);
        l4->set_color(p.code_comment);
        l4->set_alignment(jtui::TextAlignment::Leading);
        l4->set_frame(jtui::RectF{code_x, code_y + kLineH * 3.0F, code_w, kLineH});
        root.append_child(std::move(l4));
    }
}

// ───────── 整树构建 + 注入 rebuild 回调 ──────────────────────────

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

    build_grid_decoration(*root);
    build_navbar(*root, rebuild);
    build_hero(*root);
    build_code_card(*root);

    // About modal（在最末 append → z-order 最上）
    jtui_about::build_about_modal(
        *root, s_about_open,
        jtui_about::palette_to_about(br::active()),
        [rebuild]() { s_about_open = false; rebuild(); },
        kWindowWidth, kWindowHeight);

    return root;
}

// ───────── App 入口 ────────────────────────────────────────────────

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "jtUI";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    jh::register_strings();
    jtui_about::register_about_strings();

    // rebuild 回调持引用 window；切换 lang/theme 时重新 build_root + set_content。
    // 用 shared_ptr 让 lambda 能引用 rebuild 自身（自指闭包）。
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
