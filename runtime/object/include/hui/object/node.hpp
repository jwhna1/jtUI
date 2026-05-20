#pragma once

#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace hui {

class Node {
public:
    using ChildPtr = std::unique_ptr<Node>;
    using Children = std::vector<ChildPtr>;

    Node() = default;
    virtual ~Node() = default;

    Node(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(const Node&) = delete;
    Node& operator=(Node&&) = delete;

    [[nodiscard]] virtual std::string_view type_name() const noexcept {
        return "Node";
    }

    [[nodiscard]] Node* parent() const noexcept {
        return parent_;
    }

    [[nodiscard]] const Children& children() const noexcept {
        return children_;
    }

    [[nodiscard]] std::size_t child_count() const noexcept {
        return children_.size();
    }

    Node& append_child(ChildPtr child) {
        if (!child) {
            throw std::invalid_argument("append_child requires a valid node");
        }

        child->parent_ = this;
        children_.push_back(std::move(child));
        on_child_added(*children_.back());
        return *children_.back();
    }

protected:
    virtual void on_child_added(Node&) {
    }

    // ─── clear_children (v0.2 新增, 2026-04 引入) ─────────────────────────────
    //
    // 销毁全部子节点。protected 接口, 仅派生类可调; 应用层不需要直接访问.
    //
    // 历史背景: v0.1 的 Node 只暴露 append_child, ListView 等容器派生类只能
    //   "追加不删除", 表现为 model.size 缩小或内容替换时, widget tree 仍保留
    //   旧 child (典型 bug: UserSessionsViewModel 构造给 1 条 mock 设备, 登录
    //   成功后 replace_all 真实 N 条, ListView 只追加 N-1 个新 child, mock
    //   永远占在 i=0, 真实第 0 条丢失)。详见 widgets/basic/list_view.hpp。
    //
    // unsafe 调用前提:
    //   1. 调用方必须确保**没有外部 raw pointer 持有**任何 child 子树。
    //      典型场景: host 在 build_root 里 raw_ptr = w.get() 后 append_child,
    //      这种 raw_ptr 不能跨过本 API 使用。ListView 内部用 item_template 创建
    //      的 widget 创建后即交给 ListView 管理, 没有外部 raw_ptr → 安全。
    //   2. 派生类如果在 child 上挂了外部订阅/回调, 需在自己析构链路里处理。
    //      Node::clear_children 只负责销毁 children_ vector, 不知道高层订阅。
    //
    // 推荐配套调用 (Element 派生类):
    //   clear_children();
    //   mark_dirty(DirtyFlags::Structure | DirtyFlags::Layout | DirtyFlags::Paint);
    //
    // API 演进路线 (供 jtui 后续迭代参考):
    //   v0.2 (本次): 提供 clear_children() — 全清重建语义
    //   v0.3 (规划): 提供 erase_child(idx) / erase_children(range) — 局部删除
    //   v0.4 (规划): 提供 replace_children(vector) — 批量替换 (一次 dirty)
    //
    // 已知调用方:
    //   - widgets/basic/list_view.hpp::rebuild_items (v2 全量重建, 取代 v1 的
    //     "只追加不删除" trade-off)
    void clear_children() noexcept {
        children_.clear();
    }

    // ─── release_child (v0.3 新增, 2026-05-15) ─────────────────────────────
    //
    // 按 raw pointer 把某个子节点的 ownership 解还给调用方。从 children_ 中移除
    // 并返回 unique_ptr；找不到（target 不是本 node 直接子节点）返回 nullptr。
    //
    // 使用场景：跨整树 rebuild 持久化重型 widget。jtui_studio 切换主题/i18n 时
    // 整树 rebuild，但 VideoPlayer / AudioPlayer / WaveformView 持有 decoder +
    // WASAPI output + GPU texture，销毁/重建非常重（既会打断正在播放的媒体，
    // 又造成切换卡顿）。业务侧：rebuild 前用 release_child 把这几个 widget 的
    // ownership 取回，build 新 root 时 append 进新树，旧 root 析构时已无持久
    // widget 在内。详见 examples/jtui_studio。
    //
    // 注意：
    //   1. 只检查直接子节点；子树深处的不会冒泡上来，调用方需在 widget 的直接
    //      父 panel 上调用，或 walk tree 自己找到 parent。
    //   2. 调用后 target 仍然合法（unique_ptr 没销毁，被返回出去），但 parent_
    //      指针被清空。
    //   3. 调用方接管 ownership 后必须自己保管或重新 append_child；否则返回值
    //      离开作用域时 widget 被析构。
    [[nodiscard]] ChildPtr release_child(const Node* target) noexcept {
        if (target == nullptr) return nullptr;
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->get() == target) {
                ChildPtr taken = std::move(*it);
                children_.erase(it);
                taken->parent_ = nullptr;
                return taken;
            }
        }
        return nullptr;
    }

private:
    Node* parent_ {nullptr};
    Children children_ {};
};

}  // namespace hui
