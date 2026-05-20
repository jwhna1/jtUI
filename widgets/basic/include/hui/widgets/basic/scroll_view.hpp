#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

namespace hui {

// ScrollView:把 content widget 包成可滚动 viewport。
//
// v1 模型:
//   - 单一 content(unique_ptr<Widget>),通过 set_content 接管
//   - content 在 measure 阶段报需要的 SizeF;ScrollView arrange 时把 content
//     的 frame 设为 (frame.x - off_x, frame.y - off_y, max(needed.w, frame.w),
//     max(needed.h, frame.h)),让 content 内部 children 跟着偏移
//   - clips_children = true,paint_widget_tree 会把 clip 收紧到 ScrollView frame
//     超出 viewport 的 child 在递归早停;边缘上的 child paint 命令仍会画出溢出
//     像素(v2 加 PaintContext::push_clip 才严格)
//   - on_event 处理 MouseWheel:wheel_delta 推 offset.y;Capture phase 拿到事件,
//     防止内嵌的 ScrollView 也滚动(就近优先)。键盘 PageUp/Down/Home/End 已留
//   - 画水平 / 竖直 scroll bar(track + thumb)。v1 不支持拖动 thumb,v2 候选

class ScrollView : public Widget {
public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "ScrollView";
    }

    [[nodiscard]] bool clips_children() const noexcept override {
        return true;
    }

    // v1.19 (2026-05-04): 滚动条 thumb 区域抢断 children, 让 ScrollView 成为
    // hit_test 终点 → state.pressed = self → press 期间 MouseMove 由自己接收,
    // thumb 拖动正常工作。thumb 之外仍正常下钻 children, 不破坏滚动内容交互。
    [[nodiscard]] bool intercepts_children_at(PointF p) const override {
        return point_in_v_thumb(p) || point_in_h_thumb(p);
    }

    Widget& set_content(std::unique_ptr<Widget> content) {
        // v1.2 (2026-04-28): 反复 set_content 必须先清旧 child。v1.1 的"简化"
        // 只 append 不清, 旧 content 仍挂在 children_ 里继续被 paint pass 遍历,
        // 出现 N 代旧 content panel 在 viewport 内叠加渲染的视觉错位 bug
        // (Chat 业务每次 rebuild_messages 都触发, 表现为聊天内容左漂 / 右侧重影)。
        //
        // scroll_offset_ 由调用方决定何时重置: 单纯 rebuild (例如 Chat 追加消息)
        // 不应回顶, 否则用户视野被弹回顶部、新消息掉到 viewport 之外看不见。
        // 切换会话 / 切换页签等"内容语义变了"的场景, 调用方自己 set_scroll_offset({0,0})。
        clear_children();
        Widget* raw = content.get();
        content_ptr_ = raw;
        append_child(std::move(content));
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
        return *raw;
    }

    [[nodiscard]] Widget* content() const noexcept { return content_ptr_; }

    [[nodiscard]] PointF scroll_offset() const noexcept { return scroll_offset_; }

    // 即时定位（程序设置 / thumb 拖动）：立即跳到 offset，且把平滑滚动的
    // target 同步过去，避免 tick 把位置又缓动拉回旧目标。
    void set_scroll_offset(PointF offset) noexcept {
        const PointF clamped = clamp_offset(offset);
        target_offset_ = clamped;
        apply_offset_internal(clamped);
    }

    // v1.24 (2026-05-15): 平滑滚动。滚轮事件只更新 target_offset_，由 tick
    // 每帧把 scroll_offset_ 指数逼近 target —— 一次滚轮的 40px 跳变被摊到
    // 多帧渐进，视觉上丝滑不"顿"。thumb 拖动 / 程序 set 仍即时（走
    // set_scroll_offset，target 同步，tick 立即判定到位返回 false）。
    bool tick(float delta) override {
        const float dx = target_offset_.x - scroll_offset_.x;
        const float dy = target_offset_.y - scroll_offset_.y;
        if (std::abs(dx) < 0.5F && std::abs(dy) < 0.5F) {
            if (dx != 0.0F || dy != 0.0F) {
                apply_offset_internal(target_offset_);  // 收尾对齐到整数目标
            }
            return false;
        }
        // 指数逼近：每帧补上剩余距离的一部分，先快后慢。
        constexpr float kRate = 18.0F;
        const float t = std::clamp(delta * kRate, 0.0F, 1.0F);
        apply_offset_internal(PointF{
            scroll_offset_.x + dx * t,
            scroll_offset_.y + dy * t,
        });
        return true;
    }

    [[nodiscard]] SizeF content_size() const noexcept { return content_size_; }

    void set_show_scrollbars(bool show) noexcept {
        if (show_scrollbars_ == show) return;
        show_scrollbars_ = show;
        mark_dirty(DirtyFlags::Paint);
    }

    void set_wheel_step(float step) noexcept {
        wheel_step_ = step > 0.0F ? step : 1.0F;
    }

    void arrange(RectF frame_in) override {
        set_frame(frame_in);
        if (content_ptr_ == nullptr) {
            content_size_ = SizeF{};
            return;
        }
        // content 测自己想要的大小;受限于 frame 宽度(让 ScrollView 横向不滚动
        // 时 child wrap 到合适列宽)。竖向给一个很大的可用值让 content 报真实需求。
        constexpr float kBigHeight = 1.0e9F;
        content_size_ = content_ptr_->measure(SizeF{frame_in.width, kBigHeight});
        // 至少撑满 viewport
        const float w = std::max(content_size_.width, frame_in.width);
        const float h = std::max(content_size_.height, frame_in.height);
        content_size_ = SizeF{w, h};

        // 重新 clamp offset(content size 可能变小,旧 offset 越界)
        scroll_offset_ = clamp_offset(scroll_offset_);
        target_offset_ = clamp_offset(target_offset_);

        // arrange content 到偏移后的 frame
        const RectF target_content{
            frame_in.x - scroll_offset_.x,
            frame_in.y - scroll_offset_.y,
            w,
            h,
        };

        // v1.1 (2026-04-27): jtui v1 的 Panel / 普通 Widget 的 arrange 默认只
        // set_frame 自己, 不递归 children。如果 content 是 Panel (典型: 整页
        // ScrollView 包 Panel), content->arrange 改了 content frame 但 children
        // 不跟着 → wheel 改 offset 后 children 仍画在原位置, 滚动视觉无效。
        //
        // 这里在 arrange content 之前用 shift_subtree 把整个 content 子树平移
        // (target.x - old.x, target.y - old.y), 让所有后代 widget 的 frame 跟随
        // offset 变化。等价于 ListView::arrange 内对每个 item 的 shift_subtree
        // 处理思路 — jtui v1 容器 widget 通用解法。
        const RectF old_content = content_ptr_->frame();
        const float dx = target_content.x - old_content.x;
        const float dy = target_content.y - old_content.y;
        if (dx != 0.0F || dy != 0.0F) {
            // v1.20 (2026-05-04): 滚动平移走 translate_subtree, 不打 N 次脏标记。
            // shift_subtree 每节点 set_frame -> mark_dirty(Layout|Paint) -> 父链
            // propagate, N=300+ 时一次滚轮 tick 跑 300 次脏冒泡, 体感"鼠标硬"。
            // translate_subtree 只改 frame_.x/y, ScrollView 自己 mark_dirty(Paint)
            // 一次足够触发重绘 (paint_widget_tree 一旦从 root 跑就递归全树)。
            content_ptr_->translate_subtree(dx, dy);
        }
        // translate_subtree 不动 size, 这里再让 content 自己 arrange 一次刷 size。
        // content 是 Panel/Widget 时 arrange 只 set_frame (位置已对齐, 早返回不打脏);
        // size 真改了 (window resize) 会标 Layout|Paint, 这是正确路径。
        // ListView/FlexBox 等容器 arrange 会重新布局 children — 两种情况都正确。
        content_ptr_->arrange(target_content);
    }

    bool on_event(Event& ev) override {
        EventBase& base = event_base(ev);
        if (auto* m = std::get_if<MouseEvent>(&ev)) {
            // 在 Capture 阶段拦截 MouseWheel:就近 ScrollView 优先吸收,内嵌的
            // ScrollView 不会被同一次滚轮事件触发两次。
            if (m->kind == EventKind::MouseWheel &&
                (base.phase == EventPhase::Capture || base.phase == EventPhase::Target)) {
                if (m->wheel_delta != 0.0F) {
                    // v1.24: 只推 target_offset_，tick 负责缓动逼近（平滑滚动）。
                    target_offset_ = clamp_offset(PointF{
                        target_offset_.x,
                        target_offset_.y - m->wheel_delta * wheel_step_,
                    });
                    base.handled = true;
                    return true;
                }
            }

            // v1.18 (2026-05-04): 滚动条 thumb 拖动。Capture 阶段优先于 children
            // 处理鼠标事件: 点击 / 拖动落在 thumb 区域时由 ScrollView 自己消费,
            // 不传给消息列表 (否则会触发文字选区拖动等无关交互)。
            // 落在 thumb 之外的鼠标事件不消费, 让 children 正常接收。
            if (base.phase == EventPhase::Capture || base.phase == EventPhase::Target) {
                if (m->kind == EventKind::MouseDown) {
                    if (point_in_v_thumb(m->position)) {
                        dragging_v_thumb_ = true;
                        drag_start_mouse_y_ = m->position.y;
                        drag_start_offset_y_ = scroll_offset_.y;
                        base.handled = true;
                        return true;
                    }
                    if (point_in_h_thumb(m->position)) {
                        dragging_h_thumb_ = true;
                        drag_start_mouse_x_ = m->position.x;
                        drag_start_offset_x_ = scroll_offset_.x;
                        base.handled = true;
                        return true;
                    }
                } else if (m->kind == EventKind::MouseMove) {
                    if (dragging_v_thumb_) {
                        const float track_h = frame().height - 8.0F;  // 与 paint 一致
                        const float ratio = frame().height / content_size_.height;
                        const float thumb_h = std::max(track_h * ratio, 24.0F);
                        const float drag_range = track_h - thumb_h;
                        const float scroll_range = content_size_.height - frame().height;
                        if (drag_range > 0.0F && scroll_range > 0.0F) {
                            const float dy = m->position.y - drag_start_mouse_y_;
                            const float t = dy / drag_range;
                            set_scroll_offset(PointF{
                                scroll_offset_.x,
                                drag_start_offset_y_ + t * scroll_range,
                            });
                        }
                        base.handled = true;
                        return true;
                    }
                    if (dragging_h_thumb_) {
                        const float track_w = frame().width - 8.0F;
                        const float ratio = frame().width / content_size_.width;
                        const float thumb_w = std::max(track_w * ratio, 24.0F);
                        const float drag_range = track_w - thumb_w;
                        const float scroll_range = content_size_.width - frame().width;
                        if (drag_range > 0.0F && scroll_range > 0.0F) {
                            const float dx = m->position.x - drag_start_mouse_x_;
                            const float t = dx / drag_range;
                            set_scroll_offset(PointF{
                                drag_start_offset_x_ + t * scroll_range,
                                scroll_offset_.y,
                            });
                        }
                        base.handled = true;
                        return true;
                    }
                } else if (m->kind == EventKind::MouseUp) {
                    if (dragging_v_thumb_ || dragging_h_thumb_) {
                        dragging_v_thumb_ = false;
                        dragging_h_thumb_ = false;
                        base.handled = true;
                        return true;
                    }
                }
            }
        }
        return Widget::on_event(ev);
    }

    // v1.18 (2026-05-03): 滚动条搬到 paint_overlay。
    // 旧实现把滚动条画在 paint() 里, 但 paint_widget_tree 顺序是 widget.paint
    // 先于 children — children 后画的内容会覆盖 ScrollView 自己画的滚动条!
    // (具体表现: 业务里 set_show_scrollbars(true) 也看不到滚动条) 改到
    // paint_overlay 里画, 顺序变成 widget.paint → children paint → paint_overlay
    // 让滚动条画在内容之上。
    void paint(PaintContext& /*context*/) const override {
        // ScrollView 自己不画背景 (背景由父级或 content 内部 Panel 提供)。
    }

    void paint_overlay(PaintContext& context) const override {
        if (!show_scrollbars_) return;

        // track/thumb 用 text_muted × alpha 替代 bg_raised / border_strong —
        // 旧配色在 light 主题下与卡片背景 (#fff) 撞色看不见, text_muted 灰调
        // 在 light 和 dark 主题下都能形成对比, 用户能清晰看到滚动条。
        Color track_color = theme::colors().text_muted;
        track_color.a *= 0.12F;  // 极淡, 不抢戏
        Color thumb_color = theme::colors().text_muted;
        thumb_color.a *= 0.55F;  // 中等显眼

        // 竖直 scroll bar:仅当 content 高度 > viewport 高度
        if (content_size_.height > frame().height + 0.5F) {
            constexpr float bar_w = 10.0F;
            constexpr float bar_padding = 4.0F;
            const float track_x = frame().x + frame().width - bar_w - bar_padding;
            const float track_y = frame().y + bar_padding;
            const float track_h = frame().height - bar_padding * 2.0F;
            const RectF track{track_x, track_y, bar_w, track_h};
            context.fill_rounded_rect(track, track_color, bar_w * 0.5F);

            // thumb:viewport / content 比例,按 offset 决定位置
            const float ratio = frame().height / content_size_.height;
            const float thumb_h = std::max(track_h * ratio, 24.0F);
            const float scroll_range = content_size_.height - frame().height;
            const float scroll_t = scroll_range > 0.0F
                                   ? scroll_offset_.y / scroll_range
                                   : 0.0F;
            const float thumb_y = track_y + (track_h - thumb_h) * scroll_t;
            const RectF thumb{track_x, thumb_y, bar_w, thumb_h};
            context.fill_rounded_rect(thumb, thumb_color, bar_w * 0.5F);
        }

        // 水平 scroll bar:仅当 content 宽度 > viewport 宽度
        if (content_size_.width > frame().width + 0.5F) {
            constexpr float bar_h = 10.0F;
            constexpr float bar_padding = 4.0F;
            const float track_x = frame().x + bar_padding;
            const float track_y = frame().y + frame().height - bar_h - bar_padding;
            const float track_w = frame().width - bar_padding * 2.0F;
            const RectF track{track_x, track_y, track_w, bar_h};
            context.fill_rounded_rect(track, track_color, bar_h * 0.5F);

            const float ratio = frame().width / content_size_.width;
            const float thumb_w = std::max(track_w * ratio, 24.0F);
            const float scroll_range = content_size_.width - frame().width;
            const float scroll_t = scroll_range > 0.0F
                                   ? scroll_offset_.x / scroll_range
                                   : 0.0F;
            const float thumb_x = track_x + (track_w - thumb_w) * scroll_t;
            const RectF thumb{thumb_x, track_y, thumb_w, bar_h};
            context.fill_rounded_rect(thumb, thumb_color, bar_h * 0.5F);
        }
    }

private:
    // 只改 scroll_offset_ 本身（不碰 target_offset_）+ 重排 + 标脏。
    // set_scroll_offset 和 tick 都经它落地。
    void apply_offset_internal(PointF clamped) noexcept {
        if (scroll_offset_.x == clamped.x && scroll_offset_.y == clamped.y) return;
        scroll_offset_ = clamped;
        // jtui Application 的 layout pass 只对 root 调一次 arrange，默认
        // Widget::arrange 不递归子 → 这里直接 arrange(self) 走 ScrollView::
        // arrange → content 平移链路，让滚动立即生效。
        arrange(frame());
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] PointF clamp_offset(PointF off) const noexcept {
        const float max_x = std::max(content_size_.width - frame().width, 0.0F);
        const float max_y = std::max(content_size_.height - frame().height, 0.0F);
        return PointF{
            std::clamp(off.x, 0.0F, max_x),
            std::clamp(off.y, 0.0F, max_y),
        };
    }

    // v1.18: 命中判定 — 与 paint_overlay 内画 thumb 的几何严格一致, 让用户
    // 视觉看到的 thumb 矩形可被精确点击 / 拖动。
    [[nodiscard]] bool point_in_v_thumb(PointF p) const noexcept {
        if (!show_scrollbars_) return false;
        if (content_size_.height <= frame().height + 0.5F) return false;
        constexpr float bar_w = 10.0F;
        constexpr float bar_padding = 4.0F;
        const float track_x = frame().x + frame().width - bar_w - bar_padding;
        const float track_y = frame().y + bar_padding;
        const float track_h = frame().height - bar_padding * 2.0F;
        const float ratio = frame().height / content_size_.height;
        const float thumb_h = std::max(track_h * ratio, 24.0F);
        const float scroll_range = content_size_.height - frame().height;
        const float scroll_t = scroll_range > 0.0F ? scroll_offset_.y / scroll_range : 0.0F;
        const float thumb_y = track_y + (track_h - thumb_h) * scroll_t;
        // 命中区域略大于视觉 thumb (左右 +4px), 让点击更宽容; 上下严格 thumb 范围。
        return p.x >= track_x - 4.0F && p.x <= track_x + bar_w + 4.0F
            && p.y >= thumb_y && p.y <= thumb_y + thumb_h;
    }
    [[nodiscard]] bool point_in_h_thumb(PointF p) const noexcept {
        if (!show_scrollbars_) return false;
        if (content_size_.width <= frame().width + 0.5F) return false;
        constexpr float bar_h = 10.0F;
        constexpr float bar_padding = 4.0F;
        const float track_x = frame().x + bar_padding;
        const float track_y = frame().y + frame().height - bar_h - bar_padding;
        const float track_w = frame().width - bar_padding * 2.0F;
        const float ratio = frame().width / content_size_.width;
        const float thumb_w = std::max(track_w * ratio, 24.0F);
        const float scroll_range = content_size_.width - frame().width;
        const float scroll_t = scroll_range > 0.0F ? scroll_offset_.x / scroll_range : 0.0F;
        const float thumb_x = track_x + (track_w - thumb_w) * scroll_t;
        return p.y >= track_y - 4.0F && p.y <= track_y + bar_h + 4.0F
            && p.x >= thumb_x && p.x <= thumb_x + thumb_w;
    }

    Widget* content_ptr_{nullptr};
    PointF scroll_offset_{0.0F, 0.0F};
    // v1.24 (2026-05-15): 平滑滚动目标。滚轮推它，tick 把 scroll_offset_ 缓动
    // 逼近它。即时定位（set_scroll_offset）会让两者同步。
    PointF target_offset_{0.0F, 0.0F};
    SizeF content_size_{0.0F, 0.0F};
    bool show_scrollbars_{true};
    float wheel_step_{40.0F};  // 一次滚轮转动 40px(典型 wheel_delta = 1.0)

    // v1.18 (2026-05-04): thumb 拖动状态。MouseDown 落 thumb 上记录起始 mouse 位置 +
    // 起始 scroll_offset; MouseMove 期间按位移比例 (drag_range / scroll_range)
    // 算新 offset; MouseUp 清。竖直 / 水平互斥 (一次只能拖一个方向)。
    bool dragging_v_thumb_{false};
    bool dragging_h_thumb_{false};
    float drag_start_mouse_x_{0.0F};
    float drag_start_mouse_y_{0.0F};
    float drag_start_offset_x_{0.0F};
    float drag_start_offset_y_{0.0F};
};

}  // namespace hui
