#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "hui/object/widget.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

// 模型 / 视图分离的列表 widget。spec §13 P3 收尾,longclaw_jtui 用于 Sidebar
// 模块导航 / RecentActivity / DeviceLogin 列表等。
//
// IListModel:size() + changed() signal。视图只感知"有多少 item",数据由
// item_template 通过 idx 自己 capture model 取。
//
// VectorListModel<T>:基于 std::vector<T> 的常用实现,push_back / replace_all /
// clear 都自动 emit changed。
//
// ListView:接 IListModel* + item_template + item_height。监听 model.changed
// 重建 children;arrange 时按 item_height + item_spacing 竖直堆叠。
//
// v1 简化:不做 virtualization(全量渲染)。longclaw 列表都不超过几十项。要做
// 万级数据等出现真需求再扩。
//
// 配合 ScrollView 用:把 ListView 塞进 ScrollView,ListView measure 报需要的
// 总高,ScrollView 自动滚。

class IListModel {
public:
    virtual ~IListModel() = default;
    [[nodiscard]] virtual std::size_t size() const = 0;
    [[nodiscard]] virtual Signal<>& changed() = 0;
};

template <typename T>
class VectorListModel : public IListModel {
public:
    [[nodiscard]] std::size_t size() const override { return items_.size(); }
    [[nodiscard]] Signal<>& changed() override { return changed_; }

    [[nodiscard]] const T& at(std::size_t i) const { return items_[i]; }
    [[nodiscard]] const std::vector<T>& items() const noexcept { return items_; }

    void push_back(T v) {
        items_.push_back(std::move(v));
        changed_.emit();
    }
    void replace_all(std::vector<T> items) {
        items_ = std::move(items);
        changed_.emit();
    }
    void clear() {
        if (items_.empty()) return;
        items_.clear();
        changed_.emit();
    }

private:
    std::vector<T> items_{};
    Signal<> changed_{};
};

class ListView : public Widget {
public:
    using ItemTemplate = std::function<std::unique_ptr<Widget>(std::size_t idx)>;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "ListView";
    }

    void set_model(IListModel* model) {
        if (model_ == model) return;
        if (model_subscription_ != 0 && model_ != nullptr) {
            model_->changed().disconnect(model_subscription_);
            model_subscription_ = 0;
        }
        model_ = model;
        if (model_ != nullptr) {
            model_subscription_ = model_->changed().connect([this] { rebuild_items(); });
        }
        rebuild_items();
    }

    void set_item_template(ItemTemplate tmpl) {
        item_template_ = std::move(tmpl);
        rebuild_items();
    }

    void set_item_height(float h) noexcept {
        if (item_height_ == h) return;
        item_height_ = h;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }
    [[nodiscard]] float item_height() const noexcept { return item_height_; }

    void set_item_spacing(float s) noexcept {
        if (item_spacing_ == s) return;
        item_spacing_ = s;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    [[nodiscard]] std::size_t item_count() const noexcept {
        return model_ != nullptr ? model_->size() : 0U;
    }

    [[nodiscard]] SizeF measure(SizeF available) const override {
        const std::size_t n = item_count();
        if (n == 0) return SizeF{available.width, 0.0F};
        const float h = static_cast<float>(n) * item_height_
                        + (n > 1 ? static_cast<float>(n - 1) * item_spacing_ : 0.0F);
        return SizeF{available.width, h};
    }

    void arrange(RectF frame_in) override {
        set_frame(frame_in);
        // 按 model 顺序 + item_height 排列已构建的 children。
        //
        // 关键:item_template 创建的 widget 内子用相对 (0, 0) 起算的绝对 frame
        // (例如 label.set_frame(12, 10, 56, 22))。Widget::arrange 默认只 set
        // 自己的 frame,不递归 shift 子。结果是:item Panel frame 改了,但内子
        // widget 的 frame 没动,paint 仍画在 (12, 10) 等小坐标 → 与外部 widget
        // 视觉重叠(典型:Dashboard banner 区出现 Recent activity 内容残影)。
        //
        // 修法:set_frame(item) 之后递归 shift 整个子树到 target 位置(width / height
        // 用 target 的)。如果 item 是从未 arrange 过(刚 build 的初始 frame 是
        // (0, 0, 0, 0))那 dx/dy 等于 target 的 x/y,正好把所有子 widget 平移到
        // target 区域内。
        std::size_t i = 0;
        for (const auto& child : children()) {
            auto* w = dynamic_cast<Widget*>(child.get());
            if (w == nullptr) continue;
            const float y = frame_in.y + static_cast<float>(i) * (item_height_ + item_spacing_);
            const RectF target{frame_in.x, y, frame_in.width, item_height_};
            const RectF old_frame = w->frame();
            const float dx = target.x - old_frame.x;
            const float dy = target.y - old_frame.y;
            shift_subtree(*w, dx, dy);
            // 强制更新 item 自己的 width/height 到 target(shift_subtree 只动位置不动大小)
            const RectF cur = w->frame();
            w->set_frame(RectF{cur.x, cur.y, target.width, target.height});
            ++i;
        }
    }

    // Paint 自身没东西画(item 是 children,paint_widget_tree 自动递归)。
    void paint(PaintContext& /*context*/) const override {}

    ~ListView() override {
        if (model_ != nullptr && model_subscription_ != 0) {
            model_->changed().disconnect(model_subscription_);
        }
    }

private:
    // 递归把 widget 及全部 children 的 frame 平移 (dx, dy)。arrange 用。
    static void shift_subtree(Widget& w, float dx, float dy) {
        const RectF f = w.frame();
        w.set_frame(RectF{f.x + dx, f.y + dy, f.width, f.height});
        for (const auto& child : w.children()) {
            if (auto* cw = dynamic_cast<Widget*>(child.get())) {
                shift_subtree(*cw, dx, dy);
            }
        }
    }

    void rebuild_items() {
        // ─── v2 全量重建 (2026-04 改) ──────────────────────────────────────────
        //
        // 行为: 每次 model.changed 发射时, 销毁全部旧 child 子树并按当前 model->size()
        //       从 item_template 重新构建。
        //
        // 历史背景 (v1 的问题):
        //   v1 是"只追加不删除"——靠 set_visible 隐藏越界 child, 已经创建的
        //   widget tree 永远不重建。这导致 model 内容替换时 (典型: replace_all
        //   把 mock 数据换成真实数据), idx 在 [0, min(old, new)) 范围内的旧 widget
        //   仍然显示"创建那一刻 capture 的旧数据", 真实数据被吞。
        //
        //   触发场景 (longclaw 实测):
        //     UserSessionsViewModel 构造时给 1 条 mock 设备, 用户登录成功后
        //     replace_all(real_N 条), 但 ListView 的 i=0 永远显示 mock 内容
        //     ("当前设备 / 桌面端 / 刚刚"), 真实第 0 条永远不出现 → 视觉上
        //     缺一条 + 内容文案错乱。
        //
        // v2 修法:
        //   依赖 Node::clear_children (v0.2 新增的 protected API), 在每次重建前
        //   清空旧 children, 然后按 model->size() 重建全部 child。这样 widget
        //   tree 与 model 始终一致, item_template 每次都拿到最新数据。
        //
        // 性能权衡:
        //   全量重建意味着 model 每次 changed 都销毁/重建 N 个 widget。longclaw
        //   场景 (模块列表 7 项固定 / 设备列表 几台 / RecentActivity 10 条左右)
        //   没有性能问题。如果未来出现"百级以上 + 高频更新"的场景, 应当扩展为
        //   细粒度的 added / removed / replaced 信号 (IListModel::diff()), 让
        //   ListView 只重建变化的子树。
        //
        // unsafe 前提 (Node::clear_children 同款):
        //   调用方不能在 ListView::children() 上保留 raw pointer 跨过 model 变更。
        //   设计上 ListView 的 child 是 item_template 自己产物, 外部不需要 raw
        //   pointer 直接访问 child, 这条前提天然成立。
        //
        // ──────────────────────────────────────────────────────────────────────
        if (model_ == nullptr || !item_template_) {
            clear_children();
            mark_dirty(DirtyFlags::Structure | DirtyFlags::Layout | DirtyFlags::Paint);
            return;
        }

        // 全清重建: 旧 child 子树连同其 widget tree 一起被销毁。如果 item_template
        // 创建的 widget 持有外部资源 (订阅 / 文件句柄), 必须放在 widget 自己的
        // destructor 里, ListView 这一层不知道也不负责。longclaw 现役 item 模板
        // 都是纯 capture by value, 没有外部资源, 析构干净。
        clear_children();

        const std::size_t target = model_->size();
        for (std::size_t i = 0; i < target; ++i) {
            auto w = item_template_(i);
            if (w != nullptr) {
                append_child(std::move(w));
            }
        }
        mark_dirty(DirtyFlags::Structure | DirtyFlags::Layout | DirtyFlags::Paint);
    }

    IListModel* model_{nullptr};
    std::size_t model_subscription_{0};
    ItemTemplate item_template_{};
    float item_height_{40.0F};
    float item_spacing_{0.0F};
};

}  // namespace hui
