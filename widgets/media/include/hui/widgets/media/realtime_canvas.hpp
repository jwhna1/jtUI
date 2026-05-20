#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/property/realtime_source.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

// RealtimeCanvas：spec §14.3 的"高频通道" widget。
//
// 两条 paint 路径：
//   1) 旧的 set_draw_callback(...)：每次 paint 调一次回调，外部自己决定何时
//      mark_dirty。简单但 producer 数据变化和 widget 重画之间没有自动联系。
//   2) 新的 bind_source<T>(source)：绑定一个 RealtimeSource<T>，widget 自己
//      在 tick 里比较 source.generation()，发现变化才 mark Paint dirty。绑定
//      后 tick 始终返回 true 让 animation timer 保持 active —— 这是"高频通道"
//      与主 dirty 流量分离的实现：producer 在任意线程 publish 不触发主 dirty
//      传播，只更新 atomic generation；主线程 60Hz tick 自动 sample，自然限流
//      到 60fps。
//
// 两条路径可以共存：bind_source 提供数据更新触发的重画时机，draw_callback 仍是
// 实际画图的代码（callback 内部从 source.latest() 取数据画）。也可以单独用任意一条。

class RealtimeCanvas : public Widget {
public:
    using DrawCallback = std::function<void(PaintContext&)>;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "RealtimeCanvas";
    }

    void set_draw_callback(DrawCallback callback) {
        draw_callback_ = std::move(callback);
        mark_dirty(DirtyFlags::Paint);
    }

    // 绑定一个 RealtimeSource<T> 作为 paint 触发器。tick 时检查 source 的
    // generation 变化即 mark Paint dirty。passing nullptr 解除绑定。
    template <typename T>
    void bind_source(RealtimeSource<T>* source) {
        if (source == nullptr) {
            source_probe_ = nullptr;
        } else {
            source_probe_ = [source]() noexcept { return source->generation(); };
        }
        last_generation_ = 0;
        // 解绑后 tick 不再 always-on；绑上后下一帧 tick 会感知初始 generation
        // 与 0 的差异，一次 mark dirty 把首屏画出来。
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] bool is_source_bound() const noexcept {
        return static_cast<bool>(source_probe_);
    }

    // 测试 / 观测用：上一次 tick 看到的 generation。绑 source 但 producer 没
    // publish 过时返回 0。
    [[nodiscard]] std::uint64_t last_observed_generation() const noexcept {
        return last_generation_;
    }

    bool tick(float /*delta*/) override {
        if (!source_probe_) {
            return false;
        }
        const auto g = source_probe_();
        if (g != last_generation_) {
            last_generation_ = g;
            mark_dirty(DirtyFlags::Paint);
        }
        // 绑了 source 就保持 timer active：producer 可能在另一线程，主线程必须
        // 持续 sample 才能感知。功耗换实时性；不绑 source 就回退到主 dirty 路径。
        return true;
    }

    void paint(PaintContext& context) const override {
        if (draw_callback_) {
            draw_callback_(context);
        }
    }

private:
    DrawCallback draw_callback_{};
    std::function<std::uint64_t()> source_probe_{};
    std::uint64_t last_generation_{0};
};

}  // namespace hui
