#include "sections/buttons.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

namespace {

struct Row {
    jtui::Text* title;
    std::vector<jtui::Button*> buttons;  // 非拥有指针，root 持有 unique_ptr
};

}  // namespace

struct ButtonsSection::Impl {
    std::vector<Row> rows;
    // 给 apply_labels 用的 row title 对照表
    std::vector<std::pair<std::string, std::string>> row_titles;
    // 第一行实际按钮文字（variant 行需要切"填充"/"描边"/"幽灵"/"文本"）
    std::vector<std::pair<std::string, std::string>> variant_labels;
    // 尺寸行
    std::vector<std::pair<std::string, std::string>> size_labels;
    // 状态行（启用/禁用）
    std::vector<std::pair<std::string, std::string>> tone_labels;
};

ButtonsSection::ButtonsSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);

    impl_->row_titles = {
        {"Variants", "变体"},
        {"Sizes", "尺寸"},
        {"Tones", "色调"},
        {"States", "状态"},
    };
    impl_->variant_labels = {
        {"Filled", "填充"},
        {"Outlined", "描边"},
        {"Ghost", "幽灵"},
        {"Text", "文本"},
    };
    impl_->size_labels = {
        {"Small", "小"},
        {"Medium", "中"},
        {"Large", "大"},
    };
    impl_->tone_labels = {
        {"Accent", "主色"},
        {"Success", "成功"},
        {"Warning", "警告"},
        {"Danger", "危险"},
        {"Info", "信息"},
    };

    // 构建行。layout 在 apply_layout 里再定位，这里只负责 append 和基础属性。
    auto add_title = [this]() {
        auto title = std::make_unique<jtui::Text>();
        title->set_preset(jtui::TextStylePreset::Label);
        title->set_role(jtui::TextRole::Secondary);
        jtui::Text* ptr = title.get();
        root_->append_child(std::move(title));
        return ptr;
    };

    // Row 1: Variants（4 个按钮）
    {
        Row r{};
        r.title = add_title();
        constexpr std::array variants = {jtui::ButtonVariant::Filled, jtui::ButtonVariant::Outlined,
                                         jtui::ButtonVariant::Ghost, jtui::ButtonVariant::Text};
        for (auto v : variants) {
            auto btn = std::make_unique<jtui::Button>();
            btn->set_variant(v);
            btn->set_size(jtui::ButtonSize::Medium);
            r.buttons.push_back(btn.get());
            root_->append_child(std::move(btn));
        }
        impl_->rows.push_back(std::move(r));
    }

    // Row 2: Sizes（3 个按钮，filled）
    {
        Row r{};
        r.title = add_title();
        constexpr std::array sizes = {jtui::ButtonSize::Small, jtui::ButtonSize::Medium,
                                      jtui::ButtonSize::Large};
        for (auto s : sizes) {
            auto btn = std::make_unique<jtui::Button>();
            btn->set_size(s);
            r.buttons.push_back(btn.get());
            root_->append_child(std::move(btn));
        }
        impl_->rows.push_back(std::move(r));
    }

    // Row 3: Tones（5 个 filled 按钮）
    {
        Row r{};
        r.title = add_title();
        constexpr std::array tones = {jtui::theme::Tone::Accent, jtui::theme::Tone::Success,
                                      jtui::theme::Tone::Warning, jtui::theme::Tone::Danger,
                                      jtui::theme::Tone::Info};
        for (auto t : tones) {
            auto btn = std::make_unique<jtui::Button>();
            btn->set_tone(t);
            r.buttons.push_back(btn.get());
            root_->append_child(std::move(btn));
        }
        impl_->rows.push_back(std::move(r));
    }

    // Row 4: States (enabled + disabled + with icon)
    {
        Row r{};
        r.title = add_title();
        for (int i = 0; i < 4; ++i) {
            auto btn = std::make_unique<jtui::Button>();
            r.buttons.push_back(btn.get());
            root_->append_child(std::move(btn));
        }
        impl_->rows.push_back(std::move(r));
    }

    // Row 4 特殊配置：禁用 + 带图标
    auto& state_row = impl_->rows[3];
    state_row.buttons[0]->set_variant(jtui::ButtonVariant::Filled);
    state_row.buttons[1]->set_variant(jtui::ButtonVariant::Filled);
    state_row.buttons[1]->set_enabled(false);
    state_row.buttons[2]->set_variant(jtui::ButtonVariant::Filled);
    state_row.buttons[2]->set_leading_icon("\xe2\x96\xb6");  // ▶
    state_row.buttons[3]->set_variant(jtui::ButtonVariant::Outlined);
    state_row.buttons[3]->set_trailing_icon("\xe2\x86\x92"); // →
}

ButtonsSection::~ButtonsSection() = default;

std::unique_ptr<jtui::Panel> ButtonsSection::take_root() {
    return std::move(root_);
}

void ButtonsSection::apply_layout(const SectionLayout& area) {
    // 四行，每行：title 左侧 120px，右侧排按钮。行高 64，行间距 16。
    const float row_h = 52.0F;
    const float row_gap = 20.0F;
    const float title_w = 100.0F;
    const float btn_gap = 12.0F;
    const float btn_h = 40.0F;

    // 行高参数：button 实际高度按 size 调整。
    auto button_height = [](jtui::ButtonSize s) {
        switch (s) {
        case jtui::ButtonSize::Small:  return 28.0F;
        case jtui::ButtonSize::Medium: return 36.0F;
        case jtui::ButtonSize::Large:  return 44.0F;
        }
        return 36.0F;
    };

    for (std::size_t i = 0; i < impl_->rows.size(); ++i) {
        const Row& r = impl_->rows[i];
        const float row_y = area.y + 8.0F + i * (row_h + row_gap);

        r.title->set_frame(jtui::RectF{area.x, row_y + 8.0F, title_w, 20.0F});

        float x = area.x + title_w + 16.0F;
        for (jtui::Button* btn : r.buttons) {
            // 估宽度：看按钮类型
            float w = 120.0F;
            if (i == 1) {  // sizes 行
                w = 120.0F;
            }
            const float h = (i == 1) ? button_height(
                [&]() {
                    const std::size_t idx = static_cast<std::size_t>(
                        btn - r.buttons.front());
                    (void)idx;
                    return jtui::ButtonSize::Medium;
                }()) : btn_h;
            btn->set_frame(jtui::RectF{x, row_y + (row_h - h) * 0.5F, w, h});
            x += w + btn_gap;
        }
    }
}

void ButtonsSection::apply_labels(Locale locale) {
    const auto zh = locale == Locale::Chinese;
    for (std::size_t i = 0; i < impl_->rows.size(); ++i) {
        impl_->rows[i].title->set_content(
            zh ? impl_->row_titles[i].second : impl_->row_titles[i].first);
    }

    // Row 0: Variants
    for (std::size_t i = 0; i < impl_->rows[0].buttons.size(); ++i) {
        impl_->rows[0].buttons[i]->set_text(
            zh ? impl_->variant_labels[i].second : impl_->variant_labels[i].first);
    }
    // Row 1: Sizes
    for (std::size_t i = 0; i < impl_->rows[1].buttons.size(); ++i) {
        impl_->rows[1].buttons[i]->set_text(
            zh ? impl_->size_labels[i].second : impl_->size_labels[i].first);
    }
    // Row 2: Tones
    for (std::size_t i = 0; i < impl_->rows[2].buttons.size(); ++i) {
        impl_->rows[2].buttons[i]->set_text(
            zh ? impl_->tone_labels[i].second : impl_->tone_labels[i].first);
    }
    // Row 3: States（四列固定语义）
    auto& state_row = impl_->rows[3];
    state_row.buttons[0]->set_text(zh ? "启用" : "Enabled");
    state_row.buttons[1]->set_text(zh ? "禁用" : "Disabled");
    state_row.buttons[2]->set_text(zh ? "播放" : "Play");
    state_row.buttons[3]->set_text(zh ? "下一步" : "Next");
}

}  // namespace jtui::gallery
