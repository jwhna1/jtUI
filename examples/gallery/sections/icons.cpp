// Package: jtUI gallery
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// Icons section -- 展示几十个 @vscode/codicons 图标 (CodiconIcon widget).
//
// 布局: 按职能分组 5 组, 每组若干 icon, 每个 icon 一个 cell:
//   ┌── cell (96 x 88) ──┐
//   │       ┌──────┐     │
//   │       │ icon │     │  32x32 codicon glyph
//   │       └──────┘     │
//   │     name 12px      │  下方文字: 图标名 (codicon class 去前缀)
//   └────────────────────┘
//
// 与 vscode_palette 不同, 这里走真实 widget 树 (而非 RealtimeCanvas paint),
// 演示 CodiconIcon widget 在产品代码中的真实用法。每个图标是独立 widget,
// 可以单独 set_color / hover / focus, 业务侧不需要管 paint 细节。
//
// 主题切换: CodiconIcon 默认 fg 是浅灰, 这里通过 Theme::on_changed 把
// 所有 icon 的颜色刷成 theme.text_primary, 让 dark/light 视觉对比清晰。
#include "sections/icons.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

namespace {

struct IconGroup {
    const char* en_label;
    const char* zh_label;
    std::vector<const char*> names;
};

// 5 组共 56 个图标。所有名字都在 codicon_codepoints.hpp 表内, 来自 @vscode/codicons
// 0.0.45 官方 CSS。挑选原则: 覆盖 IDE / 文件 / git / 编辑 / 状态 5 大类常用图标,
// 让开源访客一眼看完就知道 jtUI 完整接通了 codicon 字体库。
const std::array<IconGroup, 5>& icon_groups() {
    static const std::array<IconGroup, 5> kGroups = {{
        {"Files & Folders", "文件与目录", {
            "file", "file-text", "file-code", "file-binary", "file-media", "file-pdf",
            "folder", "folder-opened", "folder-active", "new-file", "new-folder",
            "archive", "package", "library",
        }},
        {"Source Control", "源码与协作", {
            "git-branch", "git-commit", "git-merge", "git-pull-request",
            "source-control", "cloud", "cloud-upload", "cloud-download",
            "github", "github-alt", "fork", "history",
        }},
        {"Edit & Action", "编辑与操作", {
            "edit", "save", "save-all", "search", "settings-gear", "filter",
            "play", "stop-circle", "debug-start", "debug-stop", "refresh", "sync",
        }},
        {"UI & Navigation", "界面与导航", {
            "menu", "close", "arrow-left", "arrow-right", "chevron-down", "chevron-up",
            "ellipsis", "kebab-vertical", "screen-full", "split-horizontal", "preview",
        }},
        {"Status & Hints", "状态与提示", {
            "check", "x", "warning", "error", "info", "question",
            "star-full", "star-empty", "heart", "bell", "verified", "shield",
        }},
    }};
    return kGroups;
}

}  // namespace

struct IconsSection::Impl {
    jtui::Text* title{nullptr};
    jtui::Text* subtitle{nullptr};

    // 每组的 group header label
    std::vector<jtui::Text*> group_labels;

    // 所有 icon widget 指针, 主题切换时一次刷新颜色
    std::vector<jtui::CodiconIcon*> icons;

    // 每个 icon 下方的名字 label
    std::vector<jtui::Text*> name_labels;

    // section root frame, apply_layout 时缓存让 set_visible 后能再次 layout
    jtui::RectF area{};
};

IconsSection::IconsSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);

    auto title = std::make_unique<jtui::Text>();
    title->set_preset(jtui::TextStylePreset::Heading);
    impl_->title = title.get();
    root_->append_child(std::move(title));

    auto subtitle = std::make_unique<jtui::Text>();
    subtitle->set_preset(jtui::TextStylePreset::Caption);
    subtitle->set_role(jtui::TextRole::Secondary);
    impl_->subtitle = subtitle.get();
    root_->append_child(std::move(subtitle));

    // 创建所有 group label + icon + name label widget 树
    for (const auto& group : icon_groups()) {
        auto group_label = std::make_unique<jtui::Text>();
        group_label->set_preset(jtui::TextStylePreset::Label);
        group_label->set_role(jtui::TextRole::Muted);
        impl_->group_labels.push_back(group_label.get());
        root_->append_child(std::move(group_label));

        for (const char* name : group.names) {
            auto icon = std::make_unique<jtui::CodiconIcon>(std::string{name});
            icon->set_size_px(32.0F);
            // 初始颜色: theme text_primary. 之后随主题切换会被刷新。
            icon->set_color(jtui::theme::colors().text_primary);
            impl_->icons.push_back(icon.get());
            root_->append_child(std::move(icon));

            auto name_label = std::make_unique<jtui::Text>();
            name_label->set_preset(jtui::TextStylePreset::Caption);
            name_label->set_role(jtui::TextRole::Muted);
            name_label->set_content(std::string{name});
            name_label->set_alignment(jtui::TextAlignment::Center);
            impl_->name_labels.push_back(name_label.get());
            root_->append_child(std::move(name_label));
        }
    }

    // 主题切换时把全部 icon 重刷成新主题的 text_primary。
    // 不需要 disconnect (gallery 生命周期 = app 生命周期, lambda 持有 impl raw 指针安全)。
    Impl* impl_raw = impl_.get();
    jtui::theme::Theme::on_changed().connect([impl_raw](jtui::theme::ThemeMode) {
        const auto& c = jtui::theme::colors();
        for (jtui::CodiconIcon* icon : impl_raw->icons) {
            icon->set_color(c.text_primary);
        }
    });
}

IconsSection::~IconsSection() = default;

std::unique_ptr<jtui::Panel> IconsSection::take_root() {
    return std::move(root_);
}

void IconsSection::apply_layout(const SectionLayout& area) {
    impl_->area = jtui::RectF{area.x, area.y, area.width, area.height};

    // 顶部: title (28) + subtitle (18) + gap
    impl_->title->set_frame(jtui::RectF{area.x, area.y, area.width, 24.0F});
    impl_->subtitle->set_frame(
        jtui::RectF{area.x, area.y + 28.0F, area.width, 18.0F});

    constexpr float content_top_offset = 60.0F;
    constexpr float group_header_h = 22.0F;
    constexpr float group_gap_top = 14.0F;
    constexpr float group_gap_bottom = 8.0F;
    constexpr float cell_w = 96.0F;
    constexpr float cell_h = 78.0F;
    constexpr float icon_size = 32.0F;
    constexpr float icon_y_offset = 8.0F;
    constexpr float name_y_offset = icon_y_offset + icon_size + 6.0F;
    constexpr float name_h = 16.0F;

    const int per_row = std::max(1, static_cast<int>(area.width / cell_w));

    float y = area.y + content_top_offset;
    std::size_t icon_idx = 0;
    std::size_t group_idx = 0;
    for (const auto& group : icon_groups()) {
        // group header
        impl_->group_labels[group_idx]->set_frame(
            jtui::RectF{area.x, y, area.width, group_header_h});
        y += group_header_h + group_gap_top;

        // icons 排版: 每行 per_row 个
        const int total = static_cast<int>(group.names.size());
        int painted = 0;
        while (painted < total) {
            const int batch = std::min(per_row, total - painted);
            for (int i = 0; i < batch; ++i) {
                const float x = area.x + static_cast<float>(i) * cell_w;
                // 居中 icon: cell 宽 96, icon 32 -> 左偏移 (96-32)/2 = 32
                impl_->icons[icon_idx]->set_frame(
                    jtui::RectF{x + (cell_w - icon_size) / 2.0F,
                                y + icon_y_offset,
                                icon_size, icon_size});
                // name label 占满 cell 宽
                impl_->name_labels[icon_idx]->set_frame(
                    jtui::RectF{x, y + name_y_offset, cell_w, name_h});
                ++icon_idx;
            }
            painted += batch;
            y += cell_h;
        }
        y += group_gap_bottom;
        ++group_idx;
    }
}

void IconsSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;
    impl_->title->set_content(zh ? "图标库 Codicons" : "Codicons");
    impl_->subtitle->set_content(zh
        ? "@vscode/codicons 字体内嵌, IDWriteFactory5 自定义集合加载, 共 649 个图标 (542 unique)"
        : "Embedded @vscode/codicons via IDWriteFactory5 custom collection, 649 names (542 unique).");

    const auto& groups = icon_groups();
    for (std::size_t i = 0; i < groups.size(); ++i) {
        impl_->group_labels[i]->set_content(
            zh ? std::string{groups[i].zh_label} : std::string{groups[i].en_label});
    }
}

}  // namespace jtui::gallery
