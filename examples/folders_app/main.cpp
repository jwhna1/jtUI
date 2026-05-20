// folders_app —— 基于 jtUI 实现的桌面文件管理风格演示（jtUI Folders 子品牌）。
// 维护：曾能混 <jwhna1@gmail.com>
//
// 本轮新增（2026-05-14）：
//   - 双主题 dark / light（点击 nav 右侧 sun/moon 切换）
//   - 中英双语 EN / 中（点击 nav 右侧 globe 切换）
//   - search 输入接 mock 过滤：title 含输入子串即显示
//   - lang/theme 走"整树 rebuild"，search 只 rebuild folder grid 一块

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "jtui/jtui.hpp"

#include "app_state.hpp"
#include "brand_tokens.hpp"
#include "i18n_catalog.hpp"
#include "_shared/about_dialog.hpp"

namespace {

constexpr float kWindowWidth   = 1440.0F;
constexpr float kWindowHeight  = 1320.0F;
constexpr float kTitleBarH     = 36.0F;
constexpr float kNavBarH       = 56.0F;
constexpr float kHeroH         = 380.0F;
constexpr float kSidePad       = 80.0F;

namespace fa = folders_app;
namespace br = folders_app::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// 大小写不敏感子串包含
bool contains_ci(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto to_lower = [](unsigned char c) { return std::tolower(c); };
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [&](char a, char b) { return to_lower(static_cast<unsigned char>(a))
                                  == to_lower(static_cast<unsigned char>(b)); });
    return it != haystack.end();
}

// ───────── NavBar ─────────────────────────────────────────────────

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

    // 左：jtUI + Folders logo（jtUI 主品牌 + accent 子品牌名）
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_sub = std::make_unique<jtui::Text>("Folders");
        logo_sub->set_font_size(18.0F);
        logo_sub->set_bold(true);
        logo_sub->set_color(p.accent);
        logo_sub->set_frame(jtui::RectF{kSidePad + 44.0F, top, 90.0F, kNavBarH});
        root.append_child(std::move(logo_sub));

        auto reg = std::make_unique<jtui::Text>("®");
        reg->set_font_size(12.0F);
        reg->set_color(p.text_muted);
        reg->set_frame(jtui::RectF{kSidePad + 44.0F + 72.0F, top, 14.0F, kNavBarH});
        root.append_child(std::move(reg));
    }

    // 右：lang / theme 切换 + PRICING / FEATURES + START FOR FREE
    {
        constexpr float kBtnW       = 130.0F;
        constexpr float kBtnH       = 38.0F;
        constexpr float kLinkW      = 90.0F;
        constexpr float kIconBtnSz  = 36.0F;
        constexpr float kIconBtnGap = 8.0F;

        const float right_x = kWindowWidth - kSidePad;
        const float btn_y   = top + (kNavBarH - kBtnH) * 0.5F;
        const float icon_y  = top + (kNavBarH - kIconBtnSz) * 0.5F;

        // 主 CTA
        auto cta = std::make_unique<jtui::Button>(tr("nav.cta"));
        cta->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_primary);
        cta->set_border(p.border_strong, 1.0F);
        cta->set_corner_radius(8.0F);
        cta->set_font_size(12.0F, true);
        cta->set_frame(jtui::RectF{right_x - kBtnW, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));

        // theme 切换：codicon 字典没有 sun/moon，用专门的 color-mode
        // （视觉上是半亮半暗的圆，恰好表达"主题切换"）
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

        // lang 切换
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

        // PRICING / FEATURES 链接
        auto features = std::make_unique<jtui::Text>(tr("nav.features"));
        features->set_font_size(13.0F);
        features->set_bold(true);
        features->set_color(p.text_secondary);
        // X 公式从 2× 改 3× —— 给中间的 About 圆按钮（在 lang 左侧）让出 44px，
        // 否则 features Text 会 z-order 覆盖 About 按钮拦住 click（用户报告："有
        // 按钮但是点击无响应"，根因即此）。
        features->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 3.0F - kIconBtnSz * 3.0F - kLinkW - 16.0F,
            top, kLinkW, kNavBarH});
        root.append_child(std::move(features));

        auto pricing = std::make_unique<jtui::Text>(tr("nav.pricing"));
        pricing->set_font_size(13.0F);
        pricing->set_bold(true);
        pricing->set_color(p.text_secondary);
        pricing->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 3.0F - kIconBtnSz * 3.0F - kLinkW * 2.0F - 32.0F,
            top, kLinkW, kNavBarH});
        root.append_child(std::move(pricing));
    }
}

// ───────── Hero ────────────────────────────────────────────────────

void build_hero(hui::Panel& root) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH;
    const float center_x = kWindowWidth * 0.5F;

    // pill
    {
        constexpr float kPillW = 220.0F;
        constexpr float kPillH = 32.0F;
        const float pill_x = center_x - kPillW * 0.5F;
        const float pill_y = top + 24.0F;

        auto pill = std::make_unique<jtui::Panel>();
        pill->set_role(jtui::PanelRole::Base);
        pill->set_background_color(p.bg_card);
        pill->set_corner_radius(kPillH * 0.5F);
        pill->set_border(p.border, 1.0F);
        pill->set_frame(jtui::RectF{pill_x, pill_y, kPillW, kPillH});
        root.append_child(std::move(pill));

        constexpr float kTagW = 56.0F;
        auto news_bg = std::make_unique<jtui::Panel>();
        news_bg->set_role(jtui::PanelRole::Base);
        news_bg->set_background_color(p.accent_soft);
        news_bg->set_corner_radius((kPillH - 6.0F) * 0.5F);
        news_bg->set_frame(jtui::RectF{pill_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(news_bg));

        auto news_label = std::make_unique<jtui::Text>(tr("hero.pill.tag"));
        news_label->set_font_size(11.0F);
        news_label->set_bold(true);
        news_label->set_color(p.accent);
        news_label->set_alignment(jtui::TextAlignment::Center);
        news_label->set_frame(jtui::RectF{pill_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(news_label));

        auto pill_text = std::make_unique<jtui::Text>(tr("hero.pill.text"));
        pill_text->set_font_size(12.0F);
        pill_text->set_color(p.text_primary);
        pill_text->set_alignment(jtui::TextAlignment::Center);
        pill_text->set_frame(jtui::RectF{
            pill_x + kTagW, pill_y, kPillW - kTagW - 6.0F, kPillH});
        root.append_child(std::move(pill_text));
    }

    // 大标题
    {
        const float title_y = top + 72.0F;
        constexpr float kLineH = 56.0F;

        auto line1 = std::make_unique<jtui::Text>(tr("hero.title.line1"));
        line1->set_font_size(40.0F);
        line1->set_bold(true);
        line1->set_color(p.text_strong);
        line1->set_alignment(jtui::TextAlignment::Center);
        line1->set_frame(jtui::RectF{0.0F, title_y, kWindowWidth, kLineH});
        root.append_child(std::move(line1));

        auto line2 = std::make_unique<jtui::Text>(tr("hero.title.line2"));
        line2->set_font_size(40.0F);
        line2->set_bold(true);
        line2->set_color(p.text_strong);
        line2->set_alignment(jtui::TextAlignment::Center);
        line2->set_frame(jtui::RectF{0.0F, title_y + kLineH, kWindowWidth, kLineH});
        root.append_child(std::move(line2));
    }

    // 副文
    {
        const float sub_y = top + 200.0F;
        constexpr float kSubH = 22.0F;

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

    // CTA pill
    {
        constexpr float kBtnW = 220.0F;
        constexpr float kBtnH = 48.0F;
        const float btn_y = top + 268.0F;

        auto cta = std::make_unique<jtui::Button>(tr("hero.cta"));
        cta->set_shape(jtui::ButtonShape::Pill);
        cta->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        cta->set_font_size(14.0F, true);
        cta->set_frame(jtui::RectF{center_x - kBtnW * 0.5F, btn_y, kBtnW, kBtnH});
        root.append_child(std::move(cta));
    }

    // 三个绿勾 + 特性
    {
        const float feat_y = top + 332.0F;
        constexpr float kRowH = 22.0F;
        constexpr float kCheckSize = 14.0F;
        const std::vector<std::string> features = {
            tr("hero.feat.1"), tr("hero.feat.2"), tr("hero.feat.3"),
        };

        constexpr float kItemGap = 24.0F;
        constexpr float kCheckTextGap = 8.0F;
        constexpr float kCharW = 7.5F;
        float total_w = 0.0F;
        std::vector<float> text_widths;
        text_widths.reserve(features.size());
        for (const auto& s : features) {
            const float tw = static_cast<float>(s.size()) * kCharW;
            text_widths.push_back(tw);
            total_w += kCheckSize + kCheckTextGap + tw;
        }
        total_w += kItemGap * static_cast<float>(features.size() - 1);

        float x = center_x - total_w * 0.5F;
        for (std::size_t i = 0; i < features.size(); ++i) {
            auto chk = std::make_unique<jtui::CodiconIcon>("check");
            chk->set_color(p.accent);
            chk->set_size_px(kCheckSize);
            chk->set_frame(jtui::RectF{x, feat_y + (kRowH - kCheckSize) * 0.5F,
                                       kCheckSize, kCheckSize});
            root.append_child(std::move(chk));

            auto txt = std::make_unique<jtui::Text>(features[i]);
            txt->set_font_size(13.0F);
            txt->set_color(p.text_secondary);
            txt->set_frame(jtui::RectF{
                x + kCheckSize + kCheckTextGap, feat_y, text_widths[i], kRowH,
            });
            root.append_child(std::move(txt));

            x += kCheckSize + kCheckTextGap + text_widths[i] + kItemGap;
        }
    }
}

// ───────── Folder Grid（独立可重入）─────────────────────────────────
// search 输入只 rebuild grid 区域，避免整树 rebuild 的开销。
//
// jtui::Panel::clear_children 是 protected，外部不能直接调；这里继承一个
// GridContainer 把它提升为 public（using-declaration 改访问性）。

class GridContainer : public jtui::Panel {
public:
    using jtui::Panel::clear_children;
};

struct GridContext {
    GridContainer*       container {nullptr};
    jtui::Text*          selection_text {nullptr};
    fa::FoldersAppState* state {nullptr};
    float main_x {0.0F};
    float main_w {0.0F};
    float grid_y {0.0F};
    // 整树 rebuild 回调（与 nav theme/lang 切换共用同一把锤子）。
    // 历史上这里挂过 state.selected_folder.changed 局部 listener，但 listener
    // 里捕的 sel_ptr/grid_ptr 在 theme/lang 切换 rebuild 后会 dangling，
    // 第二次点击就崩。统一走 rebuild 后所有指针都是新的，零 dangling 风险。
    const RebuildFn*     rebuild {nullptr};
};

void rebuild_folder_grid(const GridContext& ctx) {
    if (!ctx.container || !ctx.state) return;
    ctx.container->clear_children();

    const auto& p = br::active();
    const auto& folders = ctx.state->folders();
    const std::string query = ctx.state->search_query().get();

    constexpr int kCols = 4;
    constexpr int kRows = 3;
    constexpr float kCardGap = 20.0F;
    constexpr float kRowGap  = 20.0F;
    constexpr float kCardH   = 200.0F;
    const float grid_w = ctx.main_w - 48.0F;
    const float card_w = (grid_w - kCardGap * static_cast<float>(kCols - 1))
                       / static_cast<float>(kCols);

    const std::vector<jtui::Color> hints = {
        jtui::Color::from_hex("#22272D"),
        jtui::Color::from_hex("#3A2A1E"),
        jtui::Color::from_hex("#3A1E2E"),
        jtui::Color::from_hex("#1E3A35"),
        jtui::Color::from_hex("#2E1E3A"),
    };

    // 过滤 + 取前 12 个
    std::vector<std::size_t> filtered;
    filtered.reserve(folders.size());
    for (std::size_t i = 0; i < folders.size(); ++i) {
        // 按当前语言过滤：folders[i].title 是 i18n key，tr 取当前语言显示串
        if (contains_ci(tr(folders[i].title), query)) {
            filtered.push_back(i);
        }
    }
    const std::size_t shown = std::min<std::size_t>(
        filtered.size(), static_cast<std::size_t>(kCols * kRows));

    for (std::size_t k = 0; k < shown; ++k) {
        const std::size_t idx = filtered[k];
        const auto& fd = folders[idx];

        auto card = std::make_unique<jtui::FolderCard>();
        card->set_title(tr(fd.title));
        card->set_subtitle(std::to_string(fd.photo_count) + tr("dash.photos.suffix"));
        card->set_thumb_icon(fd.icon);
        card->set_color_hint(fd.color_hint);
        card->set_locked(fd.locked);

        if (fd.shared) {
            std::vector<jtui::AvatarGroup::Entry> entries = {
                {"Alex Lee", jtui::AvatarStatus::None},
                {"Jordan",   jtui::AvatarStatus::None},
                {"Sam Wu",   jtui::AvatarStatus::None},
                {"Riley",    jtui::AvatarStatus::None},
            };
            card->set_shared_avatars(std::move(entries));
        }

        card->set_palette(
            p.bg_card, p.bg_card_hover, p.border,
            p.folder_body, p.folder_tab, p.folder_thumb,
            p.text_primary, p.text_muted, p.text_secondary,
            p.text_muted, p.bg_card);
        card->set_thumb_color_hints(hints);
        card->set_clickable(true);

        const std::size_t row = k / static_cast<std::size_t>(kCols);
        const std::size_t col = k % static_cast<std::size_t>(kCols);
        const float cx = ctx.main_x + 24.0F
                       + (card_w + kCardGap) * static_cast<float>(col);
        const float cy = ctx.grid_y
                       + (kCardH + kRowGap) * static_cast<float>(row);
        card->set_frame(jtui::RectF{cx, cy, card_w, kCardH});
        card->relayout();

        auto* state_ptr = ctx.state;
        const std::size_t captured_idx = idx;
        const RebuildFn* rebuild_ptr = ctx.rebuild;
        card->on_clicked().connect([state_ptr, captured_idx, rebuild_ptr]() {
            state_ptr->selected_folder().set(static_cast<int>(captured_idx));
            if (rebuild_ptr && *rebuild_ptr) (*rebuild_ptr)();
        });

        ctx.container->append_child(std::move(card));
    }

    // 空结果提示
    if (shown == 0 && !query.empty()) {
        auto empty = std::make_unique<jtui::Text>(
            tr("dash.empty.prefix") + query + tr("dash.empty.suffix"));
        empty->set_font_size(14.0F);
        empty->set_color(p.text_muted);
        empty->set_alignment(jtui::TextAlignment::Center);
        empty->set_frame(jtui::RectF{
            ctx.main_x, ctx.grid_y + 80.0F, ctx.main_w, 24.0F});
        ctx.container->append_child(std::move(empty));
    }
}

// ───────── Dashboard ─────────────────────────────────────────────

void build_dashboard(hui::Panel& root, fa::FoldersAppState& state,
                     const RebuildFn& rebuild) {
    const auto& p = br::active();

    const float top = kTitleBarH + kNavBarH + kHeroH;
    const float dash_h = kWindowHeight - top - 24.0F;
    const float dash_x = kSidePad;
    const float dash_y = top;
    const float dash_w = kWindowWidth - kSidePad * 2.0F;

    auto panel = std::make_unique<jtui::Panel>();
    panel->set_role(jtui::PanelRole::Base);
    panel->set_background_color(p.bg_panel);
    panel->set_corner_radius(br::radius_lg);
    panel->set_border(p.border, 1.0F);
    panel->set_frame(jtui::RectF{dash_x, dash_y, dash_w, dash_h});
    root.append_child(std::move(panel));

    // ── Sidebar ───────────────────────────────────────────────
    constexpr float kSidebarW = 260.0F;
    auto sidebar = std::make_unique<jtui::SidebarNav>();
    auto* sidebar_ptr = sidebar.get();

    sidebar->set_user(state.user().name);
    sidebar->set_user_status(jtui::AvatarStatus::Online);

    sidebar->set_main_items({
        {tr("sidebar.my_folders"), "folder"},
        {tr("sidebar.favorite"),   "star-empty"},
        {tr("sidebar.history"),    "history"},
        {tr("sidebar.invite"),     "person-add"},
    });

    sidebar->set_secondary_items({
        {tr("sidebar.support"),  "info"},
        {tr("sidebar.settings"), "settings-gear"},
    });

    sidebar->set_storage_label(tr("sidebar.storage.label"),
                               tr("sidebar.storage.desc"));
    sidebar->set_storage_used(state.used_space().get());
    sidebar->set_upgrade_label(tr("sidebar.upgrade"));

    sidebar->set_palette(
        p.bg_panel,
        p.border,
        p.text_primary,
        p.text_secondary,
        p.text_muted,
        p.accent,
        p.accent_soft,
        p.accent_fg,
        p.bg_card_hover,   // hover_bg：用更深一档，让浅色主题下 hover 可感知
        p.bg_card);        // storage_card_bg
    sidebar->set_search_palette(
        p.bg_field,
        p.border,
        p.text_secondary,
        p.bg_card_hover,
        p.text_muted);

    sidebar->set_active(state.active_nav().get());

    sidebar->set_frame(jtui::RectF{dash_x, dash_y, kSidebarW, dash_h});
    sidebar->relayout();

    // 恢复已有 search query（rebuild 后保持）
    sidebar->search_input()->set_text(state.search_query().get());

    sidebar->on_main_clicked().connect([&state, &rebuild](std::size_t idx) {
        state.active_nav().set(idx);
        rebuild();
    });

    // search 输入：set state + 整树 rebuild。曾经走"局部 rebuild grid"路径，但
    // 局部路径每次 build_dashboard 都注册一个 changed listener 到 state.search_query，
    // listener 捕获已销毁的 ctx → dangling crash。统一全树 rebuild 是当前 jtUI 缺
    // ScopedConnection 时唯一安全的写法（[[framework-scoped-connection]]）。
    sidebar->search_input()->text_property().changed().connect(
        [&state, &rebuild](const std::string& q) {
            state.search_query().set(q);
            rebuild();
        });

    root.append_child(std::move(sidebar));
    (void)sidebar_ptr;

    // ── 欢迎条 ────────────────────────────────────────────────
    const float main_x = dash_x + kSidebarW + 1.0F;
    const float main_w = dash_w - kSidebarW - 1.0F;
    constexpr float kHeaderH = 64.0F;

    {
        auto welcome = std::make_unique<jtui::Text>(
            tr("dash.welcome.prefix") + state.user().name);
        welcome->set_font_size(16.0F);
        welcome->set_bold(true);
        welcome->set_color(p.text_primary);
        welcome->set_frame(jtui::RectF{
            main_x + 24.0F, dash_y + 16.0F, main_w - 100.0F, 24.0F});
        root.append_child(std::move(welcome));

        auto sub = std::make_unique<jtui::Text>(tr("dash.welcome.sub"));
        sub->set_font_size(12.0F);
        sub->set_color(p.text_muted);
        sub->set_frame(jtui::RectF{
            main_x + 24.0F, dash_y + 40.0F, main_w - 100.0F, 18.0F});
        root.append_child(std::move(sub));

        constexpr float kBellSize = 36.0F;
        auto bell = std::make_unique<jtui::Button>("");
        bell->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, p.text_secondary);
        bell->set_corner_radius(8.0F);
        bell->set_leading_codicon("bell");
        bell->set_frame(jtui::RectF{
            main_x + main_w - 24.0F - kBellSize,
            dash_y + 16.0F + (24.0F - kBellSize * 0.5F) - 6.0F,
            kBellSize, kBellSize});
        root.append_child(std::move(bell));

        auto sep = std::make_unique<jtui::Panel>();
        sep->set_role(jtui::PanelRole::Base);
        sep->set_background_color(p.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(jtui::RectF{main_x, dash_y + kHeaderH, main_w, 1.0F});
        root.append_child(std::move(sep));
    }

    // ── My Folders 标题 + Sort ────────────────────────────────
    constexpr float kSubHeaderH = 40.0F;
    const float sub_header_y = dash_y + kHeaderH + 16.0F;
    {
        auto title = std::make_unique<jtui::Text>(tr("dash.title"));
        title->set_font_size(15.0F);
        title->set_bold(true);
        title->set_color(p.text_primary);
        title->set_frame(jtui::RectF{
            main_x + 24.0F, sub_header_y, 200.0F, kSubHeaderH});
        root.append_child(std::move(title));

        auto sort_label = std::make_unique<jtui::Text>(tr("dash.sort.label"));
        sort_label->set_font_size(12.0F);
        sort_label->set_color(p.text_muted);
        sort_label->set_alignment(jtui::TextAlignment::Trailing);
        sort_label->set_frame(jtui::RectF{
            main_x + main_w - 24.0F - 200.0F, sub_header_y + 12.0F, 80.0F, 18.0F});
        root.append_child(std::move(sort_label));

        auto sort_value = std::make_unique<jtui::Text>(tr("dash.sort.value"));
        sort_value->set_font_size(12.0F);
        sort_value->set_bold(true);
        sort_value->set_color(p.text_primary);
        sort_value->set_alignment(jtui::TextAlignment::Trailing);
        sort_value->set_frame(jtui::RectF{
            main_x + main_w - 24.0F - 110.0F, sub_header_y + 12.0F, 110.0F, 18.0F});
        root.append_child(std::move(sort_value));
    }

    // 当前选中文字反馈
    auto sel = std::make_unique<jtui::Text>(tr("dash.select.empty"));
    sel->set_font_size(11.0F);
    sel->set_color(p.text_muted);
    sel->set_frame(jtui::RectF{
        main_x + 24.0F, sub_header_y + 30.0F, main_w - 48.0F, 16.0F});
    auto* sel_ptr = sel.get();
    root.append_child(std::move(sel));

    // 选中态：build 时一次性根据 state 现值算文本。folder 点击后走整树 rebuild
    // 自然把这段重新跑一遍，不再用 changed listener（会泄漏 dangling sel_ptr）。
    {
        const int sel_idx = state.selected_folder().get();
        if (sel_idx >= 0 && static_cast<std::size_t>(sel_idx) < state.folders().size()) {
            sel_ptr->set_content(
                tr("dash.select.prefix") + tr(state.folders()[static_cast<std::size_t>(sel_idx)].title));
        }
    }

    // ── Folder Grid 容器（独立 Panel，可单独 rebuild）────────────
    const float grid_y = sub_header_y + kSubHeaderH + 24.0F;
    const float grid_h = dash_y + dash_h - grid_y - 24.0F;

    auto grid_container = std::make_unique<GridContainer>();
    grid_container->set_role(jtui::PanelRole::Base);
    grid_container->set_background_color(jtui::Color{0.0F, 0.0F, 0.0F, 0.0F});  // 透明
    grid_container->set_corner_radius(0.0F);
    grid_container->set_frame(jtui::RectF{main_x, grid_y, main_w, grid_h});
    auto* grid_ptr = grid_container.get();
    root.append_child(std::move(grid_container));

    GridContext ctx{grid_ptr, sel_ptr, &state, main_x, main_w, grid_y, &rebuild};
    rebuild_folder_grid(ctx);
}

// ───────── 整树构建 + rebuild 自指 ──────────────────────────────

std::unique_ptr<jtui::Panel> build_root(fa::FoldersAppState& state, const RebuildFn& rebuild) {
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
    build_hero(*root);
    build_dashboard(*root, state, rebuild);

    jtui_about::build_about_modal(
        *root, s_about_open,
        jtui_about::palette_to_about(br::active()),
        [rebuild]() { s_about_open = false; rebuild(); },
        kWindowWidth, kWindowHeight);

    return root;
}

// ───────── App 入口 ───────────────────────────────────────────────

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "jtUI Folders";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    fa::register_strings();
    jtui_about::register_about_strings();

    auto state = std::make_unique<fa::FoldersAppState>();

    // rebuild 自指闭包
    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, state_ptr = state.get(), rebuild_holder]() {
        window.set_content(build_root(*state_ptr, *rebuild_holder));
    };

    window.set_content(build_root(*state, *rebuild_holder));

    const int rc = app.run();
    state.reset();
    return rc;
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
