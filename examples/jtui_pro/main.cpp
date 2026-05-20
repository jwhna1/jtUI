// jtui_pro —— jtUI Pro 子产品 hero example。
// 水青黄金 accent #2DD4BF + "Built for teams that ship." 调性。
//
// 视觉范式借鉴 桌面UI.pdf p2：居中 hero + 顶部流线背景 + 底部 trust 墙。
// 流线用 PaintContext::draw_bezier 实现（5 条不同曲度的贝塞尔），
// trust 墙用 4 个胶囊徽记承载 jtUI 自身的合规标语，语义贴合企业版。
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
#include "generated_bg.hpp"
#include "i18n_catalog.hpp"
#include "_shared/about_dialog.hpp"

// 编译期嵌入的 hero_bg.png 字节数组（CMake configure 时从 assets/ 烤出来）。
// 见 examples/jtui_pro/CMakeLists.txt 的 file(READ ... HEX) 那段。
#include "hero_bg_embedded.h"

namespace {

constexpr float kWindowWidth  = 1280.0F;
constexpr float kWindowHeight = 800.0F;
constexpr float kTitleBarH    = 36.0F;
constexpr float kNavBarH      = 60.0F;
constexpr float kSidePad      = 80.0F;

namespace jp = jtui_pro;
namespace br = jtui_pro::brand;
using jtui::i18n::tr;

using RebuildFn = std::function<void()>;

// About modal 开关 —— static 状态跨 rebuild 保活
static bool s_about_open = false;

// 三态背景模式 —— Streams (默认贝塞尔流线) / Generated (程序化合成 PixelBuffer)
// / Image (PNG 解码后的真实图)。点击 nav 上的 image 按钮循环切换。
enum class BgMode { Streams, Generated, Image };
BgMode g_bg_mode = BgMode::Streams;

// PNG 背景缓存（避免每次 rebuild 都重新解码 550KB 字节流）。
// 数据源 = 编译期嵌入的 hero_bg_embedded.h（kHeroBgPng 字节数组）。
// load_from_memory 内部一次解码成 BGRA8 PixelBuffer，进程级缓存。
const jtui::PixelBuffer& image_bg_cache_() {
    static jtui::PixelBuffer cached = jtui::image::load_from_memory(
        jtui_pro::kHeroBgPng, jtui_pro::kHeroBgPngSize);
    return cached;
}

// PNG 背景 widget —— "cover" 模式 draw_texture：等比缩放让 source 占满 widget
// frame，溢出的边缘被 jtUI runtime 自动 push_clip 截掉（v1.20 加的 clips_self()
// 钩子）。与 CSS background-size: cover 等价，避免非等比拉伸导致的元素变形。
class ImageBgWidget : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "ImageBgWidget";
    }

    // 启用 runtime 自动 clip：业务零 push_clip / pop_clip 心智
    [[nodiscard]] bool clips_self() const noexcept override { return true; }

    void set_fallback(jtui::Color c) noexcept { fallback_ = c; }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        const auto& buf = image_bg_cache_();
        if (buf.empty() || buf.width <= 0 || buf.height <= 0) {
            // fallback：纯色 + 红字提示路径不对
            context.fill_rect(f, fallback_);
            context.draw_text(f, "hero_bg.png not found",
                              jtui::Color::from_hex("#FF5757"),
                              jtui::TextAlignment::Center, 14.0F, true);
            return;
        }

        // 等比 cover：算出比例较大的一边匹配 widget，另一边溢出居中
        const float src_aspect = static_cast<float>(buf.width)
                               / static_cast<float>(buf.height);
        const float dst_aspect = f.width / f.height;
        hui::RectF dest{};
        if (src_aspect > dst_aspect) {
            // source 更宽：高度顶满，宽度等比拉大并水平居中（左右裁）
            dest.height = f.height;
            dest.width  = f.height * src_aspect;
            dest.x      = f.x + (f.width - dest.width) * 0.5F;
            dest.y      = f.y;
        } else {
            // source 更高：宽度顶满，高度等比拉大并垂直居中（上下裁）
            dest.width  = f.width;
            dest.height = f.width / src_aspect;
            dest.x      = f.x;
            dest.y      = f.y + (f.height - dest.height) * 0.5F;
        }
        // runtime 在调本 paint 之前已经 push_clip(self.frame)（因为我们 override
        // clips_self()=true），溢出 widget 的部分自动被 D2D 切掉。业务零心智。
        context.draw_texture(dest, buf);
    }

private:
    jtui::Color fallback_ {};
};

// ───────── 顶部流线（自定义 widget 用 draw_bezier 多笔合成）───────

class StreamLines : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override { return "StreamLines"; }

    void set_color(jtui::Color c) noexcept {
        color_ = c;
        mark_dirty(hui::DirtyFlags::Paint);
    }

    void paint(hui::PaintContext& context) const override {
        const hui::RectF f = frame();
        if (f.width <= 0.0F || f.height <= 0.0F) return;

        // 5 条曲线，从右上飞到中下偏左，模拟"光线划过"
        // 每条曲度不同，alpha 渐弱让远处的更淡
        struct Curve {
            float x0_off, y0_off, c1x_off, c1y_off, c2x_off, c2y_off, x1_off, y1_off;
            float alpha;
            float thickness;
        };
        const std::vector<Curve> curves = {
            // 主线：从右上到中部
            { 0.95F, 0.05F, 0.70F, 0.20F, 0.55F, 0.55F, 0.30F, 0.85F, 1.00F, 2.5F},
            // 上偏远线
            { 1.00F, 0.15F, 0.78F, 0.10F, 0.62F, 0.40F, 0.40F, 0.70F, 0.65F, 1.8F},
            // 下沉线
            { 0.90F, 0.00F, 0.65F, 0.30F, 0.50F, 0.65F, 0.20F, 0.95F, 0.55F, 1.5F},
            // 短弧
            { 0.85F, 0.10F, 0.75F, 0.18F, 0.65F, 0.30F, 0.55F, 0.45F, 0.75F, 1.2F},
            // 长扫线
            { 1.00F, 0.30F, 0.60F, 0.20F, 0.45F, 0.50F, 0.25F, 0.80F, 0.40F, 1.0F},
        };

        for (const auto& cv : curves) {
            const hui::Color c{color_.r, color_.g, color_.b, color_.a * cv.alpha};
            context.draw_bezier(
                hui::PointF{f.x + f.width * cv.x0_off, f.y + f.height * cv.y0_off},
                hui::PointF{f.x + f.width * cv.c1x_off, f.y + f.height * cv.c1y_off},
                hui::PointF{f.x + f.width * cv.c2x_off, f.y + f.height * cv.c2y_off},
                hui::PointF{f.x + f.width * cv.x1_off, f.y + f.height * cv.y1_off},
                c, cv.thickness);
        }
    }

private:
    jtui::Color color_ {};
};

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

    // 左：jtUI Pro logo
    {
        auto logo_jt = std::make_unique<jtui::Text>("jtUI");
        logo_jt->set_font_size(18.0F);
        logo_jt->set_bold(true);
        logo_jt->set_color(p.text_strong);
        logo_jt->set_frame(jtui::RectF{kSidePad, top, 50.0F, kNavBarH});
        root.append_child(std::move(logo_jt));

        auto logo_pro = std::make_unique<jtui::Text>("Pro");
        logo_pro->set_font_size(18.0F);
        logo_pro->set_bold(true);
        logo_pro->set_color(p.accent);
        logo_pro->set_frame(jtui::RectF{kSidePad + 44.0F, top, 60.0F, kNavBarH});
        root.append_child(std::move(logo_pro));
    }

    // 中：PRODUCT / PRICING / SECURITY
    {
        const std::vector<std::string> link_keys = {
            "nav.product", "nav.pricing", "nav.security"};
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

    // 右：lang / theme + Talk to Sales CTA
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
        // X 公式从 3× 改 4×：jtui_pro 已有 bg_btn（symbol-color 背景模式切换按钮）
        // 占据 3× 位置；about 必须再往左挪一格避免完全被 bg_btn z-order 覆盖。
        about_btn->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 4.0F - kIconBtnSz * 4.0F,
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

        // 背景模式三态切换按钮：Streams → Generated → Image → Streams ...
        // 当前模式高亮：Image 用 accent 染色，Generated 用 success 染色，
        // Streams 用 muted 中性，让用户瞥一眼就知道在哪一态。
        auto bg_btn = std::make_unique<jtui::Button>("");
        bg_btn->set_shape(jtui::ButtonShape::Circle);
        const jtui::Color icon_color = (g_bg_mode == BgMode::Image)     ? p.accent
                                     : (g_bg_mode == BgMode::Generated) ? p.success
                                                                        : p.text_secondary;
        bg_btn->set_colors(p.bg_card, p.bg_card_hover, p.bg_field, icon_color);
        bg_btn->set_border(p.border_strong, 1.0F);
        bg_btn->set_leading_codicon("symbol-color");
        bg_btn->set_frame(jtui::RectF{
            right_x - kBtnW - kIconBtnGap * 3.0F - kIconBtnSz * 3.0F,
            icon_y, kIconBtnSz, kIconBtnSz});
        bg_btn->on_clicked().connect([rebuild]() {
            g_bg_mode = (g_bg_mode == BgMode::Streams)   ? BgMode::Generated
                      : (g_bg_mode == BgMode::Generated) ? BgMode::Image
                                                         : BgMode::Streams;
            rebuild();
        });
        root.append_child(std::move(bg_btn));
    }
}

// ───────── Hero 主体（居中）─────────────────────────────────────────

void build_hero(hui::Panel& root) {
    const auto& p = br::active();
    const float top = kTitleBarH + kNavBarH;
    const float center_x = kWindowWidth * 0.5F;

    // pill: "Enterprise · SSO + Audit logs available"
    {
        constexpr float kPillW = 360.0F;
        constexpr float kPillH = 32.0F;
        const float pill_x = center_x - kPillW * 0.5F;
        const float pill_y = top + 80.0F;

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

        auto tag = std::make_unique<jtui::Text>(tr("hero.pill.tag"));
        tag->set_font_size(11.0F);
        tag->set_bold(true);
        tag->set_color(p.accent);
        tag->set_alignment(jtui::TextAlignment::Center);
        tag->set_frame(jtui::RectF{pill_x + 3.0F, pill_y + 3.0F, kTagW, kPillH - 6.0F});
        root.append_child(std::move(tag));

        auto pill_text = std::make_unique<jtui::Text>(tr("hero.pill.text"));
        pill_text->set_font_size(12.0F);
        pill_text->set_color(p.text_primary);
        pill_text->set_alignment(jtui::TextAlignment::Center);
        pill_text->set_frame(jtui::RectF{
            pill_x + kTagW, pill_y, kPillW - kTagW - 6.0F, kPillH});
        root.append_child(std::move(pill_text));
    }

    // 大字标题：双段高亮，居中
    // EN: "Built for {teams} that {ship}."
    // ZH: "为{团队}而生，助{出货}。"
    {
        const float title_y = top + 144.0F;
        constexpr float kLineH = 72.0F;
        constexpr float kFont  = 52.0F;

        auto line = std::make_unique<jtui::Text>();
        line->set_font_size(kFont);
        line->set_bold(true);
        line->set_color(p.text_strong);
        line->set_alignment(jtui::TextAlignment::Center);
        line->set_runs({
            jtui::TextRun{tr("hero.title.seg1")},
            jtui::TextRun{tr("hero.title.seg2"), p.accent},
            jtui::TextRun{tr("hero.title.seg3")},
            jtui::TextRun{tr("hero.title.seg4"), p.accent},
            jtui::TextRun{tr("hero.title.seg5")},
        });
        line->set_frame(jtui::RectF{0.0F, title_y, kWindowWidth, kLineH});
        root.append_child(std::move(line));
    }

    // 副文（居中两行）
    {
        const float sub_y = top + 248.0F;
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

    // CTA：主 + 次（居中）
    {
        const float btn_y = top + 332.0F;
        constexpr float kBtnH = 48.0F;
        constexpr float kPrimaryW   = 180.0F;
        constexpr float kSecondaryW = 160.0F;
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
            jtui::Color{0.0F, 0.0F, 0.0F, 0.0F},
            p.bg_card,
            p.bg_card_hover,
            p.text_primary);
        secondary->set_border(p.border_strong, 1.0F);
        secondary->set_font_size(14.0F, true);
        secondary->set_frame(jtui::RectF{
            start_x + kPrimaryW + kBtnGap, btn_y, kSecondaryW, kBtnH});
        root.append_child(std::move(secondary));
    }
}

// ───────── 底部 trust 徽记墙 ──────────────────────────────────────

void build_trust_wall(hui::Panel& root) {
    const auto& p = br::active();
    const float center_x = kWindowWidth * 0.5F;
    const float wall_y = kWindowHeight - 130.0F;

    // 标题小字
    {
        auto heading = std::make_unique<jtui::Text>(tr("trust.heading"));
        heading->set_font_size(11.0F);
        heading->set_bold(true);
        heading->set_color(p.text_muted);
        heading->set_alignment(jtui::TextAlignment::Center);
        heading->set_frame(jtui::RectF{0.0F, wall_y, kWindowWidth, 18.0F});
        root.append_child(std::move(heading));
    }

    // 4 个胶囊徽记，居中横排
    const std::vector<std::string> trust_keys = {
        "trust.soc2", "trust.gdpr", "trust.saml", "trust.sla",
    };
    constexpr float kBadgeW = 140.0F;
    constexpr float kBadgeH = 38.0F;
    constexpr float kBadgeGap = 12.0F;
    const float total_w = kBadgeW * static_cast<float>(trust_keys.size())
                        + kBadgeGap * static_cast<float>(trust_keys.size() - 1);
    const float start_x = center_x - total_w * 0.5F;
    const float badge_y = wall_y + 32.0F;

    for (std::size_t i = 0; i < trust_keys.size(); ++i) {
        const float bx = start_x + (kBadgeW + kBadgeGap) * static_cast<float>(i);

        // 胶囊底
        auto bg = std::make_unique<jtui::Panel>();
        bg->set_role(jtui::PanelRole::Base);
        bg->set_background_color(p.trust_bg);
        bg->set_corner_radius(kBadgeH * 0.5F);
        bg->set_border(p.border, 1.0F);
        bg->set_frame(jtui::RectF{bx, badge_y, kBadgeW, kBadgeH});
        root.append_child(std::move(bg));

        // 左侧 verified-filled codicon（accent 色，做"已认证"视觉）
        auto check = std::make_unique<jtui::CodiconIcon>("verified-filled");
        check->set_color(p.accent);
        check->set_size_px(14.0F);
        check->set_frame(jtui::RectF{
            bx + 16.0F, badge_y + (kBadgeH - 14.0F) * 0.5F, 14.0F, 14.0F});
        root.append_child(std::move(check));

        // 文字
        auto label = std::make_unique<jtui::Text>(tr(trust_keys[i]));
        label->set_font_size(12.0F);
        label->set_bold(true);
        label->set_color(p.trust_text);
        label->set_alignment(jtui::TextAlignment::Center);
        label->set_frame(jtui::RectF{
            bx + 14.0F, badge_y, kBadgeW - 14.0F, kBadgeH});
        root.append_child(std::move(label));
    }
}

// ───────── 底部署名 ───────────────────────────────────────────────

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

    // 背景层：按 g_bg_mode 三选一，画在所有 nav/hero 之前（最底层）
    switch (g_bg_mode) {
    case BgMode::Streams: {
        auto streams = std::make_unique<StreamLines>();
        streams->set_color(p.stream_line);
        streams->set_frame(jtui::RectF{
            kWindowWidth * 0.5F, kTitleBarH + kNavBarH,
            kWindowWidth * 0.5F, kWindowHeight * 0.7F});
        root->append_child(std::move(streams));
        break;
    }
    case BgMode::Generated: {
        auto gen = std::make_unique<jp::GeneratedBgWidget>();
        gen->set_palette(p.bg_window, p.accent,
                         jtui::theme::Theme::mode() == jtui::theme::ThemeMode::Dark);
        // 占据 navbar 之下整个区域，给 hero 内容当背景
        gen->set_frame(jtui::RectF{
            0.0F, kTitleBarH + kNavBarH,
            kWindowWidth, kWindowHeight - kTitleBarH - kNavBarH});
        root->append_child(std::move(gen));
        break;
    }
    case BgMode::Image: {
        auto img = std::make_unique<ImageBgWidget>();
        img->set_fallback(p.bg_panel);
        img->set_frame(jtui::RectF{
            0.0F, kTitleBarH + kNavBarH,
            kWindowWidth, kWindowHeight - kTitleBarH - kNavBarH});
        root->append_child(std::move(img));
        break;
    }
    }

    build_navbar(*root, rebuild);
    build_hero(*root);
    build_trust_wall(*root);
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
    options.title     = "jtUI Pro";
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
