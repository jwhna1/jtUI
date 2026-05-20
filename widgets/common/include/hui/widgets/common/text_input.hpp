#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/property.hpp"
#include "hui/render/paint_context.hpp"

// jtUI TextInput
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

enum class TextInputState {
    Default,
    Error,
    Success,
};

class TextInput : public Widget {
  public:
    TextInput() = default;

    explicit TextInput(std::string placeholder) : placeholder_(std::move(placeholder)) {}

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "TextInput";
    }

    [[nodiscard]] const std::string& placeholder() const noexcept {
        return placeholder_;
    }

    void set_placeholder(std::string placeholder);

    [[nodiscard]] const std::string& text() const noexcept {
        return text_.get();
    }

    void set_text(std::string text);

    void append_char(char value);

    void backspace();

    [[nodiscard]] Property<std::string>& text_property() noexcept {
        return text_;
    }

    // 字段外的显示：label（字段上方）、helper_text（字段下方）。
    // helper_text 在 error 态自动变红；Default 态走 theme.text_muted。
    void set_label(std::string label);
    void set_helper_text(std::string helper);

    // 前缀/后缀：一般是图标字符（🔍 / ✕ / 眼睛 / 日历），也可以是短文本（"@"、"/kg"）。
    void set_leading(std::string content);
    void set_trailing(std::string content);

    void set_state(TextInputState state) noexcept;

    // 密码态：字符显示为 '•'。
    void set_password(bool password) noexcept;

    // v1.13 (2026-04-29): 无边框模式 — 不画 widget 自身的 fill / stroke 背板。
    // 业务场景: ChatPage 输入复合卡片 (CardSurface 已提供整体边框 + 圆角), 让
    // TextInput 透明嵌入, 视觉上整个卡片是一个整体, 焦点用 cursor 闪烁表达。
    // 不影响文字 / 选区 / cursor / 滚动条渲染。
    void set_borderless(bool borderless) noexcept;

    // v1.15 (2026-05-03): 回车键提交回调。设置后, 单击 Enter (无 shift / ctrl)
    // 会调 cb 而不是插入换行 — 用于"按 Enter 提交表单"这种场景 (登录页 /
    // 搜索框 / 单行输入)。未设置时 Enter 不消费, 事件继续上抛 (multi-line
    // 业务想插入 \n 自己接 KeyEvent 处理)。Shift+Enter 不触发, 留给业务做
    // "Shift+Enter 换行 / Enter 提交"对照。
    using SubmitCallback = std::function<void()>;
    void set_on_submit(SubmitCallback cb) { on_submit_ = std::move(cb); }

    [[nodiscard]] bool accepts_focus() const noexcept override {
        return enabled();
    }

    // v1.21 (2026-05-04): hover 时显示工字光标 (IBeam), 让用户一眼知道这是
    // 文本输入区。disabled 状态下退化回箭头, 视觉上提示"不可交互"。
    [[nodiscard]] CursorKind cursor_kind() const noexcept override {
        return enabled() ? CursorKind::IBeam : CursorKind::Default;
    }

    // 光标和选区。selection_anchor_ == cursor_pos_ 时无选区；不等时为选区两端
    // （min/max 由两者排序得出）。所有索引以 std::string byte index 计算（v1 仅 ASCII）。
    [[nodiscard]] std::size_t cursor_pos() const noexcept {
        return cursor_pos_;
    }
    [[nodiscard]] std::size_t selection_anchor() const noexcept {
        return selection_anchor_;
    }
    [[nodiscard]] bool has_selection() const noexcept {
        return selection_anchor_ != cursor_pos_;
    }
    [[nodiscard]] std::size_t selection_min() const noexcept {
        return selection_anchor_ < cursor_pos_ ? selection_anchor_ : cursor_pos_;
    }
    [[nodiscard]] std::size_t selection_max() const noexcept {
        return selection_anchor_ > cursor_pos_ ? selection_anchor_ : cursor_pos_;
    }

    // 直接挪光标。extend_selection = true（典型场景：Shift+方向键）保留 anchor，
    // 仅移 cursor_pos_，从而扩展或收缩选区。false（普通方向键）让 anchor 跟着走，
    // 选区被清空。
    void set_cursor_pos(std::size_t pos, bool extend_selection = false) noexcept;

    bool on_text_input(char value) override;

    bool on_key_down(std::int32_t key_code) override;

    bool on_event(Event& ev) override;

    void paint(PaintContext& context) const override;

  private:
    // 在 cursor 处插入字符（若有选区则先删选区）。on_text_input / on_event 里的 TextInput
    // 共用这条路径，确保选区语义一致。
    void insert_char_at_cursor(char value);
    // v1.2 (2026-04-28): 宽字符插入. cp 按 UTF-8 编码 (1-4 字节) 写入 text_,
    // cursor 推进对应 byte 数. 给 IME / 中日韩输入法用的入口, 与 ASCII 路径
    // (insert_char_at_cursor) 共用 has_selection / replace_text 流程.
    void insert_codepoint_at_cursor(char32_t code_point);
    // 删除当前选区（必须 has_selection）。
    void delete_selection();
    // 内部 setter：替换 text 同时把 cursor / anchor 都设到 new_cursor。
    void replace_text(std::string new_text, std::size_t new_cursor);
    // 处理 KeyDown 的核心，旧 on_key_down(key) 走 shift=ctrl=false 的分支。
    // v1.3 (2026-04-28) 加 ctrl 参数, 处理 Ctrl+A 全选 / Ctrl+C 复制 / Ctrl+V
    // 粘贴 / Ctrl+X 剪切。剪贴板访问内嵌 Win32 (CF_UNICODETEXT) 在 .cpp 内,
    // 业务层不需要注入。
    bool handle_key_down(std::int32_t key_code, bool shift, bool ctrl);

    std::string placeholder_{"Type here"};
    std::string label_{};
    std::string helper_{};
    std::string leading_{};
    std::string trailing_{};
    Property<std::string> text_{};
    std::size_t cursor_pos_{0};
    std::size_t selection_anchor_{0};
    TextInputState state_{TextInputState::Default};
    bool password_{false};
    bool borderless_{false};

    // v1.15 (2026-05-03): Enter 提交回调, 见 set_on_submit。空 std::function 视作
    // "未设置", handle_key_down 内 Enter 会 return false 让事件继续上抛。
    SubmitCallback on_submit_{};

    // v1.3 (2026-04-28): 鼠标交互状态。
    //   mouse_dragging_: 左键按下后保持 true, 期间 MouseMove 更新 cursor_pos
    //                    扩选区 (anchor 不动), 抬起置 false。
    //   ctx_menu_open_: 右键弹出的复制/剪切/粘贴菜单, anchor = 鼠标点位 (clamp
    //                   到 widget frame 内确保 hit_test 命中)。
    bool        mouse_dragging_ {false};
    bool        ctx_menu_open_ {false};
    PointF      ctx_menu_anchor_ {};

    // v1.7 (2026-04-28): 多行文本内部纵向滚动状态。
    //   scroll_offset_lines_: 已隐藏在视野顶部的行数。paint 时所有 y 计算都减
    //                          (offset * kTextLineH), byte_pos_from_xy 接收 mouse y
    //                          也加回 offset 转回逻辑行号。
    // 行内 cursor follow + 滚轮 + clip 都基于此偏移。set_text / replace_text
    // 时 cursor 跳到 beginning/end, paint 内会自动调整 offset 让 cursor 可见。
    // mutable: paint() 是 const 但要在画前调整 offset 让 cursor follow。
    mutable std::size_t scroll_offset_lines_ {0};
    // v1.8 (2026-04-28): cursor follow 触发节流。仅 cursor_pos_ 与上次 paint 时
    // 不同才把 offset 拉回 cursor 行。否则鼠标 wheel 改 offset 后下一帧 paint 又
    // 会被 follow 撤销, 文字弹回原处, 用户感觉"鼠标很硬, 滚不动"。
    mutable std::size_t last_cursor_for_follow_ {static_cast<std::size_t>(-1)};

    // 把 widget-绝对坐标的 x 转成 text() 里的 byte offset (落在 codepoint 边界
    // 上). 几何与 paint 一致 (kCharAdvance * byte 数 + leading slot)。
    // v1.6 (2026-04-28): 升级为 (x, y) 二维 — 多行 text 时先按 y 找行号,
    // 再在该行内按 x 找列 byte offset。旧的 byte_pos_from_x 仍保留作 thin wrapper
    // (内部转 (x, frame.y + kTextTopPad) 即仅匹配第一行), 让单行 widget caller 不破坏。
    [[nodiscard]] std::size_t byte_pos_from_xy(PointF widget_pt) const;
    [[nodiscard]] std::size_t byte_pos_from_x(float widget_x) const;

    // 右键菜单矩形 (绝对坐标), 三行 "复制 / 剪切 / 粘贴" 垂直堆叠。
    // clamp 到 widget frame 内, hit_test 默认 frame.contains 仍能命中菜单 click。
    [[nodiscard]] RectF context_menu_rect() const noexcept;
};

} // namespace hui
