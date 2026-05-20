// jtui_atlas —— jtUI Atlas 子品牌（observability / BI 数据大屏）。
// 森林深绿 + 黄金边点缀。"Watch every signal." 调性。
//
// 视觉范式：page header（标题 + 副文）+ 4 个 KPI 卡片行 + 2 个 chart 卡片
// + 底部事件条。所有卡片走 elevation 阴影分层（KPI level_1 轻浮起，chart
// level_2 中等浮起，events strip level_0 贴底）。重度演练 v1.22 新加的真高斯
// 模糊阴影能力 + 框架 fill_polygon 自绘 sparkline。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
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

namespace ja = jtui_atlas;
namespace br = jtui_atlas::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// ───────── Sparkline 自定义 widget ──────────────────────────────────
// 用 fill_polygon 画面积图 + line 段画顶部数据线 + 末值 dot 高亮。仅 example
// 自用，没有上提到框架——atlas 的卖点之一就是"用 jtUI 现有几何 API 也能拼
// 出像样的可视化"。

class Sparkline : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Sparkline";
    }

    void set_data(std::vector<float> v) noexcept {
        data_ = std::move(v);
        mark_dirty(hui::DirtyFlags::Paint);
    }

    void set_palette(jtui::Color line, jtui::Color fill, jtui::Color grid) noexcept {
        line_ = line;
        fill_ = fill;
        grid_ = grid;
        mark_dirty(hui::DirtyFlags::Paint);
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F || data_.empty()) return;

        // 1) 横向网格线 3 条（淡）
        constexpr int kGridLines = 3;
        for (int i = 1; i <= kGridLines; ++i) {
            const float y = f.y + f.height * static_cast<float>(i) /
                                    static_cast<float>(kGridLines + 1);
            context.line(hui::PointF{f.x, y},
                         hui::PointF{f.x + f.width, y}, grid_, 1.0F);
        }

        // 2) 数据范围
        float vmin = data_[0], vmax = data_[0];
        for (const float v : data_) {
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
        }
        const float vrange = (vmax - vmin) > 0.001F ? (vmax - vmin) : 1.0F;

        // 3) 各 sample 在 frame 内的坐标
        const std::size_t n = data_.size();
        const float top_pad = 8.0F, bot_pad = 8.0F;
        const float chart_h = f.height - top_pad - bot_pad;
        std::vector<hui::PointF> line_pts;
        line_pts.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            const float t = (n <= 1) ? 0.0F
                                     : static_cast<float>(i) /
                                           static_cast<float>(n - 1);
            const float norm = (data_[i] - vmin) / vrange;
            line_pts.push_back(hui::PointF{
                f.x + t * f.width,
                f.y + top_pad + (1.0F - norm) * chart_h,
            });
        }

        // 4) 面积填充（fill_polygon: 数据线 → 右下 → 左下闭合）
        std::vector<hui::PointF> poly = line_pts;
        poly.push_back(hui::PointF{f.x + f.width, f.y + f.height - bot_pad});
        poly.push_back(hui::PointF{f.x,           f.y + f.height - bot_pad});
        context.fill_polygon(std::move(poly), fill_);

        // 5) 顶部数据线（segment-by-segment）
        for (std::size_t i = 0; i + 1 < line_pts.size(); ++i) {
            context.line(line_pts[i], line_pts[i + 1], line_, 2.0F);
        }

        // 6) 末值高亮 dot
        const hui::PointF last = line_pts.back();
        context.fill_ellipse(
            hui::RectF{last.x - 4.0F, last.y - 4.0F, 8.0F, 8.0F}, line_);
    }

private:
    std::vector<float> data_ {};
    jtui::Color line_ {};
    jtui::Color fill_ {};
    jtui::Color grid_ {};
};

// ───────── Mock 数据 ────────────────────────────────────────────────
// 48 个采样点 = 24 小时每 30 分钟一个。形状刻意区分两个 chart 节律：
// volume 偏"双峰白天上班"，latency 偏"夜间稳，白天抖"。

std::vector<float> volume_data() {
    return {0.32F, 0.41F, 0.55F, 0.48F, 0.62F, 0.71F, 0.58F, 0.69F,
            0.78F, 0.83F, 0.77F, 0.86F, 0.74F, 0.81F, 0.69F, 0.75F,
            0.82F, 0.88F, 0.91F, 0.85F, 0.79F, 0.73F, 0.67F, 0.71F,
            0.65F, 0.59F, 0.52F, 0.46F, 0.39F, 0.42F, 0.48F, 0.55F,
            0.61F, 0.58F, 0.66F, 0.72F, 0.78F, 0.84F, 0.81F, 0.87F,
            0.93F, 0.89F, 0.82F, 0.76F, 0.69F, 0.62F, 0.55F, 0.61F};
}

std::vector<float> latency_data() {
    return {0.45F, 0.48F, 0.51F, 0.47F, 0.52F, 0.58F, 0.61F, 0.55F,
            0.49F, 0.53F, 0.59F, 0.63F, 0.67F, 0.72F, 0.68F, 0.61F,
            0.56F, 0.59F, 0.65F, 0.71F, 0.78F, 0.83F, 0.79F, 0.74F,
            0.69F, 0.65F, 0.62F, 0.58F, 0.55F, 0.51F, 0.48F, 0.45F,
            0.42F, 0.48F, 0.53F, 0.57F, 0.61F, 0.66F, 0.69F, 0.64F,
            0.58F, 0.53F, 0.49F, 0.46F, 0.51F, 0.55F, 0.59F, 0.63F};
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

    // 左：jtUI Atlas logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_atlas = std::make_unique<jtui::Text>("Atlas");
        logo_atlas->set_font_size(18.0F);
        logo_atlas->set_bold(true);
        logo_atlas->set_color(p.accent);
        logo_atlas->set_frame(jtui::RectF{kSidePad + 44.0F, top, 80.0F, kNavBarH});
        root.append_child(std::move(logo_atlas));
    }

    // 中：OVERVIEW / ALERTS / STREAMS
    {
        const std::vector<std::string> link_keys = {
            "nav.overview", "nav.alerts", "nav.streams"};
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

    // 右：CTA + theme + lang
    {
        constexpr float kBtnW       = 130.0F;
        constexpr float kBtnH       = 38.0F;
        constexpr float kIconBtnSz  = 36.0F;
        constexpr float kIconBtnGap = 8.0F;

        const float right_x = kWindowWidth - kSidePad;
        const float btn_y   = top + (kNavBarH - kBtnH) * 0.5F;
        const float icon_y  = top + (kNavBarH - kIconBtnSz) * 0.5F;

        auto cta = std::make_unique<jtui::Button>(tr("nav.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
        cta->set_border(p.gold, 1.0F);  // 黄金边点缀（Trust badge 风格）
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

// ───────── Page Header（标题 + 副文）────────────────────────────────

void build_page_header(hui::Panel& root) {
    const auto& p = br::active();
    const float y = kTitleBarH + kNavBarH + 24.0F;

    auto title = std::make_unique<jtui::Text>(tr("hero.title"));
    title->set_font_size(28.0F);
    title->set_bold(true);
    title->set_color(p.text_strong);
    title->set_alignment(jtui::TextAlignment::Leading);
    // h=40 让 28F bold 字形 ascent/descent 完整显示（DWrite 行高 ~1.4×font_size）
    title->set_frame(jtui::RectF{kSidePad, y, kWindowWidth - 2.0F * kSidePad, 40.0F});
    root.append_child(std::move(title));

    auto sub = std::make_unique<jtui::Text>(tr("hero.sub"));
    sub->set_font_size(13.0F);
    sub->set_color(p.text_secondary);
    sub->set_frame(jtui::RectF{kSidePad, y + 44.0F,
                                kWindowWidth - 2.0F * kSidePad, 18.0F});
    root.append_child(std::move(sub));
}

// ───────── 4 个 KPI 卡片 ────────────────────────────────────────────

struct KpiSpec {
    const char* label_key;
    const char* value_key;
    const char* delta_key;
    bool        delta_positive;  // 影响 delta 颜色（正向 accent / 负向 muted）
};

void build_kpi(hui::Panel& root, const KpiSpec& spec,
               jtui::RectF card_frame) {
    const auto& p = br::active();

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_shadow(jtui::theme::elevation().level_1);
    card->set_frame(card_frame);
    root.append_child(std::move(card));

    // 顶部 label
    auto label = std::make_unique<jtui::Text>(tr(spec.label_key));
    label->set_font_size(11.0F);
    label->set_bold(true);
    label->set_color(p.text_muted);
    label->set_frame(jtui::RectF{
        card_frame.x + 18.0F, card_frame.y + 16.0F,
        card_frame.width - 36.0F, 16.0F});
    root.append_child(std::move(label));

    // 大数值
    auto value = std::make_unique<jtui::Text>(tr(spec.value_key));
    value->set_font_size(30.0F);
    value->set_bold(true);
    value->set_color(p.text_strong);
    value->set_frame(jtui::RectF{
        card_frame.x + 18.0F, card_frame.y + 36.0F,
        card_frame.width - 36.0F, 40.0F});
    root.append_child(std::move(value));

    // delta 描述
    auto delta = std::make_unique<jtui::Text>(tr(spec.delta_key));
    delta->set_font_size(12.0F);
    delta->set_color(spec.delta_positive ? p.accent : p.text_secondary);
    delta->set_frame(jtui::RectF{
        card_frame.x + 18.0F, card_frame.y + card_frame.height - 28.0F,
        card_frame.width - 36.0F, 16.0F});
    root.append_child(std::move(delta));
}

void build_kpi_row(hui::Panel& root) {
    const float y = kTitleBarH + kNavBarH + 24.0F + 56.0F + 24.0F;  // 192
    constexpr float kCardH = 110.0F;
    constexpr float kCardGap = 20.0F;
    const float total_w = kWindowWidth - 2.0F * kSidePad;
    const float card_w = (total_w - 3.0F * kCardGap) / 4.0F;

    const KpiSpec kpis[4] = {
        {"kpi.users.label",   "kpi.users.value",   "kpi.users.delta",   true},
        {"kpi.latency.label", "kpi.latency.value", "kpi.latency.delta", false},
        {"kpi.errors.label",  "kpi.errors.value",  "kpi.errors.delta",  false},
        {"kpi.uptime.label",  "kpi.uptime.value",  "kpi.uptime.delta",  true},
    };

    for (std::size_t i = 0; i < 4; ++i) {
        const float x = kSidePad + static_cast<float>(i) * (card_w + kCardGap);
        build_kpi(root, kpis[i], jtui::RectF{x, y, card_w, kCardH});
    }
}

// ───────── 2 个 chart 卡片 ──────────────────────────────────────────

void build_chart_card(hui::Panel& root, const char* title_key,
                      const char* range_key, std::vector<float> data,
                      jtui::RectF card_frame) {
    const auto& p = br::active();

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_shadow(jtui::theme::elevation().level_2);
    card->set_frame(card_frame);
    root.append_child(std::move(card));

    // 标题行
    auto title = std::make_unique<jtui::Text>(tr(title_key));
    title->set_font_size(14.0F);
    title->set_bold(true);
    title->set_color(p.text_primary);
    title->set_frame(jtui::RectF{
        card_frame.x + 20.0F, card_frame.y + 16.0F, 240.0F, 22.0F});
    root.append_child(std::move(title));

    auto range = std::make_unique<jtui::Text>(tr(range_key));
    range->set_font_size(11.0F);
    range->set_color(p.text_muted);
    range->set_alignment(jtui::TextAlignment::Trailing);
    range->set_frame(jtui::RectF{
        card_frame.x + card_frame.width - 20.0F - 140.0F,
        card_frame.y + 18.0F, 140.0F, 18.0F});
    root.append_child(std::move(range));

    // Sparkline
    auto spark = std::make_unique<Sparkline>();
    spark->set_data(std::move(data));
    spark->set_palette(p.chart_line, p.chart_fill, p.chart_grid);
    spark->set_frame(jtui::RectF{
        card_frame.x + 20.0F, card_frame.y + 50.0F,
        card_frame.width - 40.0F, card_frame.height - 70.0F});
    root.append_child(std::move(spark));
}

void build_chart_row(hui::Panel& root) {
    const float y = kTitleBarH + kNavBarH + 24.0F + 56.0F + 24.0F + 110.0F + 20.0F;  // 322
    constexpr float kCardH = 240.0F;
    constexpr float kCardGap = 20.0F;
    const float total_w = kWindowWidth - 2.0F * kSidePad;
    const float card_w = (total_w - kCardGap) / 2.0F;

    build_chart_card(root, "chart.volume.title", "chart.volume.range",
                     volume_data(),
                     jtui::RectF{kSidePad, y, card_w, kCardH});

    build_chart_card(root, "chart.latency.title", "chart.latency.range",
                     latency_data(),
                     jtui::RectF{kSidePad + card_w + kCardGap, y, card_w, kCardH});
}

// ───────── 底部事件条 ───────────────────────────────────────────────

void build_events_strip(hui::Panel& root) {
    const auto& p = br::active();
    const float y = kTitleBarH + kNavBarH + 24.0F + 56.0F + 24.0F + 110.0F + 20.0F + 240.0F + 20.0F;  // 582
    constexpr float kStripH = 56.0F;
    const float total_w = kWindowWidth - 2.0F * kSidePad;

    // 容器（不浮起 elevation level_0，贴底色）
    auto strip = std::make_unique<jtui::Panel>();
    strip->set_role(jtui::PanelRole::Base);
    strip->set_background_color(p.bg_panel);
    strip->set_corner_radius(br::radius_md);
    strip->set_border(p.border, 1.0F);  // 这里保留 1px 边框：低层级元素用边框够，不再压阴影栈
    strip->set_frame(jtui::RectF{kSidePad, y, total_w, kStripH});
    root.append_child(std::move(strip));

    // 标题小字
    auto label = std::make_unique<jtui::Text>(tr("events.title"));
    label->set_font_size(11.0F);
    label->set_bold(true);
    label->set_color(p.gold);  // 黄金点缀
    label->set_frame(jtui::RectF{
        kSidePad + 20.0F, y + 8.0F, 200.0F, 16.0F});
    root.append_child(std::move(label));

    // 3 条 event 横排
    const std::vector<std::string> rows = {
        tr("events.row.1"), tr("events.row.2"), tr("events.row.3"),
    };
    const float row_w = (total_w - 2.0F * 20.0F) / 3.0F;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        auto t = std::make_unique<jtui::Text>(rows[i]);
        t->set_font_size(12.0F);
        t->set_color(p.text_secondary);
        t->set_frame(jtui::RectF{
            kSidePad + 20.0F + static_cast<float>(i) * row_w,
            y + 28.0F, row_w - 12.0F, 22.0F});
        root.append_child(std::move(t));
    }
}

// ───────── Footer ───────────────────────────────────────────────────

void build_footer(hui::Panel& root) {
    const auto& p = br::active();
    auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
    footer->set_font_size(12.0F);
    footer->set_color(p.text_muted);
    footer->set_alignment(jtui::TextAlignment::Center);
    footer->set_frame(jtui::RectF{0.0F, kWindowHeight - 28.0F, kWindowWidth, 18.0F});
    root.append_child(std::move(footer));
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
    build_page_header(*root);
    build_kpi_row(*root);
    build_chart_row(*root);
    build_events_strip(*root);
    build_footer(*root);

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
    options.title     = "jtUI Atlas";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    ja::register_strings();
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
