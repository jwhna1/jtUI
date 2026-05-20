// Package: jtUI gallery
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// VSCode Palette section -- 展示 vscode_tokens 全套色阶 + dark/light 语义色映射。
//
// 布局 (1360x620 内容区内):
//   ┌────────────────────────────────────────────────────────────────────┐
//   │ Base (21)                                                          │
//   │ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓                                              │
//   │ Accents (35)                                                       │
//   │ Blue   ▓▓▓▓▓▓▓▓▓▓▓     Red ▓▓▓▓                                    │
//   │ Green  ▓▓▓▓▓        Purple ▓▓▓▓     ┌── VSCode Semantic ────────┐  │
//   │ Yellow ▓▓▓▓▓        Orange ▓▓▓      │ statusbar    ▓ #007ACC    │  │
//   │ Gray   ▓▓▓                          │ activitybar  ▓ #333333    │  │
//   │ Seti (11) (file type colors)        │ sidebar      ▓ #252526    │  │
//   │ ▓▓▓▓▓▓▓▓▓▓▓                          │ list-select  ▓ #094771    │  │
//   │                                     │ ...                       │  │
//   │                                     └───────────────────────────┘  │
//   └────────────────────────────────────────────────────────────────────┘
//
// 全部用 RealtimeCanvas + draw_callback 走 paint, 一次性出图, 主题切换会自动
// 触发重绘 (Theme::on_changed 信号挂在 root_ptr->invalidate)。
#include "sections/vscode_palette.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hui/theme/vscode_tokens.hpp"
#include "jtui/jtui.hpp"

namespace jtui::gallery {

namespace {

struct Swatch {
    const char* label;
    jtui::Color color;
};

struct Group {
    const char* name;
    std::vector<Swatch> swatches;
};

// Base 21 灰阶 + Accents 7 组 + Seti 11 色。Palette 是静态数据, 构造一次。
std::vector<Group> build_base_rows() {
    const auto& p = jtui::theme::vscode::palette();
    return {
        {"Base", {
            {"01", p.base.base_01}, {"02", p.base.base_02}, {"03", p.base.base_03},
            {"04", p.base.base_04}, {"05", p.base.base_05}, {"06", p.base.base_06},
            {"07", p.base.base_07}, {"08", p.base.base_08}, {"09", p.base.base_09},
            {"10", p.base.base_10}, {"11", p.base.base_11}, {"12", p.base.base_12},
            {"13", p.base.base_13}, {"14", p.base.base_14}, {"15", p.base.base_15},
            {"16", p.base.base_16}, {"17", p.base.base_17}, {"18", p.base.base_18},
            {"19", p.base.base_19}, {"20", p.base.base_20}, {"21", p.base.base_21},
        }},
    };
}

std::vector<Group> build_accent_rows() {
    const auto& p = jtui::theme::vscode::palette();
    return {
        {"Blue", {
            {"01", p.accent.blue.blue_01}, {"02", p.accent.blue.blue_02},
            {"03", p.accent.blue.blue_03}, {"04", p.accent.blue.blue_04},
            {"05", p.accent.blue.blue_05}, {"06", p.accent.blue.blue_06},
            {"07", p.accent.blue.blue_07}, {"08", p.accent.blue.blue_08},
            {"09", p.accent.blue.blue_09}, {"10", p.accent.blue.blue_10},
            {"11", p.accent.blue.blue_11},
        }},
        {"Red", {
            {"01", p.accent.red.red_01}, {"02", p.accent.red.red_02},
            {"03", p.accent.red.red_03}, {"04", p.accent.red.red_04},
        }},
        {"Green", {
            {"01", p.accent.green.green_01}, {"02", p.accent.green.green_02},
            {"03", p.accent.green.green_03}, {"04", p.accent.green.green_04},
            {"05", p.accent.green.green_05},
        }},
        {"Purple", {
            {"01", p.accent.purple.purple_01}, {"02", p.accent.purple.purple_02},
            {"03", p.accent.purple.purple_03}, {"04", p.accent.purple.purple_04},
        }},
        {"Yellow", {
            {"01", p.accent.yellow.yellow_01}, {"02", p.accent.yellow.yellow_02},
            {"03", p.accent.yellow.yellow_03}, {"04", p.accent.yellow.yellow_04},
            {"05", p.accent.yellow.yellow_05},
        }},
        {"Orange", {
            {"01", p.accent.orange.orange_01}, {"02", p.accent.orange.orange_02},
            {"03", p.accent.orange.orange_03},
        }},
        {"Gray", {
            {"01", p.accent.gray.gray_01}, {"02", p.accent.gray.gray_02},
            {"03", p.accent.gray.gray_03},
        }},
    };
}

std::vector<Group> build_seti_rows() {
    const auto& p = jtui::theme::vscode::palette();
    return {
        {"Seti", {
            {"Blue", p.seti.blue}, {"Green", p.seti.green}, {"Orange", p.seti.orange},
            {"Pink", p.seti.pink}, {"Red", p.seti.red}, {"Steel", p.seti.steel},
            {"Yellow", p.seti.yellow}, {"Purple", p.seti.purple},
            {"Ignore", p.seti.ignore}, {"White", p.seti.white}, {"Gray", p.seti.gray},
        }},
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

inline float snap(float v) {
    return static_cast<float>(static_cast<int>(v >= 0.0F ? v + 0.5F : v - 0.5F));
}

}  // namespace

struct VsCodePaletteSection::Impl {
    jtui::Text* title{nullptr};
    jtui::RealtimeCanvas* canvas{nullptr};
    std::vector<Group> base_rows;
    std::vector<Group> accent_rows;
    std::vector<Group> seti_rows;
};

VsCodePaletteSection::VsCodePaletteSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);
    impl_->base_rows = build_base_rows();
    impl_->accent_rows = build_accent_rows();
    impl_->seti_rows = build_seti_rows();

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
        const auto& vs = jtui::theme::vscode::semantic();
        const jtui::RectF area = cv_ptr->frame();

        constexpr float swatch_w = 52.0F;
        constexpr float swatch_h = 32.0F;
        constexpr float swatch_gap = 6.0F;
        constexpr float label_h = 12.0F;
        constexpr float label_gap = 2.0F;
        constexpr float group_label_w = 70.0F;
        constexpr float group_inner_gap = 4.0F;
        constexpr float section_header_h = 22.0F;
        constexpr float section_gap = 16.0F;

        // 右侧 VSCode Semantic 列宽
        constexpr float semantic_col_w = 320.0F;
        constexpr float column_gap = 28.0F;
        const float right_x = snap(area.x + area.width - semantic_col_w);
        const float left_max_x = right_x - column_gap;
        const float row_avail_w = left_max_x - (area.x + group_label_w);
        const int max_per_subrow = static_cast<int>(
            (row_avail_w + swatch_gap) / (swatch_w + swatch_gap));

        // 通用画一个色块组的 helper. 返回画完后下一行的 y。
        auto paint_group_row = [&](const Group& g, float y) -> float {
            // 组名 (Base / Blue / Red ...)
            ctx.draw_text(
                jtui::RectF{area.x, y + 6.0F, group_label_w, 18.0F},
                g.name, c.text_secondary, jtui::TextAlignment::Leading, 12.0F, true);

            int painted = 0;
            const int total = static_cast<int>(g.swatches.size());
            while (painted < total) {
                const int batch = std::min(max_per_subrow, total - painted);
                float x = area.x + group_label_w;
                for (int i = 0; i < batch; ++i) {
                    const Swatch& sw = g.swatches[painted + i];
                    const jtui::RectF rect{snap(x), snap(y), swatch_w, swatch_h};
                    ctx.fill_rect(rect, sw.color);
                    ctx.draw_text(
                        jtui::RectF{snap(x), snap(y + swatch_h + label_gap),
                                    swatch_w, label_h},
                        sw.label, c.text_muted, jtui::TextAlignment::Center, 10.0F, false);
                    x += swatch_w + swatch_gap;
                }
                painted += batch;
                if (painted < total) {
                    y += swatch_h + label_gap + label_h + group_inner_gap;
                }
            }
            return y + swatch_h + label_gap + label_h + 8.0F;
        };

        // 通用画分区标题 helper.
        auto paint_section_header = [&](const char* en, float y) -> float {
            ctx.draw_text(
                jtui::RectF{area.x, y, 240.0F, section_header_h},
                en, c.text_primary, jtui::TextAlignment::Leading, 14.0F, true);
            return y + section_header_h + 4.0F;
        };

        // 左列: Base / Accents / Seti
        float y = area.y;
        y = paint_section_header("Base  21 grayscale", y);
        for (const auto& g : impl_raw->base_rows) {
            y = paint_group_row(g, y);
        }
        y += section_gap;
        y = paint_section_header("Accents  35 colors", y);
        for (const auto& g : impl_raw->accent_rows) {
            y = paint_group_row(g, y);
        }
        y += section_gap;
        y = paint_section_header("Seti  file type colors", y);
        for (const auto& g : impl_raw->seti_rows) {
            y = paint_group_row(g, y);
        }

        // 右列: VSCode Semantic (跟随 Theme::mode() 切换)
        float ry = area.y;
        ctx.draw_text(
            jtui::RectF{right_x, ry, semantic_col_w, section_header_h},
            "VSCode Semantic (Default Dark+/Light+)",
            c.text_primary, jtui::TextAlignment::Leading, 14.0F, true);
        ry += section_header_h + 4.0F;

        const std::pair<const char*, jtui::Color> semantic_rows[] = {
            {"statusbar.bg",          vs.statusbar_bg},
            {"statusbar.fg",          vs.statusbar_fg},
            {"statusbar.noFolderBg",  vs.statusbar_no_folder_bg},
            {"activityBar.bg",        vs.activitybar_bg},
            {"activityBar.fgActive",  vs.activitybar_fg_active},
            {"activityBar.badge.bg",  vs.activitybar_badge_bg},
            {"sideBar.bg",            vs.sidebar_bg},
            {"sideBar.fg",            vs.sidebar_fg},
            {"sideBar.sectionHeader", vs.sidebar_section_header_fg},
            {"list.hoverBg",          vs.list_hover_bg},
            {"list.activeSelBg",      vs.list_active_selection_bg},
            {"list.activeSelFg",      vs.list_active_selection_fg},
            {"list.inactiveSelBg",    vs.list_inactive_selection_bg},
            {"list.dropBg",           vs.list_drop_bg},
            {"notification.bg",       vs.notification_bg},
            {"notification.border",   vs.notification_border},
            {"notification.info",     vs.notification_info_icon},
            {"notification.warn",     vs.notification_warn_icon},
            {"notification.error",    vs.notification_error_icon},
        };

        constexpr float sw_box_w = 28.0F;
        constexpr float sw_box_h = 18.0F;
        constexpr float row_h = 22.0F;
        for (const auto& sr : semantic_rows) {
            const jtui::RectF sw{right_x, snap(ry), sw_box_w, sw_box_h};
            ctx.fill_rect(sw, sr.second);
            ctx.stroke_rect(sw, c.divider, 1.0F);
            ctx.draw_text(
                jtui::RectF{right_x + sw_box_w + 8.0F, ry, 180.0F, sw_box_h},
                sr.first, c.text_primary, jtui::TextAlignment::Leading, 11.0F, false);
            ctx.draw_text(
                jtui::RectF{right_x + sw_box_w + 8.0F + 180.0F, ry, 100.0F, sw_box_h},
                to_hex(sr.second), c.text_muted, jtui::TextAlignment::Leading, 10.0F, false);
            ry += row_h;
        }
    });
    root_->append_child(std::move(canvas));
}

VsCodePaletteSection::~VsCodePaletteSection() = default;

std::unique_ptr<jtui::Panel> VsCodePaletteSection::take_root() {
    return std::move(root_);
}

void VsCodePaletteSection::apply_layout(const SectionLayout& area) {
    impl_->title->set_frame(jtui::RectF{snap(area.x), snap(area.y), area.width, 24.0F});
    impl_->canvas->set_frame(
        jtui::RectF{snap(area.x), snap(area.y + 28.0F), area.width, area.height - 28.0F});
}

void VsCodePaletteSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;
    impl_->title->set_content(zh ? "VS Code 风格色卡" : "VS Code Palette");
}

}  // namespace jtui::gallery
