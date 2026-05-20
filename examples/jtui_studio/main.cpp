// jtui_studio —— jtUI Studio 子产品 hero example。
// 葡萄紫 accent #7C3AED + "Build Media-Native. Native FAST." 调性。
//
// 视觉范式：左栏双媒体卡片堆叠（上 VideoPlayer test.mp4 + 下 AudioPlayer test.mp3
// 配合 WaveformView 显示峰值波形），右栏品牌 hero 文字（pill + 双行大标题 + 副
// 文 + 3 feature bullet + 主次 CTA）。窗口宽 1280。
//
// 关键 demo：把 widgets/media 的 VideoPlayer / AudioPlayer / WaveformView 套上
// 真品牌外观跑出来，验证框架的"业务侧只组装、不再写 brand 副本"政策能撑住媒体
// 类 widget。
//
// 维护：曾能混 <jwhna1@gmail.com>

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
#include "i18n_catalog.hpp"
#include "_shared/about_dialog.hpp"

namespace {

constexpr float kWindowWidth  = 1280.0F;
constexpr float kWindowHeight = 800.0F;
constexpr float kTitleBarH    = 36.0F;
constexpr float kNavBarH      = 60.0F;
constexpr float kSidePad      = 80.0F;

constexpr float kColW    = 540.0F;   // 左右栏宽
constexpr float kColGap  = 40.0F;    // 两栏 gutter
constexpr float kCardH   = 280.0F;   // 左栏单卡片高
constexpr float kCardGap = 20.0F;    // 左栏 video/audio 卡片纵向间距

namespace js = jtui_studio;
namespace br = jtui_studio::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// ───────── 跨 rebuild 持久存活的重型媒体 widget ─────────────────────
//
// 切换主题 / i18n 走整树 rebuild，但 VideoPlayer / AudioPlayer / WaveformView
// 持有 decoder + WASAPI output + GPU texture，销毁/重建既会打断播放又造成
// 切换卡顿。这里把它们的 unique_ptr 提到 app 作用域持有，build_root 时 move
// 到新树，rebuild 时先从旧 root release_child 回来。
//
// 流程：
//   1. 首次 build_root：persist.video == nullptr → 创建并 set_source；之后
//      append_child(std::move(persist.video)) 把 ownership 给 root，raw ptr
//      留在 persist.video_raw 供下次 release 用。
//   2. rebuild 闭包顶部：从旧 root 调 release_child<T>(persist.X_raw) 把
//      ownership 拿回 persist.X；然后 build_root 再 move 进新 root。
struct PersistentWidgets {
    std::unique_ptr<jtui::VideoPlayer>  video;
    std::unique_ptr<jtui::AudioPlayer>  audio;
    std::unique_ptr<jtui::WaveformView> wave;

    // append_child 把 unique_ptr 移走后，留 raw ptr 给下次 release_child 用。
    jtui::VideoPlayer*  video_raw {nullptr};
    jtui::AudioPlayer*  audio_raw {nullptr};
    jtui::WaveformView* wave_raw  {nullptr};
};

// ───────── 跨平台找 test.mp4 / test.mp3 ─────────────────────────────
// Windows：从 exe 旁查（CMake POST_BUILD 拷过去），再 assets/，最后 basename。
// Linux：源码目录或当前目录；jtUI 的 Application 在非 Windows 下 run 直接 return，
// 不会真去 decode，所以找不到也无所谓。

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

    // 左：jtUI Studio logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_studio = std::make_unique<jtui::Text>("Studio");
        logo_studio->set_font_size(18.0F);
        logo_studio->set_bold(true);
        logo_studio->set_color(p.accent);
        logo_studio->set_frame(jtui::RectF{kSidePad + 44.0F, top, 80.0F, kNavBarH});
        root.append_child(std::move(logo_studio));
    }

    // 中：VIDEO / AUDIO / STUDIO
    {
        const std::vector<std::string> link_keys = {
            "nav.video", "nav.audio", "nav.studio"};
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

// ───────── 左栏 Video 卡片 ──────────────────────────────────────────

void build_video_card(hui::Panel& root, PersistentWidgets& persist,
                      const std::string& source_path) {
    const auto& p = br::active();
    const float card_x = kSidePad;
    const float card_y = kTitleBarH + kNavBarH + 40.0F;
    const float card_w = kColW;
    const float card_h = kCardH;

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    // v1.22: 不用 1px 描边，靠 elevation 阴影分割卡片与底窗（更现代的卡片视觉）
    card->set_shadow(jtui::theme::elevation().level_2);
    card->set_frame(jtui::RectF{card_x, card_y, card_w, card_h});
    root.append_child(std::move(card));

    {
        auto icon = std::make_unique<jtui::CodiconIcon>("device-camera");
        icon->set_color(p.accent);
        icon->set_size_px(16.0F);
        icon->set_frame(jtui::RectF{card_x + 16.0F, card_y + 18.0F, 16.0F, 16.0F});
        root.append_child(std::move(icon));

        auto title = std::make_unique<jtui::Text>(tr("card.video.title"));
        title->set_font_size(13.0F);
        title->set_bold(true);
        title->set_color(p.text_primary);
        title->set_frame(jtui::RectF{card_x + 38.0F, card_y + 14.0F, card_w - 50.0F, 22.0F});
        root.append_child(std::move(title));

        auto caption = std::make_unique<jtui::Text>(tr("card.video.caption"));
        caption->set_font_size(11.0F);
        caption->set_color(p.text_muted);
        caption->set_frame(jtui::RectF{card_x + 38.0F, card_y + 34.0F, card_w - 50.0F, 16.0F});
        root.append_child(std::move(caption));
    }

    // VideoPlayer 自带 transport，占据卡片主体区。跨 rebuild 持久存活：
    // 首次创建 + set_source，之后复用，避免重新解码 + 重启 WASAPI。
    {
        if (!persist.video) {
            persist.video = std::make_unique<jtui::VideoPlayer>();
            if (!source_path.empty()) {
                persist.video->set_source(source_path);
            }
        }
        persist.video->set_frame(jtui::RectF{
            card_x + 16.0F, card_y + 60.0F, card_w - 32.0F, card_h - 76.0F});
        persist.video_raw = persist.video.get();
        root.append_child(std::move(persist.video));
    }
}

// ───────── 左栏 Audio 卡片 ──────────────────────────────────────────

void build_audio_card(hui::Panel& root, PersistentWidgets& persist,
                      const std::string& source_path) {
    const auto& p = br::active();
    const float card_x = kSidePad;
    const float card_y = kTitleBarH + kNavBarH + 40.0F + kCardH + kCardGap;
    const float card_w = kColW;
    const float card_h = kCardH;

    auto card = std::make_unique<jtui::Panel>();
    card->set_role(jtui::PanelRole::Base);
    card->set_background_color(p.bg_card);
    card->set_corner_radius(br::radius_md);
    card->set_shadow(jtui::theme::elevation().level_2);
    card->set_frame(jtui::RectF{card_x, card_y, card_w, card_h});
    root.append_child(std::move(card));

    {
        auto icon = std::make_unique<jtui::CodiconIcon>("music");
        icon->set_color(p.accent);
        icon->set_size_px(16.0F);
        icon->set_frame(jtui::RectF{card_x + 16.0F, card_y + 18.0F, 16.0F, 16.0F});
        root.append_child(std::move(icon));

        auto title = std::make_unique<jtui::Text>(tr("card.audio.title"));
        title->set_font_size(13.0F);
        title->set_bold(true);
        title->set_color(p.text_primary);
        title->set_frame(jtui::RectF{card_x + 38.0F, card_y + 14.0F, card_w - 50.0F, 22.0F});
        root.append_child(std::move(title));

        auto caption = std::make_unique<jtui::Text>(tr("card.audio.caption"));
        caption->set_font_size(11.0F);
        caption->set_color(p.text_muted);
        caption->set_frame(jtui::RectF{card_x + 38.0F, card_y + 34.0F, card_w - 50.0F, 16.0F});
        root.append_child(std::move(caption));
    }

    // WaveformView + AudioPlayer 联动。跨 rebuild 持久存活：
    // 首次创建时 set_source + connect signal + set_peaks；之后复用，避免重复
    // connect（重复 connect 会让一次 position 变化触发 N 次 set_progress）。
    {
        const bool first_build = !persist.audio;
        if (first_build) {
            persist.audio = std::make_unique<jtui::AudioPlayer>();
            if (!source_path.empty()) {
                persist.audio->set_source(source_path);
            }
            persist.wave = std::make_unique<jtui::WaveformView>();
            persist.wave->set_tone(jtui::theme::Tone::Accent);
            persist.wave->set_peaks(persist.audio->peaks());

            jtui::AudioPlayer*  ap = persist.audio.get();
            jtui::WaveformView* wv = persist.wave.get();
            ap->on_position_changed().connect([wv, ap](double pos) {
                const double dur = ap->duration();
                const float prog = dur > 0.0 ? static_cast<float>(pos / dur) : 0.0F;
                wv->set_progress(prog);
            });
        }

        persist.wave->set_frame(jtui::RectF{
            card_x + 16.0F, card_y + 60.0F, card_w - 32.0F, 140.0F});
        persist.wave_raw = persist.wave.get();
        root.append_child(std::move(persist.wave));

        persist.audio->set_frame(jtui::RectF{
            card_x + 16.0F, card_y + 210.0F, card_w - 32.0F, 60.0F});
        persist.audio_raw = persist.audio.get();
        root.append_child(std::move(persist.audio));
    }
}

// ───────── 右栏 Hero 文字 ───────────────────────────────────────────

void build_hero_right(hui::Panel& root) {
    const auto& p = br::active();
    const float col_x = kSidePad + kColW + kColGap;
    const float top = kTitleBarH + kNavBarH + 40.0F;
    const float col_w = kWindowWidth - col_x - kSidePad;

    // pill: "v0.3  ·  Media pipeline shipped on Windows"
    {
        constexpr float kPillH = 32.0F;
        const float pill_y = top + 8.0F;

        auto pill = std::make_unique<jtui::Panel>();
        pill->set_role(jtui::PanelRole::Base);
        pill->set_background_color(p.bg_card);
        pill->set_corner_radius(kPillH * 0.5F);
        pill->set_border(p.border, 1.0F);
        pill->set_frame(jtui::RectF{col_x, pill_y, col_w * 0.92F, kPillH});
        root.append_child(std::move(pill));

        constexpr float kTagW = 56.0F;
        auto tag_bg = std::make_unique<jtui::Panel>();
        tag_bg->set_role(jtui::PanelRole::Base);
        tag_bg->set_background_color(p.accent_soft);
        tag_bg->set_corner_radius((kPillH - 6.0F) * 0.5F);
        tag_bg->set_frame(jtui::RectF{col_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag_bg));

        auto tag = std::make_unique<jtui::Text>(tr("hero.pill.tag"));
        tag->set_font_size(11.0F);
        tag->set_bold(true);
        tag->set_color(p.accent);
        tag->set_alignment(jtui::TextAlignment::Center);
        tag->set_frame(jtui::RectF{col_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag));

        auto pill_text = std::make_unique<jtui::Text>(tr("hero.pill.text"));
        pill_text->set_font_size(12.0F);
        pill_text->set_color(p.text_primary);
        pill_text->set_frame(jtui::RectF{
            col_x + kTagW + 10.0F, pill_y, col_w * 0.92F - kTagW - 16.0F, kPillH});
        root.append_child(std::move(pill_text));
    }

    // 主标题两行：line1 "Build Media-Native." line2 "Native FAST."
    {
        const float title_y = top + 60.0F;
        constexpr float kLineH = 56.0F;
        constexpr float kFont  = 42.0F;

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
        line1->set_frame(jtui::RectF{col_x, title_y, col_w, kLineH});
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
        line2->set_frame(jtui::RectF{col_x, title_y + kLineH, col_w, kLineH});
        root.append_child(std::move(line2));
    }

    // 副文 2 行
    {
        const float sub_y = top + 196.0F;
        constexpr float kSubH = 22.0F;

        auto sub1 = std::make_unique<jtui::Text>(tr("hero.sub.line1"));
        sub1->set_font_size(14.0F);
        sub1->set_color(p.text_secondary);
        sub1->set_frame(jtui::RectF{col_x, sub_y, col_w, kSubH});
        root.append_child(std::move(sub1));

        auto sub2 = std::make_unique<jtui::Text>(tr("hero.sub.line2"));
        sub2->set_font_size(14.0F);
        sub2->set_color(p.text_secondary);
        sub2->set_frame(jtui::RectF{col_x, sub_y + kSubH + 4.0F, col_w, kSubH});
        root.append_child(std::move(sub2));
    }

    // 3 feature bullets（check icon + text）
    {
        const float feat_y = top + 266.0F;
        constexpr float kRowH = 26.0F;
        constexpr float kRowGap = 6.0F;
        constexpr float kCheckSize = 14.0F;
        constexpr float kCheckTextGap = 10.0F;
        const std::vector<std::string> feats = {
            tr("hero.feat.1"), tr("hero.feat.2"), tr("hero.feat.3")};

        for (std::size_t i = 0; i < feats.size(); ++i) {
            const float row_y = feat_y + static_cast<float>(i) * (kRowH + kRowGap);

            auto chk = std::make_unique<jtui::CodiconIcon>("check");
            chk->set_color(p.accent);
            chk->set_size_px(kCheckSize);
            chk->set_frame(jtui::RectF{
                col_x, row_y + (kRowH - kCheckSize) * 0.5F,
                kCheckSize, kCheckSize});
            root.append_child(std::move(chk));

            auto txt = std::make_unique<jtui::Text>(feats[i]);
            txt->set_font_size(13.0F);
            txt->set_color(p.text_secondary);
            txt->set_frame(jtui::RectF{
                col_x + kCheckSize + kCheckTextGap, row_y,
                col_w - kCheckSize - kCheckTextGap, kRowH});
            root.append_child(std::move(txt));
        }
    }

    // 主 + 次 CTA
    {
        const float btn_y = top + 380.0F;
        constexpr float kBtnH = 44.0F;
        constexpr float kPrimaryW = 160.0F;
        constexpr float kSecondaryW = 150.0F;
        constexpr float kBtnGap = 12.0F;

        auto primary = std::make_unique<jtui::Button>(tr("hero.cta.primary"));
        primary->set_shape(jtui::ButtonShape::Pill);
        primary->set_colors(p.accent, p.accent_hover, p.accent_pressed, p.accent_fg);
        primary->set_font_size(14.0F, true);
        primary->set_frame(jtui::RectF{col_x, btn_y, kPrimaryW, kBtnH});
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
            col_x + kPrimaryW + kBtnGap, btn_y, kSecondaryW, kBtnH});
        root.append_child(std::move(secondary));
    }
}

// ───────── Footer ───────────────────────────────────────────────────

void build_footer(hui::Panel& root) {
    const auto& p = br::active();
    auto footer = std::make_unique<jtui::Text>(tr("footer.author"));
    footer->set_font_size(12.0F);
    footer->set_color(p.text_muted);
    footer->set_alignment(jtui::TextAlignment::Center);
    footer->set_frame(jtui::RectF{0.0F, kWindowHeight - 32.0F, kWindowWidth, 18.0F});
    root.append_child(std::move(footer));
}

// ───────── 整树 + rebuild 自指 ─────────────────────────────────────

std::unique_ptr<jtui::Panel> build_root(const RebuildFn& rebuild,
                                        PersistentWidgets& persist,
                                        const std::string& video_path,
                                        const std::string& audio_path) {
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
    build_video_card(*root, persist, video_path);
    build_audio_card(*root, persist, audio_path);
    build_hero_right(*root);
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
    options.title     = "jtUI Studio";
    options.frameless = true;
    options.size      = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui::i18n::set_locale(jtui::i18n::Locale::En);
    js::register_strings();
    jtui_about::register_about_strings();

    const std::string video_path = find_media_file("test.mp4");
    const std::string audio_path = find_media_file("test.mp3");

    // 持久 widget state 必须比 rebuild 闭包活得久；用 shared_ptr 让闭包能跨
    // rebuild 共享同一份 state，rebuild 不打断播放。
    auto persist = std::make_shared<PersistentWidgets>();

    auto rebuild_holder = std::make_shared<RebuildFn>();
    *rebuild_holder = [&window, persist, video_path, audio_path, rebuild_holder]() {
        // 旧 root 还挂在 window.content() 上，从中取回媒体 widget 的 ownership
        // 再触发 set_content（旧 root 进 pending_destroy_ 时已无持久 widget 在内）。
        if (auto* old_content = window.content()) {
            if (persist->video_raw) {
                persist->video = old_content->release_child<jtui::VideoPlayer>(persist->video_raw);
                persist->video_raw = nullptr;
            }
            if (persist->audio_raw) {
                persist->audio = old_content->release_child<jtui::AudioPlayer>(persist->audio_raw);
                persist->audio_raw = nullptr;
            }
            if (persist->wave_raw) {
                persist->wave = old_content->release_child<jtui::WaveformView>(persist->wave_raw);
                persist->wave_raw = nullptr;
            }
        }
        window.set_content(build_root(*rebuild_holder, *persist, video_path, audio_path));
    };

    window.set_content(build_root(*rebuild_holder, *persist, video_path, audio_path));
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
