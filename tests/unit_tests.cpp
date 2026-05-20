#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "hui/app/application.hpp"
#include "hui/event/dispatcher.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/property.hpp"
#include "hui/property/realtime_source.hpp"
#include "hui/property/signal.hpp"
#include "hui/theme/theme.hpp"
#include "hui/widgets/common/chip.hpp"
#include "hui/widgets/common/progress_bar.hpp"
#include "hui/widgets/common/semi_gauge.hpp"
#include "hui/widgets/common/tooltip.hpp"
#include "hui/widgets/basic/flex_box.hpp"
#include "hui/widgets/basic/panel.hpp"
#include "hui/widgets/basic/scroll_view.hpp"
#include "hui/widgets/basic/text.hpp"
#include "hui/widgets/basic/title_bar.hpp"
#include "hui/widgets/common/button.hpp"
#include "hui/widgets/common/checkbox.hpp"
#include "hui/widgets/common/dialog.hpp"
#include "hui/widgets/common/switch.hpp"
#include "hui/widgets/common/tabs.hpp"
#include "hui/widgets/common/text_input.hpp"
#include "hui/widgets/common/dropdown.hpp"
#include "hui/widgets/common/popover.hpp"
#include "hui/widgets/common/slider.hpp"
#include "hui/widgets/common/toolbar.hpp"
#include "hui/widgets/media/level_meter.hpp"
#include "hui/widgets/media/realtime_canvas.hpp"
#include "hui/widgets/media/timeline.hpp"
#include "hui/widgets/media/video_player.hpp"
#include "hui/theme/token_override.hpp"

namespace {

int failures = 0;

void expect_true(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << file << ':' << line << ": expected true: " << expression << '\n';
}

template <typename Actual, typename Expected>
void expect_equal(const Actual& actual, const Expected& expected, const char* actual_expression,
                  const char* expected_expression, const char* file, int line) {
    if (actual == expected) {
        return;
    }

    ++failures;
    std::cerr << file << ':' << line << ": expected " << actual_expression
              << " == " << expected_expression << " but got " << actual << " and " << expected
              << '\n';
}

#define EXPECT_TRUE(expression) expect_true((expression), #expression, __FILE__, __LINE__)
#define EXPECT_EQ(actual, expected)                                                                \
    expect_equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)

void test_property_emits_on_change() {
    hui::Property<std::string> property{"old"};
    int changed_count = 0;
    std::string observed;

    property.changed().connect([&](const std::string& value) {
        ++changed_count;
        observed = value;
    });

    EXPECT_TRUE(!property.set("old"));
    EXPECT_EQ(changed_count, 0);
    EXPECT_TRUE(property.set("new"));
    EXPECT_EQ(changed_count, 1);
    EXPECT_EQ(observed, std::string{"new"});
}

void test_node_sets_parent_when_child_is_appended() {
    hui::Panel parent;
    auto child = std::make_unique<hui::Text>("child");
    hui::Node* child_ptr = child.get();

    hui::Node& appended = parent.append_child(std::move(child));

    EXPECT_TRUE(&appended == child_ptr);
    EXPECT_TRUE(child_ptr->parent() == &parent);
    EXPECT_EQ(parent.child_count(), static_cast<std::size_t>(1));
}

void test_button_click_uses_widget_event() {
    hui::Button button{"Run"};
    bool clicked = false;

    button.on_clicked().connect([&] { clicked = true; });
    button.on_click(hui::PointF{4.0F, 4.0F});

    EXPECT_TRUE(clicked);
}

void test_text_input_handles_focus_text_and_backspace() {
    hui::TextInput input;

    EXPECT_TRUE(input.accepts_focus());
    EXPECT_TRUE(input.on_text_input('a'));
    EXPECT_TRUE(input.on_text_input('b'));
    EXPECT_EQ(input.text(), std::string{"ab"});
    EXPECT_TRUE(input.on_key_down(8));
    EXPECT_EQ(input.text(), std::string{"a"});
    EXPECT_TRUE(!input.on_text_input('\n'));
}

void test_selection_widgets_handle_clicks() {
    hui::Checkbox checkbox;
    checkbox.on_click(hui::PointF{});
    EXPECT_TRUE(checkbox.checked());

    hui::Switch toggle;
    bool observed = false;
    toggle.on_changed().connect([&](bool checked) { observed = checked; });
    toggle.on_click(hui::PointF{});
    EXPECT_TRUE(toggle.checked());
    EXPECT_TRUE(observed);
}

void test_tabs_select_item_from_click_position() {
    hui::Tabs tabs;
    tabs.set_frame(hui::RectF{0.0F, 0.0F, 300.0F, 40.0F});
    tabs.set_items({"One", "Two", "Three"});

    tabs.on_mouse_move(hui::PointF{150.0F, 10.0F});
    tabs.on_click(hui::PointF{150.0F, 10.0F});

    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(1));
}

void test_dialog_confirm_click_closes_dialog() {
    hui::Dialog dialog;
    dialog.set_open(true);
    dialog.set_frame(hui::RectF{0.0F, 0.0F, 320.0F, 220.0F});

    const hui::RectF confirm = dialog.confirm_rect();
    dialog.on_mouse_move(hui::PointF{confirm.x + 4.0F, confirm.y + 4.0F});
    dialog.on_press_changed(hui::PointF{confirm.x + 4.0F, confirm.y + 4.0F}, true);
    dialog.on_click(hui::PointF{confirm.x + 4.0F, confirm.y + 4.0F});

    EXPECT_TRUE(!dialog.open());
}

void test_title_bar_exposes_window_actions_without_runtime_type_checks() {
    hui::TitleBar title_bar;
    title_bar.set_frame(hui::RectF{0.0F, 0.0F, 240.0F, 48.0F});

    EXPECT_TRUE(title_bar.window_action_at(hui::PointF{10.0F, 10.0F}) ==
                hui::WindowAction::DragMove);
    EXPECT_TRUE(title_bar.window_action_at(hui::PointF{112.0F, 10.0F}) ==
                hui::WindowAction::Minimize);
    EXPECT_TRUE(title_bar.window_action_at(hui::PointF{158.0F, 10.0F}) ==
                hui::WindowAction::ToggleMaximize);
    EXPECT_TRUE(title_bar.window_action_at(hui::PointF{204.0F, 10.0F}) == hui::WindowAction::Close);
}

void test_signal_connect_emit_disconnect() {
    hui::Signal<int> signal;
    int last_value = 0;
    int call_count = 0;

    const auto id = signal.connect([&](int value) {
        last_value = value;
        ++call_count;
    });

    signal.emit(42);
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(last_value, 42);

    signal.disconnect(id);
    signal.emit(100);
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(last_value, 42);
}

void test_signal_multiple_subscribers_receive_events() {
    hui::Signal<> signal;
    int a = 0;
    int b = 0;

    signal.connect([&] { ++a; });
    signal.connect([&] { ++b; });
    signal.emit();
    signal.emit();

    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 2);
}

void test_event_dispatcher_hit_test_returns_deepest_widget() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto child = std::make_unique<hui::Button>("inner");
    child->set_frame(hui::RectF{40.0F, 40.0F, 80.0F, 30.0F});
    hui::Button* child_ptr = child.get();
    root.append_child(std::move(child));

    hui::Widget* hit = hui::EventDispatcher::hit_test(root, hui::PointF{60.0F, 50.0F});
    EXPECT_TRUE(hit == child_ptr);

    hui::Widget* miss = hui::EventDispatcher::hit_test(root, hui::PointF{5.0F, 5.0F});
    EXPECT_TRUE(miss == &root);
}

void test_event_dispatcher_press_capture_sends_move_to_pressed() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto button = std::make_unique<hui::Button>("run");
    button->set_frame(hui::RectF{10.0F, 10.0F, 60.0F, 30.0F});
    hui::Button* button_ptr = button.get();
    root.append_child(std::move(button));

    hui::EventState state;
    hui::Widget* pressed =
        hui::EventDispatcher::dispatch_pointer_down(&root, hui::PointF{20.0F, 20.0F}, state);
    EXPECT_TRUE(pressed == button_ptr);
    EXPECT_TRUE(state.pressed == button_ptr);
    EXPECT_TRUE(button_ptr->pressed());

    // 移出 button 边界后仍应在 pressed 状态（Press capture）
    hui::EventDispatcher::dispatch_pointer_move(&root, hui::PointF{150.0F, 150.0F}, state);
    EXPECT_TRUE(state.pressed == button_ptr);

    // 回到 button 上松开，触发 activation
    hui::Widget* activation = nullptr;
    hui::EventDispatcher::dispatch_pointer_up(&root, hui::PointF{30.0F, 20.0F}, state, activation);
    EXPECT_TRUE(activation == button_ptr);
    EXPECT_TRUE(state.pressed == nullptr);
    EXPECT_TRUE(!button_ptr->pressed());
}

void test_event_dispatcher_pointer_up_outside_does_not_activate() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto button = std::make_unique<hui::Button>("run");
    button->set_frame(hui::RectF{10.0F, 10.0F, 60.0F, 30.0F});
    hui::Button* button_ptr = button.get();
    root.append_child(std::move(button));

    hui::EventState state;
    hui::EventDispatcher::dispatch_pointer_down(&root, hui::PointF{20.0F, 20.0F}, state);
    EXPECT_TRUE(state.pressed == button_ptr);

    hui::Widget* activation = nullptr;
    hui::EventDispatcher::dispatch_pointer_up(&root, hui::PointF{180.0F, 180.0F}, state,
                                              activation);
    EXPECT_TRUE(activation == nullptr);
    EXPECT_TRUE(state.pressed == nullptr);
}

void test_event_dispatcher_focus_update_fires_widget_focus_flag() {
    hui::TextInput input;
    hui::EventState state;

    EXPECT_TRUE(hui::EventDispatcher::set_focus(state, &input));
    EXPECT_TRUE(input.focused());

    EXPECT_TRUE(!hui::EventDispatcher::set_focus(state, &input));

    EXPECT_TRUE(hui::EventDispatcher::set_focus(state, nullptr));
    EXPECT_TRUE(!input.focused());
}

void test_flex_box_row_layout_with_pixel_items() {
    auto box = std::make_unique<hui::FlexBox>();
    box->set_direction(hui::LayoutDirection::Row);
    box->set_gap(10.0F);

    auto a = std::make_unique<hui::Button>("A");
    auto b = std::make_unique<hui::Button>("B");
    hui::Widget* a_ptr = a.get();
    hui::Widget* b_ptr = b.get();
    box->append_flex(std::move(a), hui::FlexItem{hui::Length::pixels(100.0F), true});
    box->append_flex(std::move(b), hui::FlexItem{hui::Length::pixels(60.0F), true});

    box->arrange(hui::RectF{0.0F, 0.0F, 300.0F, 40.0F});

    EXPECT_EQ(a_ptr->frame().x, 0.0F);
    EXPECT_EQ(a_ptr->frame().width, 100.0F);
    EXPECT_EQ(b_ptr->frame().x, 110.0F);
    EXPECT_EQ(b_ptr->frame().width, 60.0F);
    EXPECT_EQ(a_ptr->frame().height, 40.0F);
}

void test_flex_box_row_layout_fill_divides_remaining_by_weight() {
    auto box = std::make_unique<hui::FlexBox>();
    box->set_direction(hui::LayoutDirection::Row);
    box->set_gap(0.0F);

    auto a = std::make_unique<hui::Button>("A");
    auto b = std::make_unique<hui::Button>("B");
    auto c = std::make_unique<hui::Button>("C");
    hui::Widget* a_ptr = a.get();
    hui::Widget* b_ptr = b.get();
    hui::Widget* c_ptr = c.get();
    box->append_flex(std::move(a), hui::FlexItem{hui::Length::pixels(100.0F), true});
    box->append_flex(std::move(b), hui::FlexItem{hui::Length::fill(1.0F), true});
    box->append_flex(std::move(c), hui::FlexItem{hui::Length::fill(2.0F), true});

    box->arrange(hui::RectF{0.0F, 0.0F, 400.0F, 40.0F});

    EXPECT_EQ(a_ptr->frame().width, 100.0F);
    EXPECT_EQ(b_ptr->frame().x, 100.0F);
    EXPECT_EQ(b_ptr->frame().width, 100.0F);
    EXPECT_EQ(c_ptr->frame().x, 200.0F);
    EXPECT_EQ(c_ptr->frame().width, 200.0F);
}

void test_flex_box_column_layout_cross_stretch() {
    auto box = std::make_unique<hui::FlexBox>();
    box->set_direction(hui::LayoutDirection::Column);
    box->set_gap(5.0F);

    auto a = std::make_unique<hui::Button>("A");
    auto b = std::make_unique<hui::Button>("B");
    hui::Widget* a_ptr = a.get();
    hui::Widget* b_ptr = b.get();
    box->append_flex(std::move(a), hui::FlexItem{hui::Length::pixels(30.0F), true});
    box->append_flex(std::move(b), hui::FlexItem{hui::Length::pixels(40.0F), true});

    box->arrange(hui::RectF{0.0F, 0.0F, 200.0F, 100.0F});

    EXPECT_EQ(a_ptr->frame().y, 0.0F);
    EXPECT_EQ(a_ptr->frame().height, 30.0F);
    EXPECT_EQ(a_ptr->frame().width, 200.0F);
    EXPECT_EQ(b_ptr->frame().y, 35.0F);
    EXPECT_EQ(b_ptr->frame().height, 40.0F);
    EXPECT_EQ(b_ptr->frame().width, 200.0F);
}

void test_color_from_hex_parses_common_forms() {
    const hui::Color green = hui::Color::from_hex("#22C55E");
    EXPECT_TRUE(green.r > 0.13F && green.r < 0.14F);   // 34/255 ≈ 0.1333
    EXPECT_TRUE(green.g > 0.77F && green.g < 0.78F);   // 197/255 ≈ 0.7725
    EXPECT_TRUE(green.b > 0.36F && green.b < 0.38F);   // 94/255 ≈ 0.3686
    EXPECT_EQ(green.a, 1.0F);

    // 不带 # + 带 alpha
    const hui::Color semi = hui::Color::from_hex("00000080");
    EXPECT_EQ(semi.r, 0.0F);
    EXPECT_TRUE(semi.a > 0.49F && semi.a < 0.51F);  // 128/255

    // 小写也认
    const hui::Color same = hui::Color::from_hex("#22c55e");
    EXPECT_EQ(same.r, green.r);

    // 非法长度 → 透明黑
    const hui::Color bad = hui::Color::from_hex("#ABC");
    EXPECT_EQ(bad.a, 0.0F);
}

void test_theme_accent_is_green_500_in_both_modes() {
    using namespace hui::theme;

    const hui::Color green_500 = hui::Color::from_hex("#22C55E");

    Theme::set(ThemeMode::Dark);
    EXPECT_TRUE(Theme::mode() == ThemeMode::Dark);
    EXPECT_EQ(colors().accent.r, green_500.r);
    EXPECT_EQ(colors().accent.g, green_500.g);
    EXPECT_EQ(colors().accent.b, green_500.b);

    Theme::set(ThemeMode::Light);
    EXPECT_TRUE(Theme::mode() == ThemeMode::Light);
    EXPECT_EQ(colors().accent.r, green_500.r);
    EXPECT_EQ(colors().accent.g, green_500.g);
    EXPECT_EQ(colors().accent.b, green_500.b);
}

void test_theme_swap_changes_background_and_emits_signal() {
    using namespace hui::theme;

    // 从 Light 起步，保证接下来的 set(Dark) 一定触发 emit。
    Theme::set(ThemeMode::Light);
    const hui::Color light_bg = colors().bg_base;

    int emitted = 0;
    ThemeMode observed = ThemeMode::Light;
    const auto id = Theme::on_changed().connect([&](ThemeMode mode) {
        ++emitted;
        observed = mode;
    });

    Theme::set(ThemeMode::Dark);
    EXPECT_EQ(emitted, 1);
    EXPECT_TRUE(observed == ThemeMode::Dark);

    const hui::Color dark_bg = colors().bg_base;
    EXPECT_TRUE(!(light_bg.r == dark_bg.r && light_bg.g == dark_bg.g && light_bg.b == dark_bg.b));

    // 相同 mode 再 set 一次，不应再 emit
    Theme::set(ThemeMode::Dark);
    EXPECT_EQ(emitted, 1);

    // toggle 走另一边
    Theme::toggle();
    EXPECT_TRUE(Theme::mode() == ThemeMode::Light);
    EXPECT_EQ(emitted, 2);

    Theme::on_changed().disconnect(id);
}

void test_theme_palette_green_500_matches_user_spec() {
    using namespace hui::theme;
    const hui::Color expected = hui::Color::from_hex("#22C55E");
    EXPECT_EQ(palette().green_500.r, expected.r);
    EXPECT_EQ(palette().green_500.g, expected.g);
    EXPECT_EQ(palette().green_500.b, expected.b);
}

void test_progress_bar_value_clamps_and_emits() {
    hui::ProgressBar bar;
    int emits = 0;
    float last = -1.0F;
    bar.on_changed().connect([&](float v) { ++emits; last = v; });

    bar.set_value(0.4F);
    EXPECT_EQ(bar.value(), 0.4F);
    EXPECT_EQ(emits, 1);
    EXPECT_EQ(last, 0.4F);

    // 相同值不 emit
    bar.set_value(0.4F);
    EXPECT_EQ(emits, 1);

    // 超范围 clamp
    bar.set_value(1.7F);
    EXPECT_EQ(bar.value(), 1.0F);
    bar.set_value(-0.3F);
    EXPECT_EQ(bar.value(), 0.0F);
}

void test_semi_gauge_tick_converges_to_target() {
    hui::SemiGauge gauge;
    gauge.set_value(0.0F);  // 初始化 displayed_ = 0
    gauge.set_value(1.0F);  // 目标 1

    // 推 60 次（≈ 1s @ 60fps）应该足够收敛
    for (int i = 0; i < 120 && gauge.tick(1.0F / 60.0F); ++i) {
    }
    // 再 tick 一次应该返回 false（已稳定）
    EXPECT_TRUE(!gauge.tick(1.0F / 60.0F));
}

void test_tooltip_does_not_appear_without_hover() {
    hui::Tooltip tip;
    tip.set_text("hint");
    tip.set_frame(hui::RectF{0.0F, 0.0F, 400.0F, 300.0F});
    tip.set_show_delay_ms(100.0F);

    // 无 hover，tick 推 0.5s 也不显示
    for (int i = 0; i < 30; ++i) tip.tick(1.0F / 60.0F);
    EXPECT_TRUE(!tip.visible_now());

    // hover 后推过 delay 应出现
    tip.set_hovered_anchor(true);
    for (int i = 0; i < 30; ++i) tip.tick(1.0F / 60.0F);
    EXPECT_TRUE(tip.visible_now());

    // 取消 hover 立即消
    tip.set_hovered_anchor(false);
    tip.tick(1.0F / 60.0F);
    EXPECT_TRUE(!tip.visible_now());
}

void test_advance_focus_cycles_through_focusable_widgets() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 400.0F, 300.0F});

    auto a_owned = std::make_unique<hui::TextInput>();
    auto* a = a_owned.get();
    a->set_frame(hui::RectF{10.0F, 10.0F, 100.0F, 32.0F});
    auto b_owned = std::make_unique<hui::TextInput>();
    auto* b = b_owned.get();
    b->set_frame(hui::RectF{120.0F, 10.0F, 100.0F, 32.0F});
    auto c_owned = std::make_unique<hui::TextInput>();
    auto* c = c_owned.get();
    c->set_frame(hui::RectF{230.0F, 10.0F, 100.0F, 32.0F});
    root.append_child(std::move(a_owned));
    root.append_child(std::move(b_owned));
    root.append_child(std::move(c_owned));

    std::vector<hui::Widget*> focusable;
    hui::EventDispatcher::collect_focusable(root, focusable);
    EXPECT_EQ(focusable.size(), static_cast<std::size_t>(3));
    EXPECT_TRUE(focusable[0] == a);
    EXPECT_TRUE(focusable[1] == b);
    EXPECT_TRUE(focusable[2] == c);

    hui::EventState state;
    // 无 focus 时正向 advance → 第一个
    EXPECT_TRUE(hui::EventDispatcher::advance_focus(state, &root, /*reverse=*/false));
    EXPECT_TRUE(state.focused == a);
    EXPECT_TRUE(a->focused());

    EXPECT_TRUE(hui::EventDispatcher::advance_focus(state, &root, false));
    EXPECT_TRUE(state.focused == b);

    EXPECT_TRUE(hui::EventDispatcher::advance_focus(state, &root, false));
    EXPECT_TRUE(state.focused == c);

    // 末位 → 循环回头
    EXPECT_TRUE(hui::EventDispatcher::advance_focus(state, &root, false));
    EXPECT_TRUE(state.focused == a);

    // Shift+Tab 反向：a → c
    EXPECT_TRUE(hui::EventDispatcher::advance_focus(state, &root, true));
    EXPECT_TRUE(state.focused == c);

    // disabled 后从 focusable 集合里消失
    b->set_enabled(false);
    focusable.clear();
    hui::EventDispatcher::collect_focusable(root, focusable);
    EXPECT_EQ(focusable.size(), static_cast<std::size_t>(2));
}

void test_application_shortcut_register_match_remove() {
    hui::Application app;

    int b_fired = 0;
    int comma_fired = 0;
    const auto b_id = app.register_shortcut(hui::Shortcut{
        .key_code = 0x42,  // 'B'
        .ctrl = true,
        .callback = [&] { ++b_fired; },
    });
    app.register_shortcut(hui::Shortcut{
        .key_code = 0xBC,  // VK_OEM_COMMA (',')
        .ctrl = true,
        .callback = [&] { ++comma_fired; },
    });
    EXPECT_EQ(app.shortcut_count(), static_cast<std::size_t>(2));

    // Ctrl+B 命中
    EXPECT_TRUE(app.try_invoke_shortcut(0x42, /*ctrl=*/true, /*shift=*/false, /*alt=*/false));
    EXPECT_EQ(b_fired, 1);

    // 单按 B（无 Ctrl）不命中
    EXPECT_TRUE(!app.try_invoke_shortcut(0x42, false, false, false));
    EXPECT_EQ(b_fired, 1);

    // Ctrl+Shift+B 不命中（modifier 必须精确匹配）
    EXPECT_TRUE(!app.try_invoke_shortcut(0x42, true, true, false));
    EXPECT_EQ(b_fired, 1);

    // Ctrl+, 命中
    EXPECT_TRUE(app.try_invoke_shortcut(0xBC, true, false, false));
    EXPECT_EQ(comma_fired, 1);

    // remove 后 B 不再命中
    app.remove_shortcut(b_id);
    EXPECT_EQ(app.shortcut_count(), static_cast<std::size_t>(1));
    EXPECT_TRUE(!app.try_invoke_shortcut(0x42, true, false, false));
    EXPECT_EQ(b_fired, 1);
}

void test_scroll_view_offsets_content_arrange_and_clamps() {
    hui::ScrollView sv;
    sv.set_show_scrollbars(false);
    auto content = std::make_unique<hui::Panel>();
    auto* content_ptr = content.get();
    // measure 默认返回 frame size，让 content 有 800x1500 的"想要尺寸"
    content_ptr->set_frame(hui::RectF{0.0F, 0.0F, 800.0F, 1500.0F});
    sv.set_content(std::move(content));

    sv.arrange(hui::RectF{10.0F, 20.0F, 400.0F, 300.0F});

    // 默认 offset = 0，content frame.x/y 应等于 sv frame.x/y
    EXPECT_EQ(content_ptr->frame().x, 10.0F);
    EXPECT_EQ(content_ptr->frame().y, 20.0F);
    // content 高度撑大 (max(needed.h, frame.h) = max(1500, 300) = 1500)
    EXPECT_EQ(content_ptr->frame().height, 1500.0F);

    // 滚动 200px 向下
    sv.set_scroll_offset(hui::PointF{0.0F, 200.0F});
    sv.arrange(hui::RectF{10.0F, 20.0F, 400.0F, 300.0F});
    EXPECT_EQ(content_ptr->frame().y, 20.0F - 200.0F);

    // 越界 clamp:max_y = 1500 - 300 = 1200
    sv.set_scroll_offset(hui::PointF{0.0F, 5000.0F});
    EXPECT_EQ(sv.scroll_offset().y, 1200.0F);

    // ScrollView 声明 clips_children = true
    EXPECT_TRUE(sv.clips_children());
}

void test_scroll_view_mouse_wheel_event_advances_offset() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 500.0F, 500.0F});

    auto sv_owned = std::make_unique<hui::ScrollView>();
    auto* sv = sv_owned.get();
    auto content = std::make_unique<hui::Panel>();
    content->set_frame(hui::RectF{0.0F, 0.0F, 400.0F, 2000.0F});
    sv->set_content(std::move(content));
    root.append_child(std::move(sv_owned));
    sv->arrange(hui::RectF{0.0F, 0.0F, 400.0F, 300.0F});

    EXPECT_EQ(sv->scroll_offset().y, 0.0F);

    // wheel down (delta = -1.0,wheel_step 默认 40)→ offset += 40
    EXPECT_TRUE(hui::EventDispatcher::dispatch_pointer_wheel(
        &root, hui::PointF{100.0F, 100.0F}, -1.0F));
    EXPECT_EQ(sv->scroll_offset().y, 40.0F);

    // wheel up
    EXPECT_TRUE(hui::EventDispatcher::dispatch_pointer_wheel(
        &root, hui::PointF{100.0F, 100.0F}, 0.5F));
    EXPECT_EQ(sv->scroll_offset().y, 20.0F);

    // 命中点不在 ScrollView 内 → 不消费
    EXPECT_TRUE(!hui::EventDispatcher::dispatch_pointer_wheel(
        &root, hui::PointF{600.0F, 600.0F}, -1.0F));
}

void test_toolbar_horizontal_lays_out_children_left_to_right_with_gap() {
    hui::Toolbar bar;
    bar.set_orientation(hui::ToolbarOrientation::Horizontal);
    bar.set_padding(8.0F);
    bar.set_gap(10.0F);

    auto a = std::make_unique<hui::Button>("A");
    auto b = std::make_unique<hui::Button>("B");
    a->set_frame(hui::RectF{0.0F, 0.0F, 60.0F, 32.0F});  // measure 默认返回 frame size
    b->set_frame(hui::RectF{0.0F, 0.0F, 80.0F, 32.0F});
    auto* a_ptr = a.get();
    auto* b_ptr = b.get();
    bar.add_item(std::move(a));
    bar.add_item(std::move(b));

    bar.arrange(hui::RectF{0.0F, 0.0F, 300.0F, 48.0F});

    EXPECT_EQ(a_ptr->frame().x, 8.0F);
    EXPECT_EQ(a_ptr->frame().width, 60.0F);
    EXPECT_EQ(b_ptr->frame().x, 78.0F);  // 8 padding + 60 + 10 gap
    EXPECT_EQ(b_ptr->frame().width, 80.0F);
    // 高度撑 inner_h = 48 - 16 = 32
    EXPECT_EQ(a_ptr->frame().height, 32.0F);
}

void test_toolbar_vertical_lays_out_children_top_to_bottom() {
    hui::Toolbar bar;
    bar.set_orientation(hui::ToolbarOrientation::Vertical);
    bar.set_padding(4.0F);
    bar.set_gap(6.0F);

    auto a = std::make_unique<hui::Button>("A");
    auto b = std::make_unique<hui::Button>("B");
    a->set_frame(hui::RectF{0.0F, 0.0F, 80.0F, 24.0F});
    b->set_frame(hui::RectF{0.0F, 0.0F, 80.0F, 30.0F});
    auto* a_ptr = a.get();
    auto* b_ptr = b.get();
    bar.add_item(std::move(a));
    bar.add_item(std::move(b));

    bar.arrange(hui::RectF{0.0F, 0.0F, 100.0F, 200.0F});
    EXPECT_EQ(a_ptr->frame().y, 4.0F);
    EXPECT_EQ(a_ptr->frame().height, 24.0F);
    EXPECT_EQ(b_ptr->frame().y, 34.0F);  // 4 padding + 24 + 6 gap
    EXPECT_EQ(b_ptr->frame().height, 30.0F);
    EXPECT_EQ(a_ptr->frame().width, 92.0F);  // inner_w = 100 - 8
}

void test_dropdown_click_trigger_toggles_open_and_click_item_selects() {
    hui::Dropdown dd;
    dd.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 36.0F});
    dd.set_items({"Alpha", "Beta", "Gamma"});

    int emits = 0;
    std::size_t last = hui::Dropdown::npos;
    dd.on_changed().connect([&](std::size_t v) {
        ++emits;
        last = v;
    });

    EXPECT_TRUE(!dd.open());
    EXPECT_EQ(dd.selected_index(), hui::Dropdown::npos);

    // Click trigger 打开
    dd.on_click(hui::PointF{10.0F, 10.0F});
    EXPECT_TRUE(dd.open());

    // hit_test 在 panel 区域应命中（open 状态扩展）
    const hui::RectF panel = dd.panel_rect();
    EXPECT_TRUE(dd.hit_test(hui::PointF{panel.x + 10.0F, panel.y + 15.0F}));

    // Click item 1（panel 第二项 ≈ panel.y + 4 + 30）
    dd.on_click(hui::PointF{panel.x + 10.0F, panel.y + 4.0F + 30.0F + 5.0F});
    EXPECT_EQ(dd.selected_index(), static_cast<std::size_t>(1));
    EXPECT_EQ(emits, 1);
    EXPECT_EQ(last, static_cast<std::size_t>(1));
    EXPECT_TRUE(!dd.open());  // 选中后自动关
}

void test_dropdown_keyboard_space_opens_esc_closes() {
    hui::Dropdown dd;
    dd.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 36.0F});
    dd.set_items({"A", "B"});
    EXPECT_TRUE(dd.accepts_focus());

    EXPECT_TRUE(dd.on_key_down(32));  // Space
    EXPECT_TRUE(dd.open());

    EXPECT_TRUE(dd.on_key_down(0x1B));  // Esc
    EXPECT_TRUE(!dd.open());

    // disabled 不响应
    dd.set_enabled(false);
    EXPECT_TRUE(!dd.accepts_focus());
    EXPECT_TRUE(!dd.on_key_down(32));
    EXPECT_TRUE(!dd.open());
}

void test_popover_open_state_drives_hit_test_and_emits_signal() {
    hui::Popover pop;
    pop.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 100.0F});

    int emits = 0;
    bool last = false;
    pop.on_open_changed().connect([&](bool v) {
        ++emits;
        last = v;
    });

    EXPECT_TRUE(!pop.open());
    // 关时 hit_test 总是 false（事件穿透）
    EXPECT_TRUE(!pop.hit_test(hui::PointF{50.0F, 50.0F}));

    pop.set_open(true);
    EXPECT_TRUE(pop.open());
    EXPECT_EQ(emits, 1);
    EXPECT_TRUE(last);
    EXPECT_TRUE(pop.hit_test(hui::PointF{50.0F, 50.0F}));
    EXPECT_TRUE(!pop.hit_test(hui::PointF{500.0F, 500.0F}));  // frame 外仍 false

    pop.toggle();
    EXPECT_TRUE(!pop.open());
    EXPECT_EQ(emits, 2);
}

void test_popover_set_content_takes_ownership() {
    hui::Popover pop;
    auto inner = std::make_unique<hui::Text>("inside");
    auto* inner_ptr = inner.get();
    pop.set_content(std::move(inner));
    EXPECT_TRUE(pop.content() == inner_ptr);

    // 替换 content，旧的应析构（间接验证：raw 指针被新 content 替换）
    pop.set_content(std::make_unique<hui::Text>("replacement"));
    EXPECT_TRUE(pop.content() != inner_ptr);
}

void test_timeline_click_emits_seek_and_moves_playhead() {
    hui::Timeline timeline;
    timeline.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 40.0F});
    timeline.set_duration(60.0);

    int emits = 0;
    double last = -1.0;
    timeline.on_seek().connect([&](double v) {
        ++emits;
        last = v;
    });

    // x = 8 + 192 * 0.5 = 104（去掉左右各 8px padding，track 宽 184）—— 取 t=0.5
    timeline.on_click(hui::PointF{8.0F + 184.0F * 0.5F, 20.0F});
    EXPECT_EQ(emits, 1);
    EXPECT_TRUE(timeline.playhead() > 29.5 && timeline.playhead() < 30.5);
    EXPECT_TRUE(last > 29.5 && last < 30.5);
}

void test_timeline_keyboard_left_right_home_end() {
    hui::Timeline timeline;
    timeline.set_duration(10.0);
    timeline.set_playhead(5.0);
    EXPECT_TRUE(timeline.accepts_focus());

    constexpr std::int32_t vk_left = 0x25;
    constexpr std::int32_t vk_right = 0x27;
    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_end = 0x23;

    EXPECT_TRUE(timeline.on_key_down(vk_right));
    EXPECT_EQ(timeline.playhead(), 6.0);
    EXPECT_TRUE(timeline.on_key_down(vk_left));
    EXPECT_EQ(timeline.playhead(), 5.0);
    EXPECT_TRUE(timeline.on_key_down(vk_home));
    EXPECT_EQ(timeline.playhead(), 0.0);
    EXPECT_TRUE(timeline.on_key_down(vk_end));
    EXPECT_EQ(timeline.playhead(), 10.0);
}

void test_timeline_bound_position_source_syncs_playhead_on_tick() {
    hui::LatestValue<double> position;
    hui::Timeline timeline;
    timeline.set_duration(60.0);
    timeline.bind_position_source(&position);

    EXPECT_TRUE(timeline.is_position_source_bound());
    EXPECT_TRUE(timeline.tick(0.016F));  // 绑定后 always-on
    EXPECT_EQ(timeline.playhead(), 0.0);  // generation 还是 0，没 pull

    position.publish(12.5);
    EXPECT_TRUE(timeline.tick(0.016F));
    EXPECT_EQ(timeline.playhead(), 12.5);

    // 再 tick 没新值不应改 playhead
    timeline.tick(0.016F);
    EXPECT_EQ(timeline.playhead(), 12.5);
}

void test_level_meter_set_value_clamps_to_unit_range() {
    hui::LevelMeter meter;
    meter.set_value(1.7F);
    EXPECT_EQ(meter.value(), 1.0F);
    meter.set_value(-0.3F);
    EXPECT_EQ(meter.value(), 0.0F);
    meter.set_value(0.5F);
    EXPECT_EQ(meter.value(), 0.5F);
}

void test_level_meter_peak_hold_tracks_max_then_decays() {
    hui::LevelMeter meter;
    meter.set_peak_hold(true);
    meter.set_value(0.8F);
    EXPECT_EQ(meter.peak(), 0.8F);

    // value 降下来后 peak 仍保留
    meter.set_value(0.3F);
    EXPECT_EQ(meter.peak(), 0.8F);

    // 推 2s tick 让 peak 衰减（rate = 0.45/s，2s 衰减 ~0.9）
    for (int i = 0; i < 120; ++i) {
        meter.tick(1.0F / 60.0F);
    }
    // 衰减底线是 value（0.3）
    EXPECT_TRUE(meter.peak() <= 0.3F + 0.05F);
}

void test_level_meter_bound_source_pulls_value_on_tick() {
    hui::LatestValue<float> source;
    hui::LevelMeter meter;
    meter.bind_source(&source);
    EXPECT_TRUE(meter.is_source_bound());

    source.publish(0.65F);
    meter.tick(0.016F);
    EXPECT_EQ(meter.value(), 0.65F);

    source.publish(0.92F);
    meter.tick(0.016F);
    EXPECT_EQ(meter.value(), 0.92F);
}

void test_token_override_merge_only_overrides_set_fields() {
    hui::theme::SemanticColor base{};
    base.accent = hui::Color{1.0F, 0.0F, 0.0F, 1.0F};
    base.text_primary = hui::Color{0.0F, 1.0F, 0.0F, 1.0F};
    base.bg_surface = hui::Color{0.0F, 0.0F, 1.0F, 1.0F};

    // override 只设 accent
    hui::theme::TokenOverride override{};
    override.with_accent(hui::Color{0.5F, 0.5F, 0.5F, 1.0F});

    const auto merged = hui::theme::merge_override(base, &override);
    EXPECT_EQ(merged.accent.r, 0.5F);
    EXPECT_EQ(merged.text_primary.g, 1.0F);  // 没 override，保留 base
    EXPECT_EQ(merged.bg_surface.b, 1.0F);    // 没 override，保留 base

    // override = nullptr 完全等价于 base
    const auto null_merged = hui::theme::merge_override(base, nullptr);
    EXPECT_EQ(null_merged.accent.r, base.accent.r);
}

void test_timeline_token_override_visible_through_resolve() {
    hui::Timeline timeline;
    timeline.set_duration(10.0);
    EXPECT_TRUE(timeline.token_override() == nullptr);

    timeline.set_token_override(
        hui::theme::TokenOverride{}.with_accent(hui::Color{0.1F, 0.2F, 0.3F, 1.0F}));
    EXPECT_TRUE(timeline.token_override() != nullptr);

    // resolve 合并 base + override 应取到 override 的 accent
    const auto resolved = hui::theme::resolve(timeline.token_override());
    EXPECT_EQ(resolved.accent.r, 0.1F);
    EXPECT_EQ(resolved.accent.g, 0.2F);
    EXPECT_EQ(resolved.accent.b, 0.3F);

    // clear 后 token_override 回 null
    timeline.clear_token_override();
    EXPECT_TRUE(timeline.token_override() == nullptr);
    const auto resolved_after = hui::theme::resolve(timeline.token_override());
    // 没 override 时 resolve 等价于 colors() 拷贝（具体值由当前主题决定，不能 EXPECT 具体值）
    EXPECT_TRUE(resolved_after.accent.a == 1.0F);
}

void test_checkbox_keyboard_space_enter_toggles_and_emits_changed() {
    hui::Checkbox cb;
    EXPECT_TRUE(cb.accepts_focus());

    int emits = 0;
    bool last = false;
    cb.on_changed().connect([&](bool v) {
        ++emits;
        last = v;
    });

    EXPECT_TRUE(cb.on_key_down(32));  // Space
    EXPECT_TRUE(cb.checked());
    EXPECT_EQ(emits, 1);
    EXPECT_TRUE(last);

    EXPECT_TRUE(cb.on_key_down(0x0D));  // Enter
    EXPECT_TRUE(!cb.checked());
    EXPECT_EQ(emits, 2);

    // 不认识的键不消费
    EXPECT_TRUE(!cb.on_key_down(0x70));

    // disabled 后键盘不响应
    cb.set_enabled(false);
    EXPECT_TRUE(!cb.accepts_focus());
    EXPECT_TRUE(!cb.on_key_down(32));
    EXPECT_TRUE(!cb.checked());
    EXPECT_EQ(emits, 2);
}

void test_switch_keyboard_space_enter_toggles() {
    hui::Switch sw;
    EXPECT_TRUE(sw.accepts_focus());

    int emits = 0;
    sw.on_changed().connect([&](bool) { ++emits; });

    EXPECT_TRUE(sw.on_key_down(32));
    EXPECT_TRUE(sw.checked());
    EXPECT_EQ(emits, 1);

    EXPECT_TRUE(sw.on_key_down(0x0D));
    EXPECT_TRUE(!sw.checked());

    sw.set_enabled(false);
    EXPECT_TRUE(!sw.accepts_focus());
    EXPECT_TRUE(!sw.on_key_down(32));
}

void test_slider_keyboard_arrows_home_end_pageup_pagedown() {
    hui::Slider slider;
    slider.set_range(0.0F, 100.0F);
    slider.set_value(50.0F);
    EXPECT_TRUE(slider.accepts_focus());

    constexpr std::int32_t vk_left = 0x25;
    constexpr std::int32_t vk_right = 0x27;
    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_end = 0x23;
    constexpr std::int32_t vk_pageup = 0x21;
    constexpr std::int32_t vk_pagedown = 0x22;

    // 默认 step = (max-min)/100 = 1
    EXPECT_TRUE(slider.on_key_down(vk_right));
    EXPECT_EQ(slider.value(), 51.0F);
    EXPECT_TRUE(slider.on_key_down(vk_left));
    EXPECT_EQ(slider.value(), 50.0F);

    // PageUp/Down 是 10×step
    EXPECT_TRUE(slider.on_key_down(vk_pageup));
    EXPECT_EQ(slider.value(), 60.0F);
    EXPECT_TRUE(slider.on_key_down(vk_pagedown));
    EXPECT_EQ(slider.value(), 50.0F);

    EXPECT_TRUE(slider.on_key_down(vk_home));
    EXPECT_EQ(slider.value(), 0.0F);
    EXPECT_TRUE(slider.on_key_down(vk_end));
    EXPECT_EQ(slider.value(), 100.0F);

    // 自定义 step
    slider.set_step(5.0F);
    slider.set_value(50.0F);
    EXPECT_TRUE(slider.on_key_down(vk_right));
    EXPECT_EQ(slider.value(), 55.0F);

    // disabled 不响应
    slider.set_enabled(false);
    EXPECT_TRUE(!slider.accepts_focus());
    EXPECT_TRUE(!slider.on_key_down(vk_right));
    EXPECT_EQ(slider.value(), 55.0F);
}

void test_realtime_source_latest_value_publish_increments_generation() {
    hui::LatestValue<int> source;
    EXPECT_EQ(source.generation(), static_cast<std::uint64_t>(0));
    EXPECT_TRUE(!source.latest().has_value());

    source.publish(7);
    EXPECT_EQ(source.generation(), static_cast<std::uint64_t>(1));
    EXPECT_TRUE(source.latest().has_value());
    EXPECT_EQ(source.latest().value(), 7);

    source.publish(13);
    EXPECT_EQ(source.generation(), static_cast<std::uint64_t>(2));
    EXPECT_EQ(source.latest().value(), 13);

    source.clear();
    EXPECT_EQ(source.generation(), static_cast<std::uint64_t>(3));
    EXPECT_TRUE(!source.latest().has_value());
}

void test_realtime_ring_buffer_push_increments_generation_and_overwrites_oldest() {
    hui::RealtimeRingBuffer<int> ring(3);
    EXPECT_EQ(ring.generation(), static_cast<std::uint64_t>(0));

    ring.push(1);
    ring.push(2);
    ring.push(3);
    EXPECT_EQ(ring.generation(), static_cast<std::uint64_t>(3));
    EXPECT_EQ(ring.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(ring.latest().value(), 3);

    // 满了之后再 push 应覆盖最旧的，但 size 仍 = capacity，generation 继续递增
    ring.push(4);
    EXPECT_EQ(ring.generation(), static_cast<std::uint64_t>(4));
    EXPECT_EQ(ring.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(ring.latest().value(), 4);
    // pop 顺序应该是 2, 3, 4（1 被覆盖了）
    EXPECT_EQ(ring.pop().value(), 2);
    EXPECT_EQ(ring.pop().value(), 3);
    EXPECT_EQ(ring.pop().value(), 4);
    EXPECT_TRUE(!ring.pop().has_value());
}

void test_realtime_source_concurrent_publish_snapshot_yields_consistent_values() {
    // producer 在另一个线程持续 publish 单调值；consumer 主线程持续 sample
    // generation 看到变化就读 latest，断言：每次读到的值 v <= latest gen 写入的值，
    // 且最终 publish 完后 latest = 最后一个值。这验证了 mutex 保护没让 consumer
    // 读到撕裂或乱序 T。
    hui::LatestValue<int> source;
    constexpr int kIterations = 20000;

    std::atomic<bool> producer_done{false};
    std::thread producer([&] {
        for (int i = 1; i <= kIterations; ++i) {
            source.publish(i);
        }
        producer_done.store(true);
    });

    std::uint64_t last_gen = 0;
    int observed_count = 0;
    int last_value = 0;
    while (!producer_done.load() || source.generation() != last_gen) {
        const auto g = source.generation();
        if (g != last_gen) {
            const auto v = source.latest();
            // generation 已经 > 0，必有值
            if (v.has_value()) {
                // 单调性：每次读到的值不应小于上次（producer 单调写入）
                EXPECT_TRUE(v.value() >= last_value);
                last_value = v.value();
                ++observed_count;
            }
            last_gen = g;
        }
    }
    producer.join();

    EXPECT_EQ(source.latest().value(), kIterations);
    EXPECT_EQ(source.generation(), static_cast<std::uint64_t>(kIterations));
    // consumer 至少 sample 到一次（实际通常远不止一次）
    EXPECT_TRUE(observed_count >= 1);
}

void test_realtime_canvas_unbound_tick_returns_false() {
    hui::RealtimeCanvas canvas;
    EXPECT_TRUE(!canvas.is_source_bound());
    EXPECT_TRUE(!canvas.tick(0.016F));
}

void test_realtime_canvas_bound_source_marks_paint_dirty_on_generation_change() {
    hui::LatestValue<int> source;
    hui::RealtimeCanvas canvas;
    canvas.set_frame(hui::RectF{0.0F, 0.0F, 100.0F, 100.0F});
    canvas.bind_source(&source);
    EXPECT_TRUE(canvas.is_source_bound());

    // bind 后下一帧 tick：source generation 还是 0，没变化，不 mark paint dirty。
    canvas.clear_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(canvas.tick(0.016F));  // 仍返回 true（绑定后保持 timer active）
    EXPECT_TRUE((canvas.dirty_flags() & hui::DirtyFlags::Paint) == hui::DirtyFlags::None);

    // producer publish → generation 变化 → tick 应 mark paint dirty
    source.publish(1);
    canvas.clear_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(canvas.tick(0.016F));
    EXPECT_TRUE((canvas.dirty_flags() & hui::DirtyFlags::Paint) != hui::DirtyFlags::None);
    EXPECT_EQ(canvas.last_observed_generation(), static_cast<std::uint64_t>(1));

    // 再 tick 一次，generation 没变，不应再 mark paint dirty
    canvas.clear_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(canvas.tick(0.016F));
    EXPECT_TRUE((canvas.dirty_flags() & hui::DirtyFlags::Paint) == hui::DirtyFlags::None);

    // 解绑后 tick 应返回 false（不再 always-on）
    canvas.bind_source<int>(nullptr);
    EXPECT_TRUE(!canvas.is_source_bound());
    canvas.clear_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(!canvas.tick(0.016F));
}

void test_video_player_keeps_stable_texture_id_across_frames() {
    hui::VideoPlayer player;
    const std::uint64_t initial_id = player.texture().id;
    EXPECT_TRUE(initial_id != 0U);

    // open + play + 推几帧；id 不应变化（之前每帧 fetch_add 一个新 id 是 bug：
    // 等同告诉 renderer "这是新纹理"，未来 GPU registry cache 永远 miss）
    EXPECT_TRUE(player.set_source("test_dummy_path"));
    player.play();
    for (int i = 0; i < 5; ++i) {
        // delta = 50ms > 1/30s 一帧
        player.tick(0.05F);
    }
    EXPECT_EQ(player.texture().id, initial_id);

    // stop 后清 buffer，但 id 仍稳定
    player.stop();
    EXPECT_EQ(player.texture().id, initial_id);
    EXPECT_TRUE(!player.texture().valid());  // buffer 空 → invalid

    // 重新 set_source 也复用同一 id
    EXPECT_TRUE(player.set_source("another_dummy_path"));
    EXPECT_EQ(player.texture().id, initial_id);
}

void test_tabs_keyboard_left_right_home_end() {
    hui::Tabs tabs;
    tabs.set_frame(hui::RectF{0.0F, 0.0F, 300.0F, 40.0F});
    tabs.set_items({"A", "B", "C", "D"});
    int emits = 0;
    std::size_t last = 0;
    tabs.on_changed().connect([&](std::size_t v) {
        ++emits;
        last = v;
    });

    EXPECT_TRUE(tabs.accepts_focus());

    constexpr std::int32_t vk_right = 0x27;
    constexpr std::int32_t vk_left = 0x25;
    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_end = 0x23;

    EXPECT_TRUE(tabs.on_key_down(vk_right));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(1));
    EXPECT_EQ(emits, 1);
    EXPECT_EQ(last, static_cast<std::size_t>(1));

    EXPECT_TRUE(tabs.on_key_down(vk_end));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(3));

    // 末位再 right 不动，但仍消费按键
    EXPECT_TRUE(tabs.on_key_down(vk_right));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(3));

    EXPECT_TRUE(tabs.on_key_down(vk_left));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(2));

    EXPECT_TRUE(tabs.on_key_down(vk_home));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(0));

    // 首位再 left 不动，仍消费
    EXPECT_TRUE(tabs.on_key_down(vk_left));
    EXPECT_EQ(tabs.selected_index(), static_cast<std::size_t>(0));

    // 不认识的按键不消费
    EXPECT_TRUE(!tabs.on_key_down(0x70));
}

void test_dialog_modal_hit_test_extends_to_parent_frame() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 800.0F, 600.0F});

    auto button_owned = std::make_unique<hui::Button>("under");
    auto* button = button_owned.get();
    button->set_frame(hui::RectF{50.0F, 50.0F, 80.0F, 30.0F});
    root.append_child(std::move(button_owned));

    auto dialog_owned = std::make_unique<hui::Dialog>("Title", "Body");
    auto* dialog = dialog_owned.get();
    // 故意把 dialog frame 设小，验证 modal hit_test 仍能扩到 parent frame
    dialog->set_frame(hui::RectF{300.0F, 200.0F, 200.0F, 150.0F});
    dialog->set_open(true);
    root.append_child(std::move(dialog_owned));

    // 命中测试：scrim 区域（dialog frame 之外、root frame 之内）应命中 dialog
    // 而非 button。这是 modal 阻断兄弟 widget 的关键。
    hui::Widget* hit_in_scrim =
        hui::EventDispatcher::hit_test(root, hui::PointF{60.0F, 60.0F});
    EXPECT_TRUE(hit_in_scrim == dialog);

    // 关掉 dialog 后 button 重新可达
    dialog->set_open(false);
    hui::Widget* hit_after_close =
        hui::EventDispatcher::hit_test(root, hui::PointF{60.0F, 60.0F});
    EXPECT_TRUE(hit_after_close == button);
}

void test_dialog_modal_absorbs_click_via_handled_flag() {
    hui::Dialog dialog{"T", "M"};
    dialog.set_frame(hui::RectF{0.0F, 0.0F, 320.0F, 220.0F});
    dialog.set_open(true);

    // 点击在 confirm 按钮之外（scrim 区域内）：应消费事件且不关闭 dialog
    hui::MouseEvent mouse{};
    mouse.kind = hui::EventKind::MouseClick;
    mouse.position = hui::PointF{10.0F, 10.0F};
    hui::Event ev = mouse;
    hui::EventBase& base = hui::event_base(ev);
    base.phase = hui::EventPhase::Target;

    const bool handled = dialog.on_event(ev);
    EXPECT_TRUE(handled);
    EXPECT_TRUE(base.handled);
    EXPECT_TRUE(dialog.open());

    // 点击 confirm 仍应正常关闭（默认路由通过 Widget::on_event 调到 on_click）
    const hui::RectF confirm = dialog.confirm_rect();
    hui::MouseEvent click_confirm{};
    click_confirm.kind = hui::EventKind::MouseClick;
    click_confirm.position = hui::PointF{confirm.x + 4.0F, confirm.y + 4.0F};
    hui::Event ev2 = click_confirm;
    hui::EventBase& base2 = hui::event_base(ev2);
    base2.phase = hui::EventPhase::Target;
    dialog.on_event(ev2);
    EXPECT_TRUE(!dialog.open());
}

void test_text_input_cursor_starts_at_zero_and_set_text_jumps_to_end() {
    hui::TextInput input;
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(0));
    EXPECT_TRUE(!input.has_selection());

    input.set_text("hello");
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(5));
    EXPECT_TRUE(!input.has_selection());
}

void test_text_input_arrow_keys_move_cursor_without_extending_selection() {
    hui::TextInput input;
    input.set_text("abcd");

    constexpr std::int32_t vk_left = 0x25;
    constexpr std::int32_t vk_right = 0x27;
    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_end = 0x23;

    EXPECT_TRUE(input.on_key_down(vk_left));
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(3));
    EXPECT_TRUE(!input.has_selection());

    EXPECT_TRUE(input.on_key_down(vk_home));
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(0));

    EXPECT_TRUE(input.on_key_down(vk_right));
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(1));

    EXPECT_TRUE(input.on_key_down(vk_end));
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(4));
}

void test_text_input_shift_arrow_extends_selection_via_on_event() {
    hui::TextInput input;
    input.set_text("abcd");
    input.set_focused(true);
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(4));

    constexpr std::int32_t vk_left = 0x25;
    auto press_shift_left = [&] {
        hui::KeyEvent k{};
        k.kind = hui::EventKind::KeyDown;
        k.key_code = vk_left;
        k.shift = true;
        hui::Event ev = k;
        hui::EventBase& base = hui::event_base(ev);
        base.phase = hui::EventPhase::Target;
        input.on_event(ev);
    };

    press_shift_left();
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(3));
    EXPECT_TRUE(input.has_selection());
    EXPECT_EQ(input.selection_anchor(), static_cast<std::size_t>(4));

    press_shift_left();
    press_shift_left();
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(1));
    EXPECT_EQ(input.selection_min(), static_cast<std::size_t>(1));
    EXPECT_EQ(input.selection_max(), static_cast<std::size_t>(4));

    // 普通 left（无 shift）应该收缩到 cursor，清选区
    EXPECT_TRUE(input.on_key_down(vk_left));
    EXPECT_TRUE(!input.has_selection());
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(0));
}

void test_text_input_delete_removes_selection_or_next_char() {
    hui::TextInput input;
    input.set_text("hello");
    constexpr std::int32_t vk_delete = 0x2E;
    constexpr std::int32_t vk_home = 0x24;

    EXPECT_TRUE(input.on_key_down(vk_home));  // cursor=0
    EXPECT_TRUE(input.on_key_down(vk_delete));
    EXPECT_EQ(input.text(), std::string{"ello"});
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(0));

    // cursor 在末尾时 delete 不消费
    constexpr std::int32_t vk_end = 0x23;
    EXPECT_TRUE(input.on_key_down(vk_end));
    EXPECT_TRUE(!input.on_key_down(vk_delete));

    // 选区下 delete 删选区
    input.set_text("hello");
    constexpr std::int32_t vk_left = 0x25;
    auto press_shift_left = [&] {
        hui::KeyEvent k{};
        k.kind = hui::EventKind::KeyDown;
        k.key_code = vk_left;
        k.shift = true;
        hui::Event ev = k;
        hui::event_base(ev).phase = hui::EventPhase::Target;
        input.on_event(ev);
    };
    press_shift_left();
    press_shift_left();
    EXPECT_TRUE(input.has_selection());
    EXPECT_TRUE(input.on_key_down(vk_delete));
    EXPECT_EQ(input.text(), std::string{"hel"});
    EXPECT_TRUE(!input.has_selection());
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(3));
}

void test_text_input_typing_replaces_selection() {
    hui::TextInput input;
    input.set_text("abcd");

    constexpr std::int32_t vk_home = 0x24;
    constexpr std::int32_t vk_right = 0x27;
    EXPECT_TRUE(input.on_key_down(vk_home));

    // shift+right 两次：选区 [0, 2)
    auto press_shift_right = [&] {
        hui::KeyEvent k{};
        k.kind = hui::EventKind::KeyDown;
        k.key_code = vk_right;
        k.shift = true;
        hui::Event ev = k;
        hui::event_base(ev).phase = hui::EventPhase::Target;
        input.on_event(ev);
    };
    press_shift_right();
    press_shift_right();
    EXPECT_EQ(input.selection_min(), static_cast<std::size_t>(0));
    EXPECT_EQ(input.selection_max(), static_cast<std::size_t>(2));

    // 输入 'X' 应替换选区
    EXPECT_TRUE(input.on_text_input('X'));
    EXPECT_EQ(input.text(), std::string{"Xcd"});
    EXPECT_EQ(input.cursor_pos(), static_cast<std::size_t>(1));
    EXPECT_TRUE(!input.has_selection());
}

void test_dispatch_key_down_propagates_shift_modifier_through_focus_chain() {
    hui::Panel root;
    root.set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto input_owned = std::make_unique<hui::TextInput>();
    auto* input = input_owned.get();
    input->set_frame(hui::RectF{20.0F, 20.0F, 100.0F, 32.0F});
    input->set_text("ab");
    root.append_child(std::move(input_owned));

    hui::EventState state;
    hui::EventDispatcher::set_focus(state, input);

    // dispatch_key_down 带 shift，TextInput 通过 on_event 应该读到 shift 扩选
    constexpr std::int32_t vk_left = 0x25;
    EXPECT_TRUE(hui::EventDispatcher::dispatch_key_down(state, vk_left, /*shift=*/true));
    EXPECT_TRUE(input->has_selection());
    EXPECT_EQ(input->cursor_pos(), static_cast<std::size_t>(1));
    EXPECT_EQ(input->selection_anchor(), static_cast<std::size_t>(2));
}

void test_mark_dirty_propagates_subtree_summary_to_ancestors() {
    hui::Panel root;
    auto mid = std::make_unique<hui::Panel>();
    auto* mid_ptr = mid.get();
    auto leaf = std::make_unique<hui::Button>("X");
    auto* leaf_ptr = leaf.get();
    mid->append_child(std::move(leaf));
    root.append_child(std::move(mid));

    // 整树清干净（含 Structure dirty 默认值），把 subtree_dirty 也归零
    auto clear_all = [](hui::Element& e) {
        e.clear_dirty(hui::DirtyFlags::All);
        e.clear_subtree_dirty(hui::DirtyFlags::All);
    };
    clear_all(root);
    clear_all(*mid_ptr);
    clear_all(*leaf_ptr);

    // 叶子标 paint：自己 dirty + 祖先 subtree_dirty 都应有 paint，但祖先 dirty 不动
    leaf_ptr->invalidate(hui::DirtyFlags::Paint);
    EXPECT_TRUE((leaf_ptr->dirty_flags() & hui::DirtyFlags::Paint) != hui::DirtyFlags::None);
    EXPECT_TRUE((mid_ptr->subtree_dirty_flags() & hui::DirtyFlags::Paint) != hui::DirtyFlags::None);
    EXPECT_TRUE((root.subtree_dirty_flags() & hui::DirtyFlags::Paint) != hui::DirtyFlags::None);
    EXPECT_TRUE((mid_ptr->dirty_flags() & hui::DirtyFlags::Paint) == hui::DirtyFlags::None);
    EXPECT_TRUE((root.dirty_flags() & hui::DirtyFlags::Paint) == hui::DirtyFlags::None);
}

void test_clear_subtree_dirty_lets_new_dirty_propagate() {
    hui::Panel root;
    auto leaf = std::make_unique<hui::Button>("X");
    auto* leaf_ptr = leaf.get();
    root.append_child(std::move(leaf));

    root.clear_dirty(hui::DirtyFlags::All);
    leaf_ptr->clear_dirty(hui::DirtyFlags::All);
    root.clear_subtree_dirty(hui::DirtyFlags::All);

    leaf_ptr->invalidate(hui::DirtyFlags::Paint);
    EXPECT_TRUE((root.subtree_dirty_flags() & hui::DirtyFlags::Paint) != hui::DirtyFlags::None);

    // 模拟一帧渲染完：叶子和 root 的 subtree_dirty 都清。再 mark 应能再次冒上去。
    leaf_ptr->clear_dirty(hui::DirtyFlags::All);
    leaf_ptr->clear_subtree_dirty(hui::DirtyFlags::All);
    root.clear_subtree_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(root.subtree_dirty_flags() == hui::DirtyFlags::None);

    leaf_ptr->invalidate(hui::DirtyFlags::Layout);
    EXPECT_TRUE((root.subtree_dirty_flags() & hui::DirtyFlags::Layout) != hui::DirtyFlags::None);
}

void test_append_child_inherits_subtree_dirty_from_subtree() {
    hui::Panel root;
    root.clear_dirty(hui::DirtyFlags::All);
    root.clear_subtree_dirty(hui::DirtyFlags::All);
    EXPECT_TRUE(root.subtree_dirty_flags() == hui::DirtyFlags::None);

    auto child = std::make_unique<hui::Button>("New");
    // 新建 widget 默认带 Structure dirty
    EXPECT_TRUE((child->dirty_flags() & hui::DirtyFlags::Structure) != hui::DirtyFlags::None);

    root.append_child(std::move(child));
    EXPECT_TRUE((root.subtree_dirty_flags() & hui::DirtyFlags::Structure) != hui::DirtyFlags::None);
}

class PhaseRecorder : public hui::Widget {
  public:
    struct Record {
        hui::EventPhase phase;
        hui::EventKind kind;
    };
    std::vector<Record> records;
    bool consume_in_capture {false};
    bool consume_in_target {false};
    bool consume_in_bubble {false};

    bool on_event(hui::Event& ev) override {
        hui::EventBase& base = hui::event_base(ev);
        hui::EventKind kind = hui::EventKind::MouseMove;
        if (auto* m = std::get_if<hui::MouseEvent>(&ev)) {
            kind = m->kind;
        } else if (auto* k = std::get_if<hui::KeyEvent>(&ev)) {
            kind = k->kind;
        } else if (std::get_if<hui::TextInputEvent>(&ev) != nullptr) {
            kind = hui::EventKind::TextInput;
        }
        records.push_back({base.phase, kind});

        if (base.phase == hui::EventPhase::Capture && consume_in_capture) {
            base.handled = true;
            return true;
        }
        if (base.phase == hui::EventPhase::Target && consume_in_target) {
            base.handled = true;
            return true;
        }
        if (base.phase == hui::EventPhase::Bubble && consume_in_bubble) {
            base.handled = true;
            return true;
        }
        // Target phase 把事件路由到旧 on_xxx 虚函数（默认实现）
        if (base.phase == hui::EventPhase::Target) {
            return hui::Widget::on_event(ev);
        }
        return false;
    }
};

void test_widget_on_event_default_routes_target_click_to_on_click() {
    hui::Button button{"Run"};
    button.set_frame(hui::RectF{0.0F, 0.0F, 60.0F, 30.0F});
    bool clicked = false;
    button.on_clicked().connect([&] { clicked = true; });

    hui::MouseEvent mouse{};
    mouse.kind = hui::EventKind::MouseClick;
    mouse.position = hui::PointF{10.0F, 10.0F};
    hui::Event event = mouse;
    hui::EventBase& base = hui::event_base(event);
    base.phase = hui::EventPhase::Target;

    button.on_event(event);
    EXPECT_TRUE(clicked);

    // capture / bubble 阶段默认不触发副作用
    bool clicked_again = false;
    button.on_clicked().connect([&] { clicked_again = true; });
    base.phase = hui::EventPhase::Capture;
    base.handled = false;
    button.on_event(event);
    EXPECT_TRUE(!clicked_again);

    base.phase = hui::EventPhase::Bubble;
    button.on_event(event);
    EXPECT_TRUE(!clicked_again);
}

void test_dispatch_event_along_path_visits_capture_target_bubble_in_order() {
    auto root_owned = std::make_unique<PhaseRecorder>();
    auto* root = root_owned.get();
    root->set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto mid_owned = std::make_unique<PhaseRecorder>();
    auto* mid = mid_owned.get();
    mid->set_frame(hui::RectF{20.0F, 20.0F, 160.0F, 160.0F});

    auto leaf_owned = std::make_unique<PhaseRecorder>();
    auto* leaf = leaf_owned.get();
    leaf->set_frame(hui::RectF{40.0F, 40.0F, 80.0F, 30.0F});

    mid->append_child(std::move(leaf_owned));
    root->append_child(std::move(mid_owned));

    std::vector<hui::Widget*> path;
    hui::EventDispatcher::build_path(*root, hui::PointF{60.0F, 50.0F}, path);
    EXPECT_EQ(path.size(), static_cast<std::size_t>(3));
    EXPECT_TRUE(path[0] == root);
    EXPECT_TRUE(path[1] == mid);
    EXPECT_TRUE(path[2] == leaf);

    hui::MouseEvent mouse{};
    mouse.kind = hui::EventKind::MouseClick;
    mouse.position = hui::PointF{60.0F, 50.0F};
    hui::Event event = mouse;
    hui::EventDispatcher::dispatch_event_along_path(path, event);

    // root: Capture, Bubble
    EXPECT_EQ(root->records.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(root->records[0].phase == hui::EventPhase::Capture);
    EXPECT_TRUE(root->records[1].phase == hui::EventPhase::Bubble);

    // mid: Capture, Bubble
    EXPECT_EQ(mid->records.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(mid->records[0].phase == hui::EventPhase::Capture);
    EXPECT_TRUE(mid->records[1].phase == hui::EventPhase::Bubble);

    // leaf: 只有 Target
    EXPECT_EQ(leaf->records.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(leaf->records[0].phase == hui::EventPhase::Target);
}

void test_dispatch_event_capture_handled_stops_propagation() {
    auto root_owned = std::make_unique<PhaseRecorder>();
    auto* root = root_owned.get();
    root->set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});
    root->consume_in_capture = true;

    auto leaf_owned = std::make_unique<PhaseRecorder>();
    auto* leaf = leaf_owned.get();
    leaf->set_frame(hui::RectF{20.0F, 20.0F, 80.0F, 30.0F});
    root->append_child(std::move(leaf_owned));

    std::vector<hui::Widget*> path;
    hui::EventDispatcher::build_path(*root, hui::PointF{40.0F, 30.0F}, path);

    hui::MouseEvent mouse{};
    mouse.kind = hui::EventKind::MouseClick;
    mouse.position = hui::PointF{40.0F, 30.0F};
    hui::Event event = mouse;
    const bool handled = hui::EventDispatcher::dispatch_event_along_path(path, event);

    EXPECT_TRUE(handled);
    EXPECT_EQ(root->records.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(root->records[0].phase == hui::EventPhase::Capture);
    EXPECT_EQ(leaf->records.size(), static_cast<std::size_t>(0));
}

void test_dispatch_event_bubble_reaches_ancestor_after_target() {
    auto root_owned = std::make_unique<PhaseRecorder>();
    auto* root = root_owned.get();
    root->set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto leaf_owned = std::make_unique<PhaseRecorder>();
    auto* leaf = leaf_owned.get();
    leaf->set_frame(hui::RectF{20.0F, 20.0F, 80.0F, 30.0F});
    root->append_child(std::move(leaf_owned));

    std::vector<hui::Widget*> path;
    hui::EventDispatcher::build_path(*root, hui::PointF{40.0F, 30.0F}, path);

    hui::MouseEvent mouse{};
    mouse.kind = hui::EventKind::MouseClick;
    mouse.position = hui::PointF{40.0F, 30.0F};
    hui::Event event = mouse;
    hui::EventDispatcher::dispatch_event_along_path(path, event);

    // root 在 capture（先）和 bubble（后）都应被访问；leaf 仅 target
    EXPECT_EQ(root->records.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(root->records[0].phase == hui::EventPhase::Capture);
    EXPECT_TRUE(root->records[1].phase == hui::EventPhase::Bubble);
    EXPECT_EQ(leaf->records.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(leaf->records[0].phase == hui::EventPhase::Target);
}

void test_dispatch_text_input_consumed_at_target_stops_bubble() {
    auto root_owned = std::make_unique<PhaseRecorder>();
    auto* root = root_owned.get();
    root->set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto input_owned = std::make_unique<hui::TextInput>();
    auto* input = input_owned.get();
    input->set_frame(hui::RectF{20.0F, 20.0F, 100.0F, 32.0F});
    root->append_child(std::move(input_owned));

    hui::EventState state;
    hui::EventDispatcher::set_focus(state, input);

    // 'a' 是有效字符，TextInput 在 Target phase 消费它，bubble 应该被截停
    const bool consumed = hui::EventDispatcher::dispatch_text_input(state, 'a');
    EXPECT_TRUE(consumed);
    EXPECT_EQ(input->text(), std::string{"a"});
    EXPECT_EQ(root->records.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(root->records[0].phase == hui::EventPhase::Capture);
}

void test_dispatch_key_down_bubbles_when_target_does_not_consume() {
    auto root_owned = std::make_unique<PhaseRecorder>();
    auto* root = root_owned.get();
    root->set_frame(hui::RectF{0.0F, 0.0F, 200.0F, 200.0F});

    auto input_owned = std::make_unique<hui::TextInput>();
    auto* input = input_owned.get();
    input->set_frame(hui::RectF{20.0F, 20.0F, 100.0F, 32.0F});
    root->append_child(std::move(input_owned));

    hui::EventState state;
    hui::EventDispatcher::set_focus(state, input);

    // F1 (0x70) — TextInput 只认退格(8)，其它 key_code 不消费，事件应该冒到 root
    constexpr std::int32_t f1_key = 0x70;
    const bool consumed = hui::EventDispatcher::dispatch_key_down(state, f1_key);
    EXPECT_TRUE(!consumed);
    EXPECT_EQ(root->records.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(root->records[0].phase == hui::EventPhase::Capture);
    EXPECT_TRUE(root->records[0].kind == hui::EventKind::KeyDown);
    EXPECT_TRUE(root->records[1].phase == hui::EventPhase::Bubble);
    EXPECT_TRUE(root->records[1].kind == hui::EventKind::KeyDown);
}

void test_chip_click_toggles_and_emits() {
    hui::Chip chip{"Tag"};
    int emits = 0;
    bool last = false;
    chip.on_changed().connect([&](bool s) { ++emits; last = s; });

    chip.on_click(hui::PointF{0.0F, 0.0F});
    EXPECT_TRUE(chip.selected());
    EXPECT_EQ(emits, 1);
    EXPECT_TRUE(last);

    chip.on_click(hui::PointF{0.0F, 0.0F});
    EXPECT_TRUE(!chip.selected());
    EXPECT_EQ(emits, 2);
}

} // namespace

int main() {
    test_property_emits_on_change();
    test_node_sets_parent_when_child_is_appended();
    test_button_click_uses_widget_event();
    test_text_input_handles_focus_text_and_backspace();
    test_selection_widgets_handle_clicks();
    test_tabs_select_item_from_click_position();
    test_dialog_confirm_click_closes_dialog();
    test_title_bar_exposes_window_actions_without_runtime_type_checks();
    test_signal_connect_emit_disconnect();
    test_signal_multiple_subscribers_receive_events();
    test_event_dispatcher_hit_test_returns_deepest_widget();
    test_event_dispatcher_press_capture_sends_move_to_pressed();
    test_event_dispatcher_pointer_up_outside_does_not_activate();
    test_event_dispatcher_focus_update_fires_widget_focus_flag();
    test_flex_box_row_layout_with_pixel_items();
    test_flex_box_row_layout_fill_divides_remaining_by_weight();
    test_flex_box_column_layout_cross_stretch();
    test_color_from_hex_parses_common_forms();
    test_theme_accent_is_green_500_in_both_modes();
    test_theme_swap_changes_background_and_emits_signal();
    test_theme_palette_green_500_matches_user_spec();
    test_progress_bar_value_clamps_and_emits();
    test_semi_gauge_tick_converges_to_target();
    test_tooltip_does_not_appear_without_hover();
    test_advance_focus_cycles_through_focusable_widgets();
    test_application_shortcut_register_match_remove();
    test_scroll_view_offsets_content_arrange_and_clamps();
    test_scroll_view_mouse_wheel_event_advances_offset();
    test_toolbar_horizontal_lays_out_children_left_to_right_with_gap();
    test_toolbar_vertical_lays_out_children_top_to_bottom();
    test_dropdown_click_trigger_toggles_open_and_click_item_selects();
    test_dropdown_keyboard_space_opens_esc_closes();
    test_popover_open_state_drives_hit_test_and_emits_signal();
    test_popover_set_content_takes_ownership();
    test_timeline_click_emits_seek_and_moves_playhead();
    test_timeline_keyboard_left_right_home_end();
    test_timeline_bound_position_source_syncs_playhead_on_tick();
    test_level_meter_set_value_clamps_to_unit_range();
    test_level_meter_peak_hold_tracks_max_then_decays();
    test_level_meter_bound_source_pulls_value_on_tick();
    test_token_override_merge_only_overrides_set_fields();
    test_timeline_token_override_visible_through_resolve();
    test_checkbox_keyboard_space_enter_toggles_and_emits_changed();
    test_switch_keyboard_space_enter_toggles();
    test_slider_keyboard_arrows_home_end_pageup_pagedown();
    test_realtime_source_latest_value_publish_increments_generation();
    test_realtime_ring_buffer_push_increments_generation_and_overwrites_oldest();
    test_realtime_source_concurrent_publish_snapshot_yields_consistent_values();
    test_realtime_canvas_unbound_tick_returns_false();
    test_realtime_canvas_bound_source_marks_paint_dirty_on_generation_change();
    test_video_player_keeps_stable_texture_id_across_frames();
    test_tabs_keyboard_left_right_home_end();
    test_dialog_modal_hit_test_extends_to_parent_frame();
    test_dialog_modal_absorbs_click_via_handled_flag();
    test_text_input_cursor_starts_at_zero_and_set_text_jumps_to_end();
    test_text_input_arrow_keys_move_cursor_without_extending_selection();
    test_text_input_shift_arrow_extends_selection_via_on_event();
    test_text_input_delete_removes_selection_or_next_char();
    test_text_input_typing_replaces_selection();
    test_dispatch_key_down_propagates_shift_modifier_through_focus_chain();
    test_mark_dirty_propagates_subtree_summary_to_ancestors();
    test_clear_subtree_dirty_lets_new_dirty_propagate();
    test_append_child_inherits_subtree_dirty_from_subtree();
    test_widget_on_event_default_routes_target_click_to_on_click();
    test_dispatch_event_along_path_visits_capture_target_bubble_in_order();
    test_dispatch_event_capture_handled_stops_propagation();
    test_dispatch_event_bubble_reaches_ancestor_after_target();
    test_dispatch_text_input_consumed_at_target_stops_bubble();
    test_dispatch_key_down_bubbles_when_target_does_not_consume();
    test_chip_click_toggles_and_emits();

    if (failures != 0) {
        std::cerr << failures << " test expectation(s) failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
