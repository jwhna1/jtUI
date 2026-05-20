#pragma once

#include "hui/foundation/geometry.hpp"
#include "hui/object/dirty_flags.hpp"
#include "hui/object/node.hpp"

namespace hui {

class Element : public Node {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Element";
    }

    [[nodiscard]] const RectF& frame() const noexcept {
        return frame_;
    }

    void set_frame(RectF frame) noexcept {
        if (frame_.x == frame.x && frame_.y == frame.y && frame_.width == frame.width &&
            frame_.height == frame.height) {
            return;
        }

        frame_ = frame;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    [[nodiscard]] bool visible() const noexcept {
        return visible_;
    }

    void set_visible(bool visible) noexcept {
        if (visible_ == visible) {
            return;
        }

        visible_ = visible;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    [[nodiscard]] DirtyFlags dirty_flags() const noexcept {
        return dirty_flags_;
    }

    // 子树（不含自己）累积的 dirty 摘要。整树扫描靠它剪枝：subtree_dirty_flags()
    // 和 dirty_flags() 都不含目标 flag 时，整个 subtree 可以跳过递归。
    [[nodiscard]] DirtyFlags subtree_dirty_flags() const noexcept {
        return subtree_dirty_;
    }

    void clear_dirty(DirtyFlags flags = DirtyFlags::All) noexcept {
        dirty_flags_ = dirty_flags_ & ~flags;
    }

    // 仅供"已经递归清完所有子"的代码使用（典型：renderer 一帧结束从 root 整树清）。
    // 如果只清自己 subtree_dirty 而子里仍有 dirty，下次 propagate 的早停判定会错。
    void clear_subtree_dirty(DirtyFlags flags = DirtyFlags::All) noexcept {
        subtree_dirty_ = subtree_dirty_ & ~flags;
    }

    // 外部（主题切换、host 应用逻辑）请求重绘。mark_dirty 是子类侵入式接口，
    // invalidate 是对外稳定入口。
    void invalidate(DirtyFlags flags = DirtyFlags::Paint) noexcept {
        mark_dirty(flags);
    }

    // 把整棵子树(含 self)的 frame 平移 (dx, dy)。jtUI Element 用绝对坐标 + Panel
    // 不自动 arrange children,host 把一个 page 的 root 在父坐标系里"位移"时若直
    // 接 set_frame 只会改自己,children 还停在原处 → 视觉错位(典型场景:caption
    // bar 之下塞 dashboard,dashboard 内部 widget 用 (0..body) 算的绝对 frame)。
    //
    // 本 API 把 (dx, dy) 累加到 self.frame 和递归到所有 Element 子的 frame,保持
    // 子之间相对位置不变。常见用法:host build_root 后整体 shift 到目标位置。
    void shift_subtree(float dx, float dy) noexcept {
        if (dx == 0.0F && dy == 0.0F) return;
        set_frame(RectF{frame_.x + dx, frame_.y + dy, frame_.width, frame_.height});
        for (const auto& child : children()) {
            if (auto* e = dynamic_cast<Element*>(child.get())) {
                e->shift_subtree(dx, dy);
            }
        }
    }

    // v1.20 (2026-05-04): 平移专用,跳过脏标记。
    //
    // shift_subtree 每个节点都走 set_frame -> mark_dirty(Layout | Paint) -> 父链
    // propagate_subtree_dirty,N=300+ 子树时一次滚轮 tick 跑 300 次脏标记冒泡,
    // 体感"鼠标硬"。translate_subtree 直接改 frame_.x/y,不动 size,不打脏标。
    //
    // 调用方负责手动 mark_dirty(Paint) 一次触发重绘。Layout 不脏 — 平移不变
    // size,子布局输入未改。仅"整树位置同步偏移"语义场景适用 (ScrollView 滚动
    // 是典型),其他 set_frame 仍走脏标记主路径。
    //
    // **动画场景必须用本 API 而非 set_frame**（v1.4 文档化）:
    // jtUI 走 dirty rect partial repaint。set_frame 自带 mark_dirty(Layout|Paint),
    // dirty rect 仅覆盖 widget **新位置** frame, 旧位置上一帧的像素不会被 D2D 清除
    // → 连续动画堆出"排骨状"残影。translate_subtree 不打脏标 + 由父级容器在自己
    // frame 上 mark_dirty(Paint) 触发一个**覆盖足够大**的 dirty rect, 整块重画
    // 自动清除旧像素。jtui_cinema CarouselAnimator 翻页动画 + ScrollView 平滑滚动
    // 都走此路径。
    //
    // 错用 set_frame 做动画的修法: 改用 translate_subtree + 把动画驱动 widget 的
    // frame 设为覆盖所有平移 target 的可视区域 + 该驱动 widget 自身 mark_dirty(Paint)。
    void translate_subtree(float dx, float dy) noexcept {
        if (dx == 0.0F && dy == 0.0F) return;
        frame_.x += dx;
        frame_.y += dy;
        for (const auto& child : children()) {
            if (auto* e = dynamic_cast<Element*>(child.get())) {
                e->translate_subtree(dx, dy);
            }
        }
    }

protected:
    void mark_dirty(DirtyFlags flags) noexcept {
        if (flags == DirtyFlags::None) {
            return;
        }
        dirty_flags_ |= flags;
        propagate_subtree_dirty(parent(), flags);
    }

    void on_child_added(Node& child) override {
        auto* elem = dynamic_cast<Element*>(&child);
        if (elem == nullptr) {
            return;
        }
        // 新挂上的子默认带 Structure dirty（Element 字段初始值），需要把 dirty 摘要
        // 冒到自身（含自己的 subtree_dirty_）。propagate 从 this 起步会先设到 this->subtree_dirty_。
        const DirtyFlags subtree = elem->dirty_flags_ | elem->subtree_dirty_;
        if (subtree == DirtyFlags::None) {
            return;
        }
        propagate_subtree_dirty(this, subtree);
    }

private:
    static void propagate_subtree_dirty(Node* node, DirtyFlags flags) noexcept {
        while (node != nullptr) {
            auto* element = dynamic_cast<Element*>(node);
            if (element != nullptr) {
                const DirtyFlags before = element->subtree_dirty_;
                element->subtree_dirty_ |= flags;
                if (before == element->subtree_dirty_) {
                    // 该祖先（及更上层）已经包含了我们要冒的全部 flag，早停。
                    return;
                }
            }
            node = node->parent();
        }
    }

    RectF frame_ {};
    bool visible_ {true};
    DirtyFlags dirty_flags_ {DirtyFlags::Structure};
    DirtyFlags subtree_dirty_ {DirtyFlags::None};
};

}  // namespace hui
