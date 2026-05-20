// jtui_cinema —— jtUI Cinema 子品牌 hero example。
// PDF 桌面UI.pdf p15 视觉范式：横向视频卡片 carousel，每张缩略图中央浮 play 圆形
// 按钮，左右翻页箭头，底部 COMING SOON 大字水印。
//
// 暖橘 #FB923C accent + 近黑底，slogan "World's First Native Cinematic UI."
//
// 真实交互（dogfood VideoPlayer + persist + rebuild）：
//   1. 5 张卡片分两页（page 0 = 0,1,2；page 1 = 2,3,4），左右箭头切页。
//   2. 点击任意卡片的中央 play → 把全局 VideoPlayer 移到该卡片位置覆盖
//      thumbnail，调 video.play() 真实播 test.mp4。
//   3. 主题 / i18n 切换走整树 rebuild，VideoPlayer 通过 PersistentWidgets
//      跨 rebuild 保活 ([[jtui-persistent-child]])，不打断播放。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fstream>
#endif

#include "jtui/jtui.hpp"

#include "brand_tokens.hpp"
#include "carousel_animator.hpp"
#include "i18n_catalog.hpp"
#include "thumbnail_art.hpp"
#include "_shared/about_dialog.hpp"

namespace {

constexpr float kWindowWidth  = 1280.0F;
constexpr float kWindowHeight = 800.0F;
constexpr float kTitleBarH    = 36.0F;
constexpr float kNavBarH      = 60.0F;
constexpr float kSidePad      = 80.0F;

// 卡片度量：3 张可见 + 左右 padding 占满窗口宽 1280
constexpr float kCardW   = 360.0F;
constexpr float kCardH   = 280.0F;
constexpr float kThumbH  = 200.0F;   // 16:9 缩略图区域
constexpr float kCardGap = 20.0F;

constexpr int   kCardCount = 5;
constexpr int   kPerPage   = 3;
constexpr int   kPageCount = 2;   // 0 显示 0,1,2 ; 1 显示 2,3,4
constexpr float kCardStep  = kCardW + kCardGap;   // 卡片步进 380px

// page → carousel X 偏移（pixel）。page 0 = 0；page 1 = 2 个 step 让 cards 2,3,4 进视野。
[[nodiscard]] constexpr float page_to_offset(int page) noexcept {
    return static_cast<float>(page) * (kCardCount - kPerPage) * kCardStep;
}

namespace jc = jtui_cinema;
namespace br = jtui_cinema::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// 视频条目元数据（id 即 ThumbnailArt 风格 id：0=China 1=Mongolia 2=Vietnam 3=Japan 4=Taiwan）
struct ReelItem {
    int         id;
    std::string title_key;
    std::string region_key;
};

const std::array<ReelItem, kCardCount>& reel_items() {
    static const std::array<ReelItem, kCardCount> items{{
        {0, "reel.item1.title", "reel.item1.region"},
        {1, "reel.item2.title", "reel.item2.region"},
        {2, "reel.item3.title", "reel.item3.region"},
        {3, "reel.item4.title", "reel.item4.region"},
        {4, "reel.item5.title", "reel.item5.region"},
    }};
    return items;
}

// ───────── 业务状态 ─────────────────────────────────────────────────
//
// 跨 rebuild 共享的状态结构。所有交互（翻页、点击卡片 play）改这里再 rebuild。

struct CinemaState {
    int   carousel_page    = 0;     // 0 或 1，决定 next/prev 按钮 enable
    float carousel_offset  = 0.0F;  // 当前 X 偏移（pixel），由 animator 实时写回
    float carousel_target  = 0.0F;  // 目标 X 偏移（pixel），翻页时改
    int   playing_index    = -1;    // -1 = 未播放，0..4 = 哪张卡片正在播
    bool  about_open       = false; // About modal 是否显示
};

// 跨 rebuild 保活的重型 widget。VideoPlayer 持有 decoder 解码器，
// 销毁/重建会打断播放并造成切换卡顿。
struct PersistentWidgets {
    std::unique_ptr<jtui::VideoPlayer> video;
    jtui::VideoPlayer*                 video_raw {nullptr};
    bool                               source_set {false};
};

// ───────── 跨平台找 test.mp4 ────────────────────────────────────────

#if !defined(_WIN32)
std::string probe_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good() ? path : std::string{};
}
#endif

std::string find_media_file(const std::string& basename) {
#if defined(_WIN32)
    wchar_t exe_buf[MAX_PATH] = {0};
    if (GetModuleFileNameW(nullptr, exe_buf, MAX_PATH) > 0) {
        std::wstring exe_path{exe_buf};
        const auto slash = exe_path.find_last_of(L"/\\");
        if (slash != std::wstring::npos) {
            const std::wstring exe_dir = exe_path.substr(0, slash + 1);
            const std::wstring wbase{basename.begin(), basename.end()};
            const std::wstring candidates[] = {
                exe_dir + wbase,
                exe_dir + L"assets\\" + wbase,
                wbase,
            };
            for (const auto& wp : candidates) {
                const DWORD attr = GetFileAttributesW(wp.c_str());
                if (attr == INVALID_FILE_ATTRIBUTES ||
                    (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    continue;
                }
                const int len = WideCharToMultiByte(
                    CP_UTF8, 0, wp.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (len <= 1) continue;
                std::string utf8(static_cast<std::size_t>(len - 1), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wp.c_str(), -1, utf8.data(), len,
                                    nullptr, nullptr);
                return utf8;
            }
        }
    }
    return basename;
#else
    const std::string candidates[] = {
        "./" + basename,
        "./assets/" + basename,
    };
    for (const auto& p : candidates) {
        auto ok = probe_file(p);
        if (!ok.empty()) return ok;
    }
    return basename;
#endif
}

// ───────── NavBar ──────────────────────────────────────────────────

void build_navbar(hui::Panel& root, CinemaState& state, const RebuildFn& rebuild) {
    const auto& p = br::active();
    const float top = kTitleBarH;

    // 底部分割线
    {
        auto sep = std::make_unique<jtui::Panel>();
        sep->set_role(jtui::PanelRole::Base);
        sep->set_background_color(p.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(jtui::RectF{0.0F, top + kNavBarH, kWindowWidth, 1.0F});
        root.append_child(std::move(sep));
    }

    // 左：jtUI Cinema(R) logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_cinema = std::make_unique<jtui::Text>("Cinema");
        logo_cinema->set_font_size(18.0F);
        logo_cinema->set_bold(true);
        logo_cinema->set_color(p.accent);
        logo_cinema->set_frame(jtui::RectF{kSidePad + 44.0F, top, 90.0F, kNavBarH});
        root.append_child(std::move(logo_cinema));

        auto reg_mark = std::make_unique<jtui::Text>("(R)");
        reg_mark->set_font_size(11.0F);
        reg_mark->set_color(p.text_muted);
        reg_mark->set_frame(jtui::RectF{kSidePad + 44.0F + 64.0F, top + 6.0F, 24.0F, 18.0F});
        root.append_child(std::move(reg_mark));
    }

    // 中：FILMS / STACK / SPECS / ABOUT —— 前三非交互 Text，ABOUT 是 ghost Button
    {
        const std::vector<std::string> link_keys = {
            "nav.films", "nav.stack", "nav.specs"};
        constexpr float kLinkW = 100.0F;
        constexpr float kLinkGap = 8.0F;
        const int total_items = static_cast<int>(link_keys.size()) + 1;  // +1 = ABOUT
        const float total_w = kLinkW * static_cast<float>(total_items)
                            + kLinkGap * static_cast<float>(total_items - 1);
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

        // ABOUT 按钮 —— 透明 ghost 样式，hover 时显示 bg_card 提示可点
        constexpr float kAboutBtnH = 32.0F;
        auto about_btn = std::make_unique<jtui::Button>(tr("nav.about"));
        about_btn->set_shape(jtui::ButtonShape::Pill);
        about_btn->set_colors(
            jtui::Color{0.0F, 0.0F, 0.0F, 0.0F},   // idle 透明
            p.bg_card,                              // hover bg_card
            p.bg_card_hover,                        // pressed
            p.accent);                              // 文字 accent
        about_btn->set_font_size(13.0F, true);
        about_btn->set_frame(jtui::RectF{
            start_x + (kLinkW + kLinkGap) * static_cast<float>(link_keys.size()),
            top + (kNavBarH - kAboutBtnH) * 0.5F, kLinkW, kAboutBtnH});
        about_btn->on_clicked().connect([rebuild, &state]() {
            state.about_open = true;
            rebuild();
        });
        root.append_child(std::move(about_btn));
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

// ───────── Hero block：左大标题 + 右翻页按钮 ─────────────────────────

void build_hero_block(hui::Panel& root, CinemaState& state, const RebuildFn& rebuild) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH + 40.0F;

    // 主标题两行（左对齐）
    {
        const float title_x = kSidePad;
        const float title_w = 760.0F;
        constexpr float kLineH = 70.0F;
        constexpr float kFont  = 56.0F;

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
        line1->set_frame(jtui::RectF{title_x, top, title_w, kLineH});
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
        line2->set_frame(jtui::RectF{title_x, top + kLineH, title_w, kLineH});
        root.append_child(std::move(line2));
    }

    // 副文一行
    {
        auto sub = std::make_unique<jtui::Text>(tr("hero.sub.line1"));
        sub->set_font_size(15.0F);
        sub->set_color(p.text_secondary);
        sub->set_frame(jtui::RectF{kSidePad, top + 152.0F, 760.0F, 22.0F});
        root.append_child(std::move(sub));
    }

    // 右侧翻页按钮 [<-] [->]
    {
        constexpr float kArrowSz  = 48.0F;
        constexpr float kArrowGap = 12.0F;

        const float right_x = kWindowWidth - kSidePad;
        const float btn_y   = top + 20.0F;

        const int cur = state.carousel_page;

        auto prev = std::make_unique<jtui::Button>("");
        prev->set_shape(jtui::ButtonShape::Circle);
        const bool prev_enabled = cur > 0;
        prev->set_colors(
            prev_enabled ? p.bg_card : p.bg_field,
            p.bg_card_hover,
            p.bg_field,
            prev_enabled ? p.text_primary : p.text_disabled);
        prev->set_border(p.border_strong, 1.0F);
        prev->set_leading_codicon("chevron-left");
        prev->set_frame(jtui::RectF{
            right_x - kArrowSz * 2.0F - kArrowGap, btn_y, kArrowSz, kArrowSz});
        prev->on_clicked().connect([rebuild, &state]() {
            if (state.carousel_page > 0) {
                state.carousel_page -= 1;
                state.carousel_target = page_to_offset(state.carousel_page);
                // rebuild 把 page 颜色/页码刷新；新 animator 从 state 接力
                // 继续平滑滑动到 target（动画期间 state.carousel_offset 已被
                // 写回到当前实时位置，rebuild 不会造成视觉跳跃）。
                rebuild();
            }
        });
        root.append_child(std::move(prev));

        auto next = std::make_unique<jtui::Button>("");
        next->set_shape(jtui::ButtonShape::Circle);
        const bool next_enabled = cur < kPageCount - 1;
        next->set_colors(
            next_enabled ? p.accent : p.bg_field,
            p.accent_hover,
            p.accent_pressed,
            next_enabled ? p.accent_fg : p.text_disabled);
        next->set_leading_codicon("chevron-right");
        next->set_frame(jtui::RectF{
            right_x - kArrowSz, btn_y, kArrowSz, kArrowSz});
        next->on_clicked().connect([rebuild, &state]() {
            if (state.carousel_page < kPageCount - 1) {
                state.carousel_page += 1;
                state.carousel_target = page_to_offset(state.carousel_page);
                rebuild();
            }
        });
        root.append_child(std::move(next));

        // 页码指示 (1/2)
        auto page_text = std::make_unique<jtui::Text>(
            std::to_string(cur + 1) + " / " + std::to_string(kPageCount));
        page_text->set_font_size(12.0F);
        page_text->set_color(p.text_muted);
        page_text->set_alignment(jtui::TextAlignment::Trailing);
        page_text->set_frame(jtui::RectF{
            right_x - kArrowSz * 2.0F - kArrowGap - 80.0F,
            btn_y + kArrowSz + 6.0F,
            kArrowSz * 2.0F + kArrowGap + 80.0F,
            16.0F});
        root.append_child(std::move(page_text));
    }
}

// ───────── 单张视频卡片 ─────────────────────────────────────────────
//
// 绘制顺序（z 由低到高 = append 顺序）：
//   1. 卡片底（角部圆角，bg_card）
//   2. 缩略图色块（带细微暗化层）
//   3. 中央 play 按钮（圆形，accent 边框）→ 点击触发该卡片播放
//   4. 卡片底部 title + region
//
// 如果该卡片正在播（playing_index == item.id），中央 play 按钮换成
// "PLAYING" 浮标，缩略图位置等待外部把 VideoPlayer 覆盖上去。

// 卡片所有可滑动 widget 都通过 push_carousel 注册到 animator targets，
// 翻页时随 carousel 一起 X 平移。整树扁平 —— 每个 widget 都是 root 的直接子。
template <typename T>
T* push_carousel(hui::Panel& root, std::unique_ptr<T> w,
                 jc::CarouselAnimator* animator) {
    T* raw = w.get();
    root.append_child(std::move(w));
    if (animator != nullptr) animator->add_target(raw);
    return raw;
}

void build_reel_card(hui::Panel& root,
                     CinemaState& state,
                     jc::CarouselAnimator* animator,
                     const RebuildFn& rebuild,
                     const ReelItem& item,
                     float card_x, float card_y) {
    const auto& p = br::active();
    const bool is_playing = (state.playing_index == item.id);

    // 1) 卡片底
    //    corner_radius 从 radius_lg(20) 降到 radius_sm(10)：原 20px 圆角让 ThumbnailArt
    //    几何在 4 个角的圆角扇区内露出小色块（实战截图：China 灯笼角的暖橘小三角飞出
    //    右上圆角）。10px 圆角影响范围只有原 1/4，配合下方 ThumbnailArt 的 clips_self
    //    矩形裁剪，多数越界自然消失。视觉上更"电影画框"感。
    {
        auto card = std::make_unique<jtui::Panel>();
        card->set_role(jtui::PanelRole::Base);
        card->set_background_color(p.bg_card);
        card->set_corner_radius(br::radius_sm);
        card->set_shadow(jtui::theme::elevation().level_2);
        card->set_frame(jtui::RectF{card_x, card_y, kCardW, kCardH});
        push_carousel(root, std::move(card), animator);
    }

    // 2) 缩略图——自定义 ThumbnailArt widget 用 fill_polygon / draw_bezier 等
    //    几何 + 渐变直接 paint 风格化插画（5 种地理）。正在播则改由外部 VideoPlayer
    //    覆盖在卡片缩略图位置（同一矩形），thumbnail 不画。
    if (!is_playing) {
        auto art = std::make_unique<jc::ThumbnailArt>();
        art->set_style(item.id);
        art->set_accent(p.accent);
        art->set_corner_radius(br::radius_sm);
        art->set_frame(jtui::RectF{card_x, card_y, kCardW, kThumbH});
        push_carousel(root, std::move(art), animator);

        // 暗化层（柔化插画 + 让中央 play 按钮更突出）
        auto overlay = std::make_unique<jtui::Panel>();
        overlay->set_role(jtui::PanelRole::Base);
        overlay->set_background_color(p.thumb_overlay);
        overlay->set_corner_radius(br::radius_sm);
        overlay->set_frame(jtui::RectF{card_x, card_y, kCardW, kThumbH});
        push_carousel(root, std::move(overlay), animator);

        // 3) 中央 play 按钮
        constexpr float kPlaySz = 80.0F;
        const float play_x = card_x + (kCardW - kPlaySz) * 0.5F;
        const float play_y = card_y + (kThumbH - kPlaySz) * 0.5F;

        auto play_btn = std::make_unique<jtui::Button>("");
        play_btn->set_shape(jtui::ButtonShape::Circle);
        // 半透白 + accent 描边 → 点击高亮变 accent 填充
        play_btn->set_colors(
            jtui::Color{1.0F, 1.0F, 1.0F, 0.10F},
            jtui::Color{1.0F, 1.0F, 1.0F, 0.22F},
            p.accent_pressed,
            p.accent);
        play_btn->set_border(p.play_ring_idle, 2.0F);
        play_btn->set_leading_codicon("play");
        play_btn->set_font_size(28.0F, false);
        play_btn->set_frame(jtui::RectF{play_x, play_y, kPlaySz, kPlaySz});

        const int item_id = item.id;
        play_btn->on_clicked().connect([&state, animator, rebuild, item_id]() {
            state.playing_index = item_id;
            // 切到该卡片所在 page，让被 active 的卡片可见（滑动而非 rebuild）
            const int new_page = (item_id < kPerPage) ? 0 : 1;
            if (new_page != state.carousel_page) {
                state.carousel_page = new_page;
                state.carousel_target = page_to_offset(new_page);
                if (animator != nullptr) animator->slide_to(state.carousel_target);
            }
            // playing_index 切换是低频结构变化（thumbnail → PLAYING badge + 加 VideoPlayer），
            // 必须 rebuild 重建 widget tree。VideoPlayer 通过 PersistentWidgets 保活。
            rebuild();
        });
        push_carousel(root, std::move(play_btn), animator);
    } else {
        // 占位：cinema 主播放区会在外部把 VideoPlayer 放上来。
        // 这里只在卡片左上角放一个 "PLAYING" 角标。
        auto badge_bg = std::make_unique<jtui::Panel>();
        badge_bg->set_role(jtui::PanelRole::Base);
        badge_bg->set_background_color(p.accent);
        badge_bg->set_corner_radius(br::radius_pill * 0.5F);
        badge_bg->set_frame(jtui::RectF{card_x + 12.0F, card_y + 12.0F, 86.0F, 22.0F});
        push_carousel(root, std::move(badge_bg), animator);

        auto badge = std::make_unique<jtui::Text>(tr("reel.playing"));
        badge->set_font_size(11.0F);
        badge->set_bold(true);
        badge->set_color(p.accent_fg);
        badge->set_alignment(jtui::TextAlignment::Center);
        badge->set_frame(jtui::RectF{card_x + 12.0F, card_y + 12.0F, 86.0F, 22.0F});
        push_carousel(root, std::move(badge), animator);
    }

    // 4) 卡片底部：title + region
    {
        const float info_y = card_y + kThumbH + 16.0F;
        auto title = std::make_unique<jtui::Text>(tr(item.title_key));
        title->set_font_size(15.0F);
        title->set_bold(true);
        title->set_color(p.text_primary);
        title->set_frame(jtui::RectF{card_x + 16.0F, info_y, kCardW - 32.0F, 22.0F});
        push_carousel(root, std::move(title), animator);

        auto region = std::make_unique<jtui::Text>(tr(item.region_key));
        region->set_font_size(12.0F);
        region->set_color(p.text_muted);
        region->set_frame(jtui::RectF{card_x + 16.0F, info_y + 24.0F, kCardW - 32.0F, 18.0F});
        push_carousel(root, std::move(region), animator);
    }
}

// ───────── Reel：5 张并排 + carousel 动画驱动器 + VideoPlayer 跟随 ────
//
// 同时 layout 全部 5 张卡片：每张 X = kSidePad + i*kStepX - state.carousel_offset。
// 翻页时 animator 让 state.carousel_offset 平滑逼近 state.carousel_target，
// 同时对每张卡片的所有 widget（含 VideoPlayer）的 frame.x 同步偏移。
// 屏幕外卡片的 widget 也存在但不在可视区，D2D 自带 clip 会跳过。
//
// animator 在 build_reel 内部创建+ append，翻页按钮回调通过 rebuild 让新一轮
// build_reel 的新 animator 从 state.carousel_target 接力滑动，因此无需返回值。

void build_reel(hui::Panel& root,
                CinemaState& state,
                PersistentWidgets& persist,
                const std::string& video_path,
                const RebuildFn& rebuild) {
    const auto& items = reel_items();
    const float reel_top  = kTitleBarH + kNavBarH + 40.0F + 200.0F;
    const float reel_base = kSidePad - state.carousel_offset;  // 当前左侧起点

    // 先创建 animator 拿到 raw ptr，之后 push_carousel 时注册 targets。
    // animator.frame 覆盖整个 reel 可视区（窗口宽 × kCardH）—— mark_dirty(Paint)
    // 时父级接收的 dirty rect 覆盖所有 target 的新/旧位置并集 → 整块重画，
    // translate_subtree 不打脏标也无残影。
    auto animator_owned = std::make_unique<jc::CarouselAnimator>();
    jc::CarouselAnimator* animator = animator_owned.get();
    animator->set_frame(jtui::RectF{0.0F, reel_top, kWindowWidth, kCardH});
    animator->set_offsets(state.carousel_offset, state.carousel_target);
    // tick 推进 current 时回写 state，主题/i18n rebuild 时新 animator 能从 state 恢复位置
    animator->set_on_current_changed(
        [&state](float new_current) { state.carousel_offset = new_current; });
    animator->set_on_target_changed(
        [&state](float new_target)  { state.carousel_target = new_target; });

    // 5 张卡片并排 layout
    for (int i = 0; i < kCardCount; ++i) {
        const float card_x = reel_base + static_cast<float>(i) * kCardStep;
        build_reel_card(root, state, animator, rebuild, items[i], card_x, reel_top);
    }

    // VideoPlayer 覆盖到 playing 卡片位置，并加入 carousel targets 跟随滑动
    if (state.playing_index >= 0) {
        if (!persist.video) {
            persist.video = std::make_unique<jtui::VideoPlayer>();
        }
        if (!persist.source_set && !video_path.empty()) {
            persist.video->set_source(video_path);
            persist.source_set = true;
        }
        if (persist.video->state() != jtui::VideoPlayer::PlayState::Playing) {
            persist.video->play();
        }

        const float vp_x = reel_base + static_cast<float>(state.playing_index) * kCardStep;
        persist.video->set_frame(jtui::RectF{vp_x, reel_top, kCardW, kThumbH});
        persist.video_raw = persist.video.get();
        animator->add_target(persist.video.get());
        root.append_child(std::move(persist.video));
    }

    // animator 最后 append（visible widget 上方，不画自己但 tick 会被调）
    root.append_child(std::move(animator_owned));
}

// ───────── 底部水印 COMING SOON + footer ───────────────────────────

void build_watermark_and_footer(hui::Panel& root) {
    const auto& p = br::active();

    // 大字水印（极淡，几乎只是轮廓在）
    {
        const float wm_top = kTitleBarH + kNavBarH + 40.0F + 200.0F + kCardH + 8.0F;
        auto wm = std::make_unique<jtui::Text>(tr("hero.watermark"));
        wm->set_font_size(96.0F);
        wm->set_bold(true);
        wm->set_color(p.watermark);
        wm->set_alignment(jtui::TextAlignment::Center);
        wm->set_frame(jtui::RectF{0.0F, wm_top, kWindowWidth, 100.0F});
        root.append_child(std::move(wm));
    }

    // 底部 launching 标
    {
        auto launching = std::make_unique<jtui::Text>(tr("hero.launching"));
        launching->set_font_size(12.0F);
        launching->set_color(p.text_muted);
        launching->set_alignment(jtui::TextAlignment::Trailing);
        launching->set_frame(jtui::RectF{
            0.0F, kWindowHeight - 50.0F,
            kWindowWidth - kSidePad, 18.0F});
        root.append_child(std::move(launching));
    }

    // footer
    {
        auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
        footer->set_font_size(12.0F);
        footer->set_color(p.text_muted);
        footer->set_alignment(jtui::TextAlignment::Center);
        footer->set_frame(jtui::RectF{0.0F, kWindowHeight - 28.0F, kWindowWidth, 18.0F});
        root.append_child(std::move(footer));
    }
}

// ───────── About Modal ─────────────────────────────────────────────
//
// 实现已抽到 examples/_shared/about_dialog.hpp，所有 brand example 共用。
// 这里只负责：把 brand palette 映射成 AboutColors + 调用 jtui_about
// 公共入口。

void build_about_modal_jc(hui::Panel& root, CinemaState& state, const RebuildFn& rebuild) {
    if (!state.about_open) return;
    const auto& p = br::active();

    jtui_about::AboutColors c{};
    c.scrim          = jtui::Color{0.0F, 0.0F, 0.0F, 0.62F};
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

    jtui_about::build_about_modal(
        root, /*open=*/true, c,
        [rebuild, &state]() { state.about_open = false; rebuild(); },
        kWindowWidth, kWindowHeight);
}

[[maybe_unused]] inline void unused_silence_old_about() {
    // 旧 inline 实现（已废弃）保留占位避免名字冲突；本函数永不被调用。
}

#if 0
void build_about_modal_unused(hui::Panel& root, CinemaState& state, const RebuildFn& rebuild) {
    if (!state.about_open) return;
    const auto& p = br::active();

    // 1) scrim：全屏半透黑 + 点击关闭。用 Button 接收点击。
    auto scrim = std::make_unique<jtui::Button>("");
    scrim->set_shape(jtui::ButtonShape::Default);
    scrim->set_corner_radius(0.0F);
    scrim->set_colors(
        jtui::Color{0.0F, 0.0F, 0.0F, 0.62F},
        jtui::Color{0.0F, 0.0F, 0.0F, 0.66F},
        jtui::Color{0.0F, 0.0F, 0.0F, 0.62F},
        jtui::Color{1.0F, 1.0F, 1.0F, 0.0F});
    scrim->set_frame(jtui::RectF{0.0F, 0.0F, kWindowWidth, kWindowHeight});
    scrim->on_clicked().connect([rebuild, &state]() {
        state.about_open = false;
        rebuild();
    });
    root.append_child(std::move(scrim));

    // 2) modal card 居中
    constexpr float kModalW = 520.0F;
    constexpr float kModalH = 480.0F;
    constexpr float kPad    = 32.0F;
    const float card_x = (kWindowWidth  - kModalW) * 0.5F;
    const float card_y = (kWindowHeight - kModalH) * 0.5F;

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_panel);
    card->set_corner_radius(br::radius_xl);
    card->set_border(p.border_strong, 1.0F);
    card->set_shadow(jtui::theme::elevation().level_4);
    card->set_frame(jtui::RectF{card_x, card_y, kModalW, kModalH});
    root.append_child(std::move(card));

    float y = card_y + kPad;

    // jtUI logo 行（jtUI 暖橘 highlight + jtai 团队 tag）
    {
        auto jt = std::make_unique<jtui::Text>("jtUI");
        jt->set_font_size(28.0F);
        jt->set_bold(true);
        jt->set_color(p.accent);
        jt->set_frame(jtui::RectF{card_x + kPad, y, 80.0F, 36.0F});
        root.append_child(std::move(jt));

        // jtai 角标（pill 风格）
        constexpr float kTagW = 72.0F;
        constexpr float kTagH = 22.0F;
        auto tag_bg = std::make_unique<jtui::Panel>();
        tag_bg->set_role(jtui::PanelRole::Base);
        tag_bg->set_background_color(p.accent_soft);
        tag_bg->set_corner_radius(kTagH * 0.5F);
        tag_bg->set_frame(jtui::RectF{
            card_x + kPad + 84.0F, y + 8.0F, kTagW, kTagH});
        root.append_child(std::move(tag_bg));

        auto tag = std::make_unique<jtui::Text>("by jtai");
        tag->set_font_size(11.0F);
        tag->set_bold(true);
        tag->set_color(p.accent);
        tag->set_alignment(jtui::TextAlignment::Center);
        tag->set_frame(jtui::RectF{
            card_x + kPad + 84.0F, y + 8.0F, kTagW, kTagH});
        root.append_child(std::move(tag));
    }
    y += 48.0F;

    // 标题
    {
        auto t = std::make_unique<jtui::Text>(tr("about.title"));
        t->set_font_size(22.0F);
        t->set_bold(true);
        t->set_color(p.text_strong);
        t->set_frame(jtui::RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 32.0F});
        root.append_child(std::move(t));
    }
    y += 36.0F;

    // 副标题
    {
        auto t = std::make_unique<jtui::Text>(tr("about.subtitle"));
        t->set_font_size(14.0F);
        t->set_color(p.text_secondary);
        t->set_frame(jtui::RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 22.0F});
        root.append_child(std::move(t));
    }
    y += 30.0F;

    // 分割线
    {
        auto sep = std::make_unique<jtui::Panel>();
        sep->set_role(jtui::PanelRole::Base);
        sep->set_background_color(p.border);
        sep->set_corner_radius(0.0F);
        sep->set_frame(jtui::RectF{card_x + kPad, y, kModalW - kPad * 2.0F, 1.0F});
        root.append_child(std::move(sep));
    }
    y += 18.0F;

    // meta rows: 标签 + 值（label muted，value primary）
    struct MetaRow { const char* label_key; const char* value_key; bool emphasized; };
    const MetaRow rows[] = {
        {"about.history.label", "about.history.value", false},
        {"about.team.label",    "about.team.value",    false},
        {"about.lead.label",    "about.lead.value",    false},
        {"about.version.label", "about.version.value", false},
        {"about.vision.label",  "about.vision.value",  true },  // vision 用 accent 强调
    };
    constexpr float kRowH    = 30.0F;
    constexpr float kLabelW  = 96.0F;
    for (const auto& mr : rows) {
        auto label = std::make_unique<jtui::Text>(tr(mr.label_key));
        label->set_font_size(12.0F);
        label->set_bold(true);
        label->set_color(p.text_muted);
        label->set_frame(jtui::RectF{card_x + kPad, y + 4.0F, kLabelW, kRowH - 8.0F});
        root.append_child(std::move(label));

        auto value = std::make_unique<jtui::Text>(tr(mr.value_key));
        value->set_font_size(13.0F);
        value->set_bold(mr.emphasized);
        value->set_color(mr.emphasized ? p.accent : p.text_primary);
        value->set_frame(jtui::RectF{
            card_x + kPad + kLabelW, y + 4.0F,
            kModalW - kPad * 2.0F - kLabelW, kRowH - 8.0F});
        root.append_child(std::move(value));

        y += kRowH;
    }

    // Close 按钮（底部居中）
    {
        constexpr float kBtnW = 140.0F;
        constexpr float kBtnH = 40.0F;
        auto close = std::make_unique<jtui::Button>(tr("about.close"));
        close->set_shape(jtui::ButtonShape::Pill);
        close->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        close->set_font_size(13.0F, true);
        close->set_frame(jtui::RectF{
            card_x + (kModalW - kBtnW) * 0.5F,
            card_y + kModalH - kPad - kBtnH,
            kBtnW, kBtnH});
        close->on_clicked().connect([rebuild, &state]() {
            state.about_open = false;
            rebuild();
        });
        root.append_child(std::move(close));
    }
}
#endif  // 旧 inline build_about_modal 已被 _shared/about_dialog.hpp 取代

// ───────── 整树 + rebuild 自指 ─────────────────────────────────────

std::unique_ptr<jtui::Panel> build_root(const RebuildFn& rebuild,
                                        CinemaState& state,
                                        PersistentWidgets& persist,
                                        const std::string& video_path) {
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

    build_navbar(*root, state, rebuild);
    build_hero_block(*root, state, rebuild);
    build_reel(*root, state, persist, video_path, rebuild);
    build_watermark_and_footer(*root);
    build_about_modal_jc(*root, state, rebuild);

    return root;
}

int run_app() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title     = "jtUI Cinema";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    jc::register_strings();
    jtui_about::register_about_strings();

    const std::string video_path = find_media_file("test.mp4");

    auto state   = std::make_shared<CinemaState>();
    auto persist = std::make_shared<PersistentWidgets>();

    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, state, persist, video_path, rebuild_holder]() {
        // 旧 root 还挂在 window.content() 上，从中取回 VideoPlayer 的 ownership
        // 再触发 set_content（旧 root 进 pending_destroy_ 时已无持久 widget 在内）。
        if (auto* old_content = window.content()) {
            if (persist->video_raw) {
                persist->video = old_content->release_child<jtui::VideoPlayer>(persist->video_raw);
                persist->video_raw = nullptr;
            }
        }
        window.set_content(build_root(*rebuild_holder, *state, *persist, video_path));
    };

    window.set_content(build_root(*rebuild_holder, *state, *persist, video_path));
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
