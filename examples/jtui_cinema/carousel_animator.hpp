#pragma once

// jtui_cinema carousel 平移动画驱动器 —— invisible widget。
//
// 设计：build 时把所有需要被翻页 X 平移的 widget raw ptr 注册进 targets_，
// tick 时按 progress (0..1) over duration 用 ease-in-out-cubic 计算当前位置，
// 把增量 dx 累加到 targets_ 内每个 widget 的 frame.x。
//
// 为什么是 progress + easing 而不是弹簧式（指数逼近）：
//   - jtUI Application 在 LBUTTONUP 末尾立即调一次 tick(1/60) 来消除按钮反馈
//     延迟。对弹簧式动画，第一帧 step = diff * k 与 diff 成比例 —— 翻页距离
//     760px 时第一帧瞬间推进 72px，看起来是"先跳一下再滑"。
//   - progress + ease-in-out 第一帧 eased(0.04) ≈ 0.00026 → 推进 0.2px，
//     视觉从静止柔和加速（人眼能跟住），中段最快，结尾减速吸合。这才是
//     "滑动"的体感。
//
// 跨 rebuild 保活：每次 rebuild 新建 animator，通过 set_offsets(current, target)
// 注入当前实时位置 + 目标位置。current_offset_ 在 tick 中持续通过 callback 写
// 回 state，所以新 animator 接续旧的动画进度不跳跃。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include "jtui/jtui.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

namespace jtui_cinema {

// ease-in-out cubic: 起步柔 → 中段快 → 收尾柔。
// t<0.5  → 4t³
// t>=0.5 → 1 - (-2t+2)³/2
[[nodiscard]] inline float ease_in_out_cubic(float t) noexcept {
    t = std::clamp(t, 0.0F, 1.0F);
    if (t < 0.5F) {
        return 4.0F * t * t * t;
    }
    const float p = -2.0F * t + 2.0F;
    return 1.0F - p * p * p * 0.5F;
}

class CarouselAnimator : public hui::Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "CarouselAnimator";
    }

    void add_target(hui::Element* w) {
        if (w != nullptr) targets_.push_back(w);
    }

    // build_root 完成后由业务侧调用注入持久化状态。
    //   current: 当前实际偏移（state.carousel_offset，跨 rebuild 由本类回写）
    //   target:  翻页目标（state.carousel_target，翻页按钮回调改写）
    // 如果 target != current，启动 ease-in-out 滑动从 current → target。
    void set_offsets(float current, float target) noexcept {
        current_offset_ = current;
        if (std::abs(target - current) > kEps) {
            start_offset_  = current;
            target_offset_ = target;
            progress_      = 0.0F;
            animating_     = true;
            mark_dirty(hui::DirtyFlags::Paint);
        } else {
            start_offset_  = target;
            target_offset_ = target;
            progress_      = 1.0F;
            animating_     = false;
        }
    }

    // 运行中改 target（例如动画途中再次点翻页）：从当前位置起新一段动画。
    void slide_to(float new_target) noexcept {
        if (std::abs(new_target - current_offset_) <= kEps) {
            target_offset_ = new_target;
            return;
        }
        start_offset_  = current_offset_;
        target_offset_ = new_target;
        progress_      = 0.0F;
        animating_     = true;
        mark_dirty(hui::DirtyFlags::Paint);
        if (on_target_changed_) on_target_changed_(target_offset_);
    }

    // 动画时长（秒）。默认 0.8s 慢柔感（用户偏好）。
    // 0.45 snappy / 0.6 中性 / 0.8 慢柔 / 1.0+ 拖泥带水
    void set_duration(float seconds) noexcept {
        duration_ = std::clamp(seconds, 0.05F, 5.0F);
    }

    void set_on_current_changed(std::function<void(float)> cb) {
        on_current_changed_ = std::move(cb);
    }
    void set_on_target_changed(std::function<void(float)> cb) {
        on_target_changed_ = std::move(cb);
    }

    [[nodiscard]] float current_offset() const noexcept { return current_offset_; }
    [[nodiscard]] float target_offset()  const noexcept { return target_offset_;  }
    [[nodiscard]] bool  animating()      const noexcept { return animating_;      }

    bool tick(float dt) override {
        if (!animating_) return false;

        progress_ += dt / duration_;
        if (progress_ >= 1.0F) {
            const float dx = target_offset_ - current_offset_;
            shift_targets(-dx);   // 卡片朝相反方向平移（offset 正 = 卡片左移）
            current_offset_ = target_offset_;
            progress_       = 1.0F;
            animating_      = false;
            if (on_current_changed_) on_current_changed_(current_offset_);
            mark_dirty(hui::DirtyFlags::Paint);
            return false;
        }

        const float eased       = ease_in_out_cubic(progress_);
        const float new_current = start_offset_ + (target_offset_ - start_offset_) * eased;
        const float dx          = new_current - current_offset_;
        if (dx != 0.0F) {
            shift_targets(-dx);
            current_offset_ = new_current;
            if (on_current_changed_) on_current_changed_(current_offset_);
        }
        mark_dirty(hui::DirtyFlags::Paint);
        return true;
    }

    void paint(hui::PaintContext& /*ctx*/) const override {}

private:
    // 平移每个 target 的 frame —— 用 translate_subtree 而非 set_frame：
    //   - set_frame 自带 mark_dirty(Layout|Paint)，dirty rect = 新位置 frame，
    //     旧位置上一帧的像素不会被清 → 滑动时拖出残影（用户实拍：Taiwan 卡片
    //     右侧多道竖向卡片轮廓"分身"）。
    //   - translate_subtree 只改 frame_.x 不打脏标，由 animator 自身 mark_dirty
    //     一次（frame 覆盖整 reel 区域），让父接收的 dirty rect 覆盖所有 target
    //     的新旧位置并集 → 整块重画，旧像素自然被擦。
    // 这套模式直接对齐 widgets/basic/src/scroll_view.cpp 的 translate_subtree +
    // 父级单次 mark_dirty(Paint) 策略。
    void shift_targets(float dx) noexcept {
        if (dx == 0.0F) return;
        for (auto* w : targets_) {
            if (w != nullptr) w->translate_subtree(dx, 0.0F);
        }
    }

    static constexpr float kEps = 0.5F;

    std::vector<hui::Element*>  targets_;
    float                       start_offset_   {0.0F};
    float                       current_offset_ {0.0F};
    float                       target_offset_  {0.0F};
    float                       progress_       {1.0F};   // 1.0 = 静止
    float                       duration_       {0.80F};  // 秒
    bool                        animating_      {false};
    std::function<void(float)>  on_current_changed_;
    std::function<void(float)>  on_target_changed_;
};

}  // namespace jtui_cinema
