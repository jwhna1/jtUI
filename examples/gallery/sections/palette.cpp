#include "sections/palette.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

namespace {

// 单个色块：在 cpp 里用 const 成员，避免每帧再分配。
struct Swatch {
    const char* label;
    jtui::Color color;
};

struct Row {
    const char* name;
    std::vector<Swatch> swatches;
};

// 整套 Palette 色阶是运行时常量（Theme 单例的 palette() 返回静态数据），一次构造即可。
std::vector<Row> build_palette_rows_once() {
    const auto& p = jtui::theme::palette();
    return {
        {"Green",   {{"50", p.green_50}, {"100", p.green_100}, {"200", p.green_200},
                     {"300", p.green_300}, {"400", p.green_400}, {"500", p.green_500},
                     {"600", p.green_600}, {"700", p.green_700}, {"800", p.green_800},
                     {"900", p.green_900}, {"950", p.green_950}}},
        {"Neutral", {{"0", p.neutral_0}, {"50", p.neutral_50}, {"100", p.neutral_100},
                     {"200", p.neutral_200}, {"300", p.neutral_300}, {"400", p.neutral_400},
                     {"500", p.neutral_500}, {"600", p.neutral_600}, {"700", p.neutral_700},
                     {"800", p.neutral_800}, {"850", p.neutral_850}, {"900", p.neutral_900},
                     {"950", p.neutral_950}, {"1000", p.neutral_1000}}},
        {"Red",     {{"500", p.red_500}, {"600", p.red_600}}},
        {"Orange",  {{"500", p.orange_500}, {"600", p.orange_600}}},
        {"Amber",   {{"500", p.amber_500}}},
        {"Yellow",  {{"500", p.yellow_500}}},
        {"Blue",    {{"500", p.blue_500}, {"600", p.blue_600}}},
        {"Violet",  {{"500", p.violet_500}}},
        {"Pink",    {{"500", p.pink_500}}},
    };
}

std::string to_hex(const jtui::Color& c) {
    const int r = static_cast<int>(c.r * 255.0F + 0.5F);
    const int g = static_cast<int>(c.g * 255.0F + 0.5F);
    const int b = static_cast<int>(c.b * 255.0F + 0.5F);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return std::string{buf};
}

// 坐标整数对齐。GDI+ AA + PixelOffsetModeHalf 在非整数边界上会做亚像素羽化，
// 小色块的边缘看起来就"发虚"。Swatch 固定用整数边界就能拿到锐利矩形。
inline float snap(float v) {
    return static_cast<float>(static_cast<int>(v >= 0.0F ? v + 0.5F : v - 0.5F));
}

}  // namespace

struct PaletteSection::Impl {
    jtui::Text* title {nullptr};
    jtui::RealtimeCanvas* canvas {nullptr};
    // 缓存：palette 阶梯在整个进程生命周期里不变，构造一次即可。
    std::vector<Row> rows;
};

PaletteSection::PaletteSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);
    impl_->rows = build_palette_rows_once();

    auto title = std::make_unique<jtui::Text>();
    title->set_preset(jtui::TextStylePreset::Heading);
    impl_->title = title.get();
    root_->append_child(std::move(title));

    auto canvas = std::make_unique<jtui::RealtimeCanvas>();
    impl_->canvas = canvas.get();
    jtui::RealtimeCanvas* cv_ptr = impl_->canvas;
    Impl* impl_raw = impl_.get();
    canvas->set_draw_callback([cv_ptr, impl_raw](jtui::PaintContext& ctx) {
        const auto& c = jtui::theme::colors();
        const jtui::RectF area = cv_ptr->frame();

        // 色块尺寸：D2D FillRectangle 走 ALIASED 模式后边缘像素精确，不会糊。
        // 紧凑布局确保 9 行（含 Neutral 的 2 sub-row）都能装在 canvas 里：
        //   sub_row_total = swatch_h + label_gap + label_h = 36 + 2 + 14 = 52
        //   单行 Y 推进 = sub_row_total + row_outer_gap = 58
        //   Neutral 双 sub-row Y 推进 = 52 + 4 + 52 + 6 = 114
        //   8 单行 + 1 双 sub-row = 8*58 + 114 = 578 ≤ canvas height
        constexpr float swatch_w = 64.0F;
        constexpr float swatch_h = 36.0F;
        constexpr float swatch_gap = 8.0F;
        constexpr float label_gap = 2.0F;
        constexpr float label_h = 14.0F;
        constexpr float sub_row_total = swatch_h + label_gap + label_h;  // = 52
        constexpr float sub_row_inner_gap = 4.0F;
        constexpr float row_outer_gap = 6.0F;
        constexpr float name_col_w = 88.0F;

        // 右列 Semantic 占的宽度（含间距）。用它倒推左列色块可用宽度。
        constexpr float semantic_col_w = 280.0F;
        constexpr float column_gap = 24.0F;
        const float right_x = snap(area.x + area.width - semantic_col_w);
        const float left_max_x = right_x - column_gap;  // 左列色块最右边界

        // 单条 row 一行能挤几个色块。Neutral 14 个在 1300px 视区下会越过 Semantic
        // 列，需要换行；其它色阶（≤11）一行就够。
        const float row_avail_w = left_max_x - (area.x + name_col_w);
        const int max_per_subrow = std::max(
            1, static_cast<int>((row_avail_w + swatch_gap) / (swatch_w + swatch_gap)));

        float y = area.y;
        for (const Row& row : impl_raw->rows) {
            // row 名字只在第一条 sub-row 写
            ctx.draw_text(
                jtui::RectF{area.x, y + 8.0F, name_col_w, 20.0F},
                row.name, c.text_secondary, jtui::TextAlignment::Leading, 12.0F, true);

            int painted = 0;
            while (painted < static_cast<int>(row.swatches.size())) {
                const int batch =
                    std::min(max_per_subrow,
                             static_cast<int>(row.swatches.size()) - painted);
                float x = area.x + name_col_w;
                for (int i = 0; i < batch; ++i) {
                    const Swatch& sw = row.swatches[painted + i];
                    const jtui::RectF rect{snap(x), snap(y), swatch_w, swatch_h};
                    ctx.fill_rect(rect, sw.color);
                    ctx.draw_text(
                        jtui::RectF{snap(x), snap(y) + swatch_h + label_gap, swatch_w, label_h},
                        sw.label, c.text_muted, jtui::TextAlignment::Center, 11.0F, true);
                    x += swatch_w + swatch_gap;
                }
                painted += batch;
                if (painted < static_cast<int>(row.swatches.size())) {
                    // 同一行内换下一段 sub-row：紧凑间距
                    y += sub_row_total + sub_row_inner_gap;
                }
            }
            // 切到下一个色阶名（不同色彩组之间留稍大的间距）
            y += sub_row_total + row_outer_gap;
            if (y > area.y + area.height) break;
        }

        // 右列：SemanticColor 映射。这些值会随主题变化，所以每帧查，但查一次就够，
        // 不做任何 vector 分配（用 std::pair 数组直接写死）。
        float ry = area.y;

        const std::pair<const char*, jtui::Color> semantic_rows[] = {
            {"accent", c.accent},
            {"accent_hover", c.accent_hover},
            {"accent_pressed", c.accent_pressed},
            {"accent_soft", c.accent_soft},
            {"success", c.success},
            {"warning", c.warning},
            {"danger", c.danger},
            {"info", c.info},
            {"bg_surface", c.bg_surface},
            {"bg_raised", c.bg_raised},
            {"text_primary", c.text_primary},
            {"text_secondary", c.text_secondary},
            {"border", c.border},
            {"track", c.track},
        };
        ctx.draw_text(
            jtui::RectF{right_x, ry, 280.0F, 18.0F},
            "Semantic", c.text_secondary, jtui::TextAlignment::Leading, 12.0F, true);
        ry += 24.0F;
        for (const auto& sr : semantic_rows) {
            const jtui::RectF sw{right_x, snap(ry), 28.0F, 22.0F};
            ctx.fill_rect(sw, sr.second);
            ctx.draw_text(
                jtui::RectF{right_x + 36.0F, ry + 2.0F, 140.0F, 18.0F},
                sr.first, c.text_primary, jtui::TextAlignment::Leading, 11.0F);
            ctx.draw_text(
                jtui::RectF{right_x + 180.0F, ry + 2.0F, 100.0F, 18.0F},
                to_hex(sr.second), c.text_muted, jtui::TextAlignment::Leading, 11.0F);
            ry += 26.0F;
        }
    });
    root_->append_child(std::move(canvas));
}

PaletteSection::~PaletteSection() = default;

std::unique_ptr<jtui::Panel> PaletteSection::take_root() {
    return std::move(root_);
}

void PaletteSection::apply_layout(const SectionLayout& area) {
    // Canvas 整体上移 12px：标题 + canvas 之间留 28px（之前 40 太宽）。
    // 给底部色块（Pink）留出一点安全余量，不再越界。
    impl_->title->set_frame(jtui::RectF{snap(area.x), snap(area.y), area.width, 24.0F});
    impl_->canvas->set_frame(
        jtui::RectF{snap(area.x), snap(area.y + 28.0F), area.width, area.height - 28.0F});
}

void PaletteSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;
    impl_->title->set_content(zh ? "配色方案" : "Color Palette");
}

}  // namespace jtui::gallery
