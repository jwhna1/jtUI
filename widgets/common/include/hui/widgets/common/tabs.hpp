#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"

// jtUI Tabs
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

class Tabs : public Widget {
  public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Tabs";
    }

    void set_items(std::vector<std::string> items);

    void add_item(std::string item);

    [[nodiscard]] const std::vector<std::string>& items() const noexcept {
        return items_;
    }

    [[nodiscard]] std::size_t selected_index() const noexcept {
        return selected_index_;
    }

    void set_selected_index(std::size_t index) noexcept;

    // 选中变化通知。比靠每帧轮询 selected_index 便宜得多 —— 不需要为了监听它
    // 挂一个永远 return true 的 TabWatcher，让 animation timer 永不停。
    [[nodiscard]] Signal<std::size_t>& on_changed() noexcept { return changed_; }

    [[nodiscard]] std::size_t index_at(PointF point) const noexcept;

    void set_hovered_index(std::size_t index) noexcept;

    void on_mouse_move(PointF point) override;

    void on_mouse_leave() override;

    void on_click(PointF point) override;

    // 接受焦点：focus 后 ←/→ 切 tab、Home/End 跳首尾。
    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled() && !items_.empty();
    }

    bool on_key_down(std::int32_t key_code) override;

    bool tick(float delta) override;

    void paint(PaintContext& context) const override;

  private:
    std::vector<std::string> items_{};
    std::size_t selected_index_{0};
    std::size_t hovered_index_{npos};
    // 指示条的当前插值位置（单位：tab 下标），平滑滑向 selected_index_。
    float indicator_position_{0.0F};
    // 首次 paint 完才解锁动画，避免 main.cpp 里的初始 set_selected_index 让指示条
    // 从 tab 0 慢悠悠滑到目标位，看起来像开机延迟。
    mutable bool animation_primed_{false};
    Signal<std::size_t> changed_{};
};

} // namespace hui
