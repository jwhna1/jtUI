// jtui_pulse —— jtUI Pulse 子产品 hero example。
// 珊瑚红 accent #FB7185 + "See what's happening, right now." 调性。
//
// 视觉范式借鉴 桌面UI.pdf p6：左 1/2 文字 hero（5 星评分 + 标题 + 副文 +
// email 输入 + Start Watching 主按钮 + 隐私小字 + 3 check feat）+ 右 1/2 程序
// 化生成的 live chart 装饰图。
//
// 关键 demo（验证刚完成的 jtUI 框架升级）：
//   - 直接用 jtui::Button + set_colors 自定义品牌色，**不再写 BrandButton 副本**
//     （v1.20 task #32）—— 这是首个享受新能力的 example。
//   - LiveChartBg override clips_self() = true，业务零 push_clip
//     （v1.20 task #31）。
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
#include "live_chart_bg.hpp"

namespace {

constexpr float kWindowWidth  = 1280.0F;
constexpr float kWindowHeight = 800.0F;
constexpr float kTitleBarH    = 36.0F;
constexpr float kNavBarH      = 60.0F;
constexpr float kSidePad      = 80.0F;

namespace jp = jtui_pulse;
namespace br = jtui_pulse::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

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

    // 左：jtUI Pulse logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_pulse = std::make_unique<jtui::Text>("Pulse");
        logo_pulse->set_font_size(18.0F);
        logo_pulse->set_bold(true);
        logo_pulse->set_color(p.accent);
        logo_pulse->set_frame(jtui::RectF{kSidePad + 44.0F, top, 70.0F, kNavBarH});
        root.append_child(std::move(logo_pulse));
    }

    // 中：METRICS / ALERTS / DOCS
    {
        const std::vector<std::string> link_keys = {
            "nav.metrics", "nav.alerts", "nav.docs"};
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

    // 右：lang / theme + Start Watching CTA
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
        cta->set_frame(jtui::RectF{right_x - kBtnW, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));

        // theme 切换：圆形 + leading icon
        auto theme_btn = std::make_unique<jtui::Button>("");
        theme_btn->set_shape(jtui::ButtonShape::Circle);
        theme_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
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

        // lang 切换
        auto lang_btn = std::make_unique<jtui::Button>("");
        lang_btn->set_shape(jtui::ButtonShape::Circle);
        lang_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
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
    constexpr float kColW = 540.0F;  // 加宽避免 48px 标题被 D2D 截断

    // 5 星评分行
    {
        const float row_y = top + 56.0F;
        constexpr float kStarSize = 18.0F;
        constexpr float kStarGap  = 4.0F;
        for (int i = 0; i < 5; ++i) {
            auto star = std::make_unique<jtui::CodiconIcon>("star-full");
            star->set_color(p.star);
            star->set_size_px(kStarSize);
            star->set_frame(jtui::RectF{
                left_x + (kStarSize + kStarGap) * static_cast<float>(i),
                row_y + 2.0F, kStarSize, kStarSize});
            root.append_child(std::move(star));
        }
        const float text_x = left_x + (kStarSize + kStarGap) * 5.0F + 8.0F;
        auto txt = std::make_unique<jtui::Text>(tr("rating.text"));
        txt->set_font_size(13.0F);
        txt->set_color(p.text_secondary);
        txt->set_frame(jtui::RectF{text_x, row_y, kColW - (text_x - left_x), 22.0F});
        root.append_child(std::move(txt));
    }

    // 大字标题 line1
    {
        const float title_y = top + 100.0F;
        constexpr float kLineH = 60.0F;
        constexpr float kFont  = 42.0F;  // 48 → 42 让两行标题在 540 列宽内不被截

        auto line1 = std::make_unique<jtui::Text>(tr("hero.title.l1"));
        line1->set_font_size(kFont);
        line1->set_bold(true);
        line1->set_color(p.text_strong);
        line1->set_alignment(jtui::TextAlignment::Leading);
        line1->set_frame(jtui::RectF{left_x, title_y, kColW, kLineH});
        root.append_child(std::move(line1));

        // line2 双段高亮：right 和 now 都染珊瑚红
        auto line2 = std::make_unique<jtui::Text>();
        line2->set_font_size(kFont);
        line2->set_bold(true);
        line2->set_color(p.text_strong);
        line2->set_alignment(jtui::TextAlignment::Leading);
        line2->set_runs({
            jtui::TextRun{tr("hero.title.l2.prefix")},
            jtui::TextRun{tr("hero.title.l2.highlight1"), p.accent},
            jtui::TextRun{tr("hero.title.l2.middle")},
            jtui::TextRun{tr("hero.title.l2.highlight2"), p.accent},
            jtui::TextRun{tr("hero.title.l2.suffix")},
        });
        line2->set_frame(jtui::RectF{left_x, title_y + kLineH, kColW, kLineH});
        root.append_child(std::move(line2));
    }

    // 副文
    {
        const float sub_y = top + 240.0F;
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

    // email 输入框 + Start Watching 按钮（横排）
    {
        const float row_y = top + 320.0F;
        constexpr float kRowH = 48.0F;
        constexpr float kBtnW = 180.0F;
        constexpr float kGap  = 8.0F;
        const float input_w = kColW - kBtnW - kGap;

        auto input = std::make_unique<jtui::TextInput>();
        input->set_placeholder(tr("hero.email.placeholder"));
        input->set_frame(jtui::RectF{left_x, row_y, input_w, kRowH});
        root.append_child(std::move(input));

        // 主 CTA：Start Watching（大尺寸 pill）
        auto cta = std::make_unique<jtui::Button>(tr("hero.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_size(jtui::ButtonSize::Large);
        cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        cta->set_frame(jtui::RectF{
            left_x + input_w + kGap, row_y, kBtnW, kRowH});
        root.append_child(std::move(cta));
    }

    // 隐私小字
    {
        auto hint = std::make_unique<jtui::Text>(tr("hero.privacy"));
        hint->set_font_size(11.0F);
        hint->set_color(p.text_muted);
        hint->set_frame(jtui::RectF{left_x, top + 380.0F, kColW, 18.0F});
        root.append_child(std::move(hint));
    }

    // 3 个 check feature
    {
        const float feat_y = top + 416.0F;
        constexpr float kRowH = 22.0F;
        constexpr float kCheckSize = 14.0F;
        constexpr float kItemGap = 24.0F;
        constexpr float kCheckTextGap = 6.0F;
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

    // 底部署名（底栏左对齐）
    {
        auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
        footer->set_font_size(12.0F);
        footer->set_color(p.text_muted);
        footer->set_alignment(jtui::TextAlignment::Leading);
        footer->set_frame(jtui::RectF{left_x, kWindowHeight - 32.0F, kColW, 18.0F});
        root.append_child(std::move(footer));
    }
}

// ───────── 右半屏 Live Chart 装饰 ───────────────────────────────────

void build_chart_right(hui::Panel& root) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH + 40.0F;
    const float left_x = kWindowWidth * 0.5F + 20.0F;
    const float chart_w = kWindowWidth - left_x - kSidePad;
    const float chart_h = kWindowHeight - top - 80.0F;

    // 圆角卡片包裹 chart，给图加边框感
    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_border(p.border_strong, 1.0F);
    card->set_frame(jtui::RectF{left_x, top, chart_w, chart_h});
    root.append_child(std::move(card));

    // 顶部 chart label 行
    {
        auto label_dot = std::make_unique<jtui::Panel>();
        label_dot->set_role(jtui::PanelRole::Base);
        label_dot->set_background_color(p.success);
        label_dot->set_corner_radius(4.0F);
        label_dot->set_frame(jtui::RectF{left_x + 16.0F, top + 18.0F, 8.0F, 8.0F});
        root.append_child(std::move(label_dot));

        auto label = std::make_unique<jtui::Text>(tr("chart.label"));
        label->set_font_size(12.0F);
        label->set_bold(true);
        label->set_color(p.text_secondary);
        label->set_frame(jtui::RectF{left_x + 32.0F, top + 12.0F, 200.0F, 20.0F});
        root.append_child(std::move(label));

        auto endpoint = std::make_unique<jtui::Text>(tr("chart.endpoint"));
        endpoint->set_font_size(12.0F);
        endpoint->set_color(p.text_muted);
        endpoint->set_alignment(jtui::TextAlignment::Trailing);
        endpoint->set_frame(jtui::RectF{
            left_x + chart_w - 16.0F - 200.0F, top + 12.0F, 200.0F, 20.0F});
        root.append_child(std::move(endpoint));
    }

    // chart 本体（占 card 内部主体区域）—— 启用 v1.20 clips_self() 自动 clip
    {
        constexpr float kChartTop = 48.0F;
        constexpr float kChartBot = 24.0F;
        auto chart = std::make_unique<jp::LiveChartBg>();
        chart->set_palette(p.bg_card, p.chart_line, p.chart_fill, p.chart_grid,
                           jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark);
        chart->set_frame(jtui::RectF{
            left_x + 16.0F, top + kChartTop,
            chart_w - 32.0F, chart_h - kChartTop - kChartBot});
        root.append_child(std::move(chart));
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

    build_navbar(*root, rebuild);
    build_hero_left(*root);
    build_chart_right(*root);

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
    options.title     = "jtUI Pulse";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    jp::register_strings();
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
