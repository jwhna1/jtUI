#include "sections/inputs.hpp"

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

struct InputsSection::Impl {
    jtui::Text* col_title_left {nullptr};
    jtui::Text* col_title_right {nullptr};

    // 左列：TextInput 多种状态
    jtui::TextInput* input_basic {nullptr};
    jtui::TextInput* input_iconed {nullptr};
    jtui::TextInput* input_password {nullptr};
    jtui::TextInput* input_error {nullptr};
    jtui::TextInput* input_success {nullptr};

    // 右列：Checkbox tri-state + Switch + Slider + Chip group
    jtui::Checkbox* cb_checked {nullptr};
    jtui::Checkbox* cb_unchecked {nullptr};
    jtui::Checkbox* cb_indeterminate {nullptr};
    jtui::Checkbox* cb_disabled {nullptr};
    jtui::Switch* sw_on {nullptr};
    jtui::Switch* sw_off {nullptr};
    jtui::Switch* sw_disabled {nullptr};
    jtui::Slider* slider {nullptr};
    std::vector<jtui::Chip*> chips;
};

InputsSection::InputsSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);

    auto make_title = [this]() {
        auto t = std::make_unique<jtui::Text>();
        t->set_preset(jtui::TextStylePreset::Heading);
        jtui::Text* ptr = t.get();
        root_->append_child(std::move(t));
        return ptr;
    };
    impl_->col_title_left = make_title();
    impl_->col_title_right = make_title();

    auto make_input = [this](jtui::TextInput*& slot) {
        auto inp = std::make_unique<jtui::TextInput>();
        slot = inp.get();
        root_->append_child(std::move(inp));
    };

    make_input(impl_->input_basic);
    make_input(impl_->input_iconed);
    make_input(impl_->input_password);
    make_input(impl_->input_error);
    make_input(impl_->input_success);

    impl_->input_iconed->set_leading("\xf0\x9f\x94\x8d");  // 🔍
    impl_->input_password->set_password(true);
    impl_->input_error->set_state(jtui::TextInputState::Error);
    impl_->input_success->set_state(jtui::TextInputState::Success);
    impl_->input_success->set_trailing("\xe2\x9c\x93");  // ✓

    auto make_checkbox = [this](jtui::Checkbox*& slot) {
        auto cb = std::make_unique<jtui::Checkbox>();
        slot = cb.get();
        root_->append_child(std::move(cb));
    };
    make_checkbox(impl_->cb_checked);
    make_checkbox(impl_->cb_unchecked);
    make_checkbox(impl_->cb_indeterminate);
    make_checkbox(impl_->cb_disabled);
    impl_->cb_checked->set_checked(true);
    impl_->cb_indeterminate->set_state(jtui::CheckState::Indeterminate);
    impl_->cb_disabled->set_enabled(false);

    auto make_switch = [this](jtui::Switch*& slot) {
        auto s = std::make_unique<jtui::Switch>();
        slot = s.get();
        root_->append_child(std::move(s));
    };
    make_switch(impl_->sw_on);
    make_switch(impl_->sw_off);
    make_switch(impl_->sw_disabled);
    impl_->sw_on->set_checked(true);
    impl_->sw_disabled->set_enabled(false);
    impl_->sw_disabled->set_checked(true);

    auto slider = std::make_unique<jtui::Slider>();
    slider->set_value(0.65F);
    impl_->slider = slider.get();
    root_->append_child(std::move(slider));

    constexpr std::array chip_tones = {
        jtui::theme::Tone::Accent, jtui::theme::Tone::Success,
        jtui::theme::Tone::Warning, jtui::theme::Tone::Danger,
        jtui::theme::Tone::Info};
    for (auto t : chip_tones) {
        auto chip = std::make_unique<jtui::Chip>();
        chip->set_tone(t);
        chip->set_selected(t == jtui::theme::Tone::Accent);
        impl_->chips.push_back(chip.get());
        root_->append_child(std::move(chip));
    }
}

InputsSection::~InputsSection() = default;

std::unique_ptr<jtui::Panel> InputsSection::take_root() {
    return std::move(root_);
}

void InputsSection::apply_layout(const SectionLayout& area) {
    const float col_w = (area.width - 40.0F) * 0.5F;
    const float col_gap = 40.0F;
    const float left_x = area.x;
    const float right_x = area.x + col_w + col_gap;

    impl_->col_title_left->set_frame(jtui::RectF{left_x, area.y, col_w, 28.0F});
    impl_->col_title_right->set_frame(jtui::RectF{right_x, area.y, col_w, 28.0F});

    // 左列：5 个 input，每个 74 高（label + field + helper）
    const float inp_h = 74.0F;
    const float inp_gap = 12.0F;
    float y = area.y + 48.0F;
    auto layout_input = [&](jtui::TextInput* inp) {
        inp->set_frame(jtui::RectF{left_x, y, col_w, inp_h});
        y += inp_h + inp_gap;
    };
    layout_input(impl_->input_basic);
    layout_input(impl_->input_iconed);
    layout_input(impl_->input_password);
    layout_input(impl_->input_error);
    layout_input(impl_->input_success);

    // 右列：4 checkbox（一行一个，32 高）+ 3 switch + 1 slider + chips row
    y = area.y + 48.0F;
    const float cb_h = 32.0F;
    for (jtui::Checkbox* cb : {impl_->cb_checked, impl_->cb_unchecked,
                               impl_->cb_indeterminate, impl_->cb_disabled}) {
        cb->set_frame(jtui::RectF{right_x, y, col_w, cb_h});
        y += cb_h + 8.0F;
    }

    y += 8.0F;
    const float sw_h = 34.0F;
    for (jtui::Switch* sw : {impl_->sw_on, impl_->sw_off, impl_->sw_disabled}) {
        sw->set_frame(jtui::RectF{right_x, y, col_w, sw_h});
        y += sw_h + 10.0F;
    }

    y += 14.0F;
    impl_->slider->set_frame(jtui::RectF{right_x, y, col_w, 24.0F});
    y += 40.0F;

    // Chip 行
    float cx = right_x;
    for (jtui::Chip* chip : impl_->chips) {
        chip->set_frame(jtui::RectF{cx, y, 80.0F, 28.0F});
        cx += 88.0F;
    }
}

void InputsSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;

    impl_->col_title_left->set_content(zh ? "文本输入" : "Text Fields");
    impl_->col_title_right->set_content(zh ? "选择与调节" : "Selection & Adjust");

    impl_->input_basic->set_label(zh ? "项目名" : "Project name");
    impl_->input_basic->set_placeholder(zh ? "在此输入..." : "Type here...");
    impl_->input_basic->set_text(zh ? "jtUI Demo" : "jtUI Demo");

    impl_->input_iconed->set_label(zh ? "搜索" : "Search");
    impl_->input_iconed->set_placeholder(zh ? "查找模块..." : "Find modules...");

    impl_->input_password->set_label(zh ? "密码" : "Password");
    impl_->input_password->set_text(zh ? "password" : "password");
    impl_->input_password->set_helper_text(zh ? "至少 8 位" : "At least 8 chars");

    impl_->input_error->set_label(zh ? "电子邮件" : "Email");
    impl_->input_error->set_text("bad-email");
    impl_->input_error->set_helper_text(zh ? "邮箱格式无效" : "Invalid email format");

    impl_->input_success->set_label(zh ? "API Key" : "API Key");
    impl_->input_success->set_text("sk_live_*********");
    impl_->input_success->set_helper_text(zh ? "已连接" : "Connected");

    impl_->cb_checked->set_label(zh ? "已启用实时预览" : "Enable realtime preview");
    impl_->cb_unchecked->set_label(zh ? "自动保存" : "Auto save");
    impl_->cb_indeterminate->set_label(zh ? "部分字段选中" : "Some fields selected");
    impl_->cb_disabled->set_label(zh ? "硬件加速（不可用）" : "Hardware acceleration (unavailable)");

    impl_->sw_on->set_label(zh ? "深色模式" : "Dark mode");
    impl_->sw_off->set_label(zh ? "降噪" : "Noise reduction");
    impl_->sw_disabled->set_label(zh ? "试验模式（锁定）" : "Experimental (locked)");

    const std::array<std::pair<const char*, const char*>, 5> chip_labels = {{
        {"All", "全部"}, {"Done", "完成"}, {"Warning", "告警"},
        {"Error", "错误"}, {"Info", "信息"}}};
    for (std::size_t i = 0; i < impl_->chips.size(); ++i) {
        impl_->chips[i]->set_text(zh ? chip_labels[i].second : chip_labels[i].first);
    }
}

}  // namespace jtui::gallery
