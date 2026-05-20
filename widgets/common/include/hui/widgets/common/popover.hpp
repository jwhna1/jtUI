#pragma once

#include <memory>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// Popover：含浮层内容的小弹层。spec §13 提到的常规 widget 之一。
//
// 与 Dialog 的区别：
//   - Popover 不模态，不全屏 scrim；外部 click 可由调用方手动关闭（auto-dismiss
//     这一层 v1 不内置，因为需要听全局 click，要 root 协助才优雅）
//   - 没有 confirm 按钮等业务结构；content 是任意 widget（unique_ptr<Widget>）
//   - frame 由调用方 set_frame 设置；定位（基于 anchor 算 offset）由调用方处理
//
// 典型用法：菜单项弹出、日期选择浮层、tooltip 进阶版。
//
// v1 行为：
//   - open == false：visible() 不变但 hit_test 返回 false（事件穿透）+ paint 跳过
//   - open == true：画背景 + 边框 + content；hit_test 仅命中自己 frame；content 通过
//     append_child 挂到自己作为 widget 子树参与 paint / hit_test 链
//   - on_open_changed signal：状态改变时 emit（用于关联 trigger 的视觉切换）

class Popover : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Popover";
    }

    [[nodiscard]] bool open() const noexcept { return open_; }

    void set_open(bool o) noexcept {
        if (open_ == o) return;
        open_ = o;
        mark_dirty(DirtyFlags::Paint);
        open_changed_.emit(open_);
    }

    void toggle() noexcept { set_open(!open_); }

    // content 接管所有权并 append 为 child，让 paint_widget_tree 自动递归画。
    // 重复 set_content 会替换之前的 content（旧 content 析构）。
    void set_content(std::unique_ptr<Widget> content) {
        // v1 简化：单 content 槽。Node 的 children 不暴露 erase API，所以用
        // content_ 自己持有，避免和 Node 的 children 重复管理。paint() 里手动
        // 调 content_->paint。
        content_ = std::move(content);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] Widget* content() const noexcept { return content_.get(); }

    [[nodiscard]] Signal<bool>& on_open_changed() noexcept { return open_changed_; }

    [[nodiscard]] bool hit_test(PointF point) const noexcept override {
        if (!visible() || !open_) return false;
        return frame().contains(point);
    }

    void paint(PaintContext& context) const override {
        if (!open_) return;
        const auto& c = theme::colors();
        const float r = theme::radius().lg;
        // v1.22 (2026-05-15): 升级到真高斯模糊阴影（D2D 1.1 CLSID_D2D1Shadow）。
        // popover 走 level_3 elevation（dropdown 语义），把以前那条"画一圈暗矩形
        // 当软阴影"的硬 hack 收编掉。
        const theme::Shadow& sh = theme::elevation().level_3;
        context.fill_shadow(frame(), r,
                            PointF{sh.offset_x, sh.offset_y},
                            sh.blur, sh.spread, sh.color);
        context.fill_rounded_rect(frame(), c.bg_raised, r);
        context.stroke_rounded_rect(frame(), c.border, r, 1.0F);

        // content 由 paint() 内部递归画（v1 简化 — 不通过 children_ 走 paint_widget_tree
        // 的 z-order 链）。content frame 假定调用方已经按 popover frame 安排好。
        if (content_) {
            content_->paint(context);
        }
    }

private:
    bool open_{false};
    std::unique_ptr<Widget> content_{};
    Signal<bool> open_changed_{};
};

}  // namespace hui
