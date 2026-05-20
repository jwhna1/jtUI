#include "hui/widgets/common/text_input.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "hui/app/application.hpp"  // v1.10 (2026-04-29): Application::measure_text_width 真实字宽测量

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "hui/theme/theme.hpp"

// jtUI TextInput 实现
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

namespace {

// v1 用单字节宽度估算（Latin / 半角 ASCII 的近似）。font_size = 13pt 在 96 DPI 下,
// DirectWrite Segoe UI 实测平均字符宽 5-5.5px(数字+符号偏窄,字母偏宽)。
// 估算偏小 → 光标可能落在文字内部 1-2px(几乎看不出);估算偏大 → 光标远离
// 文字末尾(几个字符宽,视觉很难看)。两害取偏小:5.0F。
// 真精确算需要接 IDWriteTextLayout::GetMetrics 暴露 measureText API,
// 当前 v1 简化用估算。
constexpr float kCharAdvance = 5.0F;

// v1.5 (2026-04-28): 按 UTF-8 字符类型分宽度估算 (替代 kCharAdvance * byte 数)。
// 旧算法只按字节数算 → CJK (3 byte) * 5 = 15px 与真实 13-14px 偏差小, 但 ASCII
// (1 byte) * 5 = 5px 与真实 ~7-8px 偏差大, 选 10 个字母时高亮少 ~30px。
// 新估算分类:
//   - ASCII (1 byte):     ~7.0px  (Segoe UI 13pt 字母平均)
//   - 2-byte (Latin ext): ~7.0px
//   - 3-byte (CJK):       ~14.0px (Microsoft YaHei 13pt 等宽汉字)
//   - 4-byte (emoji):     ~16.0px
// 选区高亮 / 鼠标位置反推 / 密码态 placeholder 光标都共用这套。
inline float glyph_advance_for_leading(std::uint8_t lead) noexcept {
    if (lead < 0x80U)               return 7.0F;
    if ((lead & 0xE0U) == 0xC0U)    return 7.0F;
    if ((lead & 0xF0U) == 0xE0U)    return 14.0F;
    return 16.0F;
}

// 累加 [lo, hi) 子串的 advance (UTF-8 边界对齐由调用方保证 — cursor_pos_ 约定
// 永远落在 codepoint 边界, 所以 lo / hi 也是边界)。
inline float glyph_advance_range(const std::string& s, std::size_t lo, std::size_t hi) noexcept {
    if (hi > s.size()) hi = s.size();
    if (lo >= hi) return 0.0F;
    float w = 0.0F;
    std::size_t i = lo;
    while (i < hi) {
        const auto u = static_cast<std::uint8_t>(s[i]);
        w += glyph_advance_for_leading(u);
        // 跳到下一个 codepoint 起始
        std::size_t next = i + 1;
        while (next < s.size() && (static_cast<std::uint8_t>(s[next]) & 0xC0U) == 0x80U) {
            ++next;
        }
        i = next;
    }
    return w;
}

// v1.6 (2026-04-28): 多行支持 — \n 拆分文字成 LineSpan 列表。
// 每段 [start, end), end 不含 \n 自身。空文本返回单段 [0, 0)。
struct LineSpan { std::size_t start; std::size_t end; };

inline std::vector<LineSpan> split_lines_utf8(const std::string& s) {
    std::vector<LineSpan> out;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\n') {
            out.push_back({line_start, i});
            line_start = i + 1;
        }
    }
    out.push_back({line_start, s.size()});
    return out;
}

// 给定 byte_idx (text 中的 byte offset) 找它落在哪一行 + 该行起始 byte。
// byte_idx 必须在 [0, s.size()] 内 (cursor_pos_ 约定)。返回的 line_index 总是
// 有效 ([0, lines.size())); col_byte = byte_idx - lines[line_index].start。
struct LinePos { std::size_t line_index; std::size_t col_byte; };

inline LinePos line_pos_for_byte(const std::vector<LineSpan>& lines, std::size_t byte_idx) {
    for (std::size_t i = 0; i < lines.size(); ++i) {
        // 注意 byte_idx 等于 lines[i].end 时仍属本行 (\n 后才进下一行); 但如果 i
        // 不是最后一行, byte_idx == end 意味着光标在 \n 处, 视觉上仍属本行末尾。
        if (byte_idx <= lines[i].end) {
            return {i, byte_idx - lines[i].start};
        }
    }
    return {lines.empty() ? 0U : lines.size() - 1U, 0U};
}

// Windows 虚拟键码（避免 include windows.h 在跨平台代码里）
constexpr std::int32_t kVkBack = 0x08;
constexpr std::int32_t kVkReturn = 0x0D;
constexpr std::int32_t kVkLeft = 0x25;
constexpr std::int32_t kVkRight = 0x27;
constexpr std::int32_t kVkHome = 0x24;
constexpr std::int32_t kVkEnd = 0x23;
constexpr std::int32_t kVkDelete = 0x2E;

// v1.3 (2026-04-28): 内嵌 Win32 剪贴板访问 forward decl, 定义在文件后部
// (handle_key_down 之上). on_event MouseDown 也要用这两个函数, 所以这里 forward
// 让前面 (on_event 体内) 引用合法。同 translation unit 的 anonymous namespace
// 共享符号, forward decl + 后置 definition 配对生效。
bool ti_clipboard_set(const std::string& utf8);
std::string ti_clipboard_get();

// v1.2 (2026-04-28): UTF-8 codepoint 边界辅助。
// 旧版 backspace / delete / Left / Right 按 1 字节挪光标 / 删除, 中文 (3 字节) /
// emoji (4 字节) 被切碎产生乱码 (典型表现: 输入框出现 "¦Ⅱ ←㏑∧¥" 之类乱码)。
// 这两个函数在 cursor 位置反向 / 正向找最近的 codepoint 起始字节, 保证操作粒度
// 始终是完整 codepoint。约定 cursor_pos_ 始终在 codepoint 边界上 (本类内部所有
// 修改 cursor_pos_ 的地方都走这两个函数 / 走 set_text 末尾)。
inline std::size_t prev_utf8_start(const std::string& s, std::size_t pos) noexcept {
    if (pos == 0 || pos > s.size()) return 0;
    std::size_t i = pos - 1;
    // utf-8 continuation byte: 10xxxxxx; 反向跳过最多 3 个 (utf-8 最长 4 字节)。
    while (i > 0 && (static_cast<std::uint8_t>(s[i]) & 0xC0U) == 0x80U) {
        --i;
    }
    return i;
}

inline std::size_t next_utf8_start(const std::string& s, std::size_t pos) noexcept {
    if (pos >= s.size()) return s.size();
    // 跳过 leading byte, 再跳过所有 continuation bytes。
    std::size_t i = pos + 1;
    while (i < s.size() && (static_cast<std::uint8_t>(s[i]) & 0xC0U) == 0x80U) {
        ++i;
    }
    return i;
}

// v1.14 (2026-05-03): 密码态 mask 等长替换 helper。
// 把原文 [start..end) 子串里的 codepoint 数, 整段替换成 N 个 '•' (UTF-8
// \xe2\x80\xa2 三字节)。用于密码模式下 measure_text_width 的输入归一化:
// paint 阶段画的是 mask 字符, 但 cursor x / hit-test x 之前用原文 measure,
// 明文宽度 ≠ mask 宽度 (英文 ~7px / 中文 ~14px / 数字 ~7px vs '•' ~6.5px),
// 导致光标停在视觉错位处。改用 mask 化后的子串 measure, 视觉严格对齐。
inline std::string mask_substring(const std::string& src,
                                    std::size_t start, std::size_t end) {
    std::size_t cp = 0;
    std::size_t i = start;
    while (i < end) {
        i = next_utf8_start(src, i);
        ++cp;
    }
    std::string out;
    out.reserve(cp * 3);
    for (std::size_t k = 0; k < cp; ++k) {
        out += "\xe2\x80\xa2";
    }
    return out;
}

} // namespace

void TextInput::set_placeholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_text(std::string text) {
    if (text_.set(std::move(text))) {
        // 业务方 set_text 通常是初始化或重置：把光标放末尾，清选区。
        cursor_pos_ = text_.get().size();
        selection_anchor_ = cursor_pos_;
        mark_dirty(DirtyFlags::Paint);
    }
}

void TextInput::append_char(char value) {
    insert_char_at_cursor(value);
}

void TextInput::backspace() {
    if (has_selection()) {
        delete_selection();
        return;
    }
    if (cursor_pos_ == 0) {
        return;
    }
    std::string next = text();
    const std::size_t prev = prev_utf8_start(next, cursor_pos_);
    next.erase(prev, cursor_pos_ - prev);
    replace_text(std::move(next), prev);
}

void TextInput::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_helper_text(std::string helper) {
    helper_ = std::move(helper);
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_leading(std::string content) {
    leading_ = std::move(content);
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_trailing(std::string content) {
    trailing_ = std::move(content);
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_state(TextInputState state) noexcept {
    if (state_ == state) return;
    state_ = state;
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_password(bool password) noexcept {
    if (password_ == password) return;
    password_ = password;
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_borderless(bool borderless) noexcept {
    if (borderless_ == borderless) return;
    borderless_ = borderless;
    mark_dirty(DirtyFlags::Paint);
}

void TextInput::set_cursor_pos(std::size_t pos, bool extend_selection) noexcept {
    const std::size_t clamped = std::min(pos, text().size());
    if (cursor_pos_ == clamped && (extend_selection || selection_anchor_ == cursor_pos_)) {
        return;
    }
    cursor_pos_ = clamped;
    if (!extend_selection) {
        selection_anchor_ = cursor_pos_;
    }
    mark_dirty(DirtyFlags::Paint);
}

bool TextInput::on_text_input(char value) {
    if (!enabled() || value < 32 || value >= 127) {
        return false;
    }

    insert_char_at_cursor(value);
    return true;
}

bool TextInput::on_key_down(std::int32_t key_code) {
    return handle_key_down(key_code, /*shift=*/false, /*ctrl=*/false);
}

bool TextInput::on_event(Event& ev) {
    EventBase& base = event_base(ev);
    if (base.phase != EventPhase::Target) {
        return Widget::on_event(ev);
    }

    // v1.3 (2026-04-28): 鼠标交互 — 单击放光标 / 拖动扩选区 / 右键弹菜单。
    if (auto* mouse = std::get_if<MouseEvent>(&ev)) {
        if (!enabled()) return false;
        switch (mouse->kind) {
        case EventKind::MouseDown: {
            if (mouse->button == MouseButton::Right) {
                // 右键: toggle 菜单
                ctx_menu_open_ = !ctx_menu_open_;
                ctx_menu_anchor_ = mouse->position;
                mark_dirty(DirtyFlags::Paint);
                base.handled = true;
                return true;
            }
            // 左键: 先检菜单
            if (ctx_menu_open_) {
                const RectF menu = context_menu_rect();
                if (mouse->position.x >= menu.x &&
                    mouse->position.x <= menu.x + menu.width &&
                    mouse->position.y >= menu.y &&
                    mouse->position.y <= menu.y + menu.height) {
                    // 命中菜单 — 按 x 落入哪一列决定动作 (3 列: 复制/剪切/粘贴)
                    const float col_w = menu.width / 3.0F;
                    const int col = static_cast<int>((mouse->position.x - menu.x) / col_w);
                    if (col == 0 && has_selection()) {
                        // 复制
                        const auto lo = selection_min();
                        const auto hi = selection_max();
                        ti_clipboard_set(text().substr(lo, hi - lo));
                    } else if (col == 1 && has_selection()) {
                        // 剪切
                        const auto lo = selection_min();
                        const auto hi = selection_max();
                        ti_clipboard_set(text().substr(lo, hi - lo));
                        delete_selection();
                    } else if (col == 2) {
                        // 粘贴
                        std::string clip = ti_clipboard_get();
                        std::string filtered;
                        filtered.reserve(clip.size());
                        for (char ch : clip) {
                            const auto u = static_cast<unsigned char>(ch);
                            if (u == '\r') continue;
                            if (u < 32 && u != '\n' && u != '\t') continue;
                            filtered.push_back(ch);
                        }
                        if (has_selection()) delete_selection();
                        std::string next = text();
                        next.insert(cursor_pos_, filtered);
                        replace_text(std::move(next), cursor_pos_ + filtered.size());
                    }
                    ctx_menu_open_ = false;
                    mark_dirty(DirtyFlags::Paint);
                    base.handled = true;
                    return true;
                }
                ctx_menu_open_ = false;
                mark_dirty(DirtyFlags::Paint);
                // fall through 走单击放光标
            }
            // 单击 → 放光标 + 起选 anchor
            const std::size_t pos = byte_pos_from_xy(mouse->position);
            cursor_pos_ = pos;
            selection_anchor_ = pos;
            mouse_dragging_ = true;
            mark_dirty(DirtyFlags::Paint);
            base.handled = true;
            return true;
        }
        case EventKind::MouseMove: {
            if (mouse_dragging_) {
                const std::size_t pos = byte_pos_from_xy(mouse->position);
                cursor_pos_ = std::min(pos, text().size());
                // anchor 不动 — 扩选区
                mark_dirty(DirtyFlags::Paint);
                base.handled = true;
                return true;
            }
            return false;
        }
        case EventKind::MouseUp: {
            if (mouse->button == MouseButton::Left && mouse_dragging_) {
                mouse_dragging_ = false;
                base.handled = true;
                return true;
            }
            return false;
        }
        case EventKind::MouseWheel: {
            // v1.7 (2026-04-28): 多行内部纵向滚动。
            // v1.11 (2026-04-29): line height 动态。
            const auto& s = text();
            if (s.empty()) return false;
            const auto lines = split_lines_utf8(s);
            const float kTextLineH  = Application::measure_line_height(13.0F, false);
            constexpr float kTextTopPad = 12.0F;
            const float available_h = std::max(frame().height - kTextTopPad - 8.0F,
                                                  kTextLineH);
            const std::size_t visible_lines = std::max<std::size_t>(
                static_cast<std::size_t>(available_h / kTextLineH), 1U);
            if (lines.size() <= visible_lines) return false;
            // v1.8: 步长加到 5 让用户感觉滚动顺滑 (旧 3 行偏小, 大 wheel 时
            // 多次 dispatch 才能滚一屏)。
            const int step = (mouse->wheel_delta > 0.0F) ? -5 : 5;
            int next = static_cast<int>(scroll_offset_lines_) + step;
            if (next < 0) next = 0;
            const int max_offset = static_cast<int>(lines.size()) -
                                    static_cast<int>(visible_lines);
            if (next > max_offset) next = max_offset;
            scroll_offset_lines_ = static_cast<std::size_t>(next);
            mark_dirty(DirtyFlags::Paint);
            base.handled = true;
            return true;
        }
        default:
            return false;
        }
    }

    // KeyDown：从 KeyEvent 读 shift / ctrl 位决定行为。走 handle_key_down 绕开
    // 旧 on_key_down(key) 这条丢失 modifier 的路径。
    if (auto* key = std::get_if<KeyEvent>(&ev)) {
        if (key->kind == EventKind::KeyDown && enabled()) {
            const bool consumed = handle_key_down(key->key_code, key->shift, key->ctrl);
            if (consumed) {
                base.handled = true;
            }
            return consumed;
        }
        return false;
    }

    // 字符输入：默认 dispatch 已经能调 on_text_input，但这里我们要自己处理"先删
    // 选区再插入"的语义，避免 on_text_input 被调时 cursor 状态错位。
    // v1.2 (2026-04-28): 不再限制 codepoint < 127, 中文/日文/韩文 (BMP 内) 都
    // 走 insert_codepoint_at_cursor (UTF-8 编码后插入). 控制字符 (< 32) 仍跳过.
    if (auto* text = std::get_if<TextInputEvent>(&ev)) {
        if (enabled() && text->code_point >= 32U) {
            insert_codepoint_at_cursor(text->code_point);
            base.handled = true;
            return true;
        }
        return false;
    }

    return Widget::on_event(ev);
}

void TextInput::insert_char_at_cursor(char value) {
    if (has_selection()) {
        delete_selection();
    }
    std::string next = text();
    next.insert(next.begin() + static_cast<std::ptrdiff_t>(cursor_pos_), value);
    replace_text(std::move(next), cursor_pos_ + 1U);
}

void TextInput::insert_codepoint_at_cursor(char32_t cp) {
    if (has_selection()) {
        delete_selection();
    }
    // UTF-8 编码 (1-4 字节). codepoint 来自 WM_CHAR 已经是 BMP 内, 一般 1-3
    // 字节就够; 留 4 字节路径给后续 surrogate pair 解析.
    char buf[4];
    int len = 0;
    if (cp < 0x80U) {
        buf[0] = static_cast<char>(cp);
        len = 1;
    } else if (cp < 0x800U) {
        buf[0] = static_cast<char>(0xC0U | (cp >> 6));
        buf[1] = static_cast<char>(0x80U | (cp & 0x3FU));
        len = 2;
    } else if (cp < 0x10000U) {
        buf[0] = static_cast<char>(0xE0U | (cp >> 12));
        buf[1] = static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        buf[2] = static_cast<char>(0x80U | (cp & 0x3FU));
        len = 3;
    } else {
        buf[0] = static_cast<char>(0xF0U | (cp >> 18));
        buf[1] = static_cast<char>(0x80U | ((cp >> 12) & 0x3FU));
        buf[2] = static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        buf[3] = static_cast<char>(0x80U | (cp & 0x3FU));
        len = 4;
    }
    std::string next = text();
    next.insert(cursor_pos_, buf, static_cast<std::size_t>(len));
    replace_text(std::move(next), cursor_pos_ + static_cast<std::size_t>(len));
}

void TextInput::delete_selection() {
    const std::size_t lo = selection_min();
    const std::size_t hi = selection_max();
    if (lo == hi) {
        return;
    }
    std::string next = text();
    next.erase(lo, hi - lo);
    replace_text(std::move(next), lo);
}

void TextInput::replace_text(std::string new_text, std::size_t new_cursor) {
    const bool changed = text_.set(std::move(new_text));
    cursor_pos_ = std::min(new_cursor, text_.get().size());
    selection_anchor_ = cursor_pos_;
    if (changed) {
        mark_dirty(DirtyFlags::Paint);
    } else {
        // text 没变但光标可能动过（例如 Home/End），仍要刷一次 paint。
        mark_dirty(DirtyFlags::Paint);
    }
}

// v1.3 (2026-04-28): 内嵌 Win32 剪贴板访问, 给 Ctrl+C / Ctrl+V / Ctrl+X 用。
// jtui widget 直接依赖 win32 不优雅, 但我们已经 build 在 Win 路径; 后续 jtui
// macOS / Linux 后端启用时再抽出 hui::clipboard provider 接口。
namespace {
#ifdef _WIN32

bool ti_clipboard_set(const std::string& utf8) {
    BOOL opened = FALSE;
    for (int attempt = 0; attempt < 3; ++attempt) {
        opened = OpenClipboard(nullptr);
        if (opened) break;
        Sleep(50);
    }
    if (!opened) return false;
    EmptyClipboard();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                     static_cast<int>(utf8.size()), nullptr, 0);
    if (wlen < 0) wlen = 0;
    const std::size_t bytes = (static_cast<std::size_t>(wlen) + 1) * sizeof(wchar_t);
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hg == nullptr) { CloseClipboard(); return false; }
    auto* dst = static_cast<wchar_t*>(GlobalLock(hg));
    if (dst == nullptr) { GlobalFree(hg); CloseClipboard(); return false; }
    if (wlen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                              static_cast<int>(utf8.size()), dst, wlen);
    }
    dst[wlen] = L'\0';
    GlobalUnlock(hg);
    if (SetClipboardData(CF_UNICODETEXT, hg) == nullptr) {
        GlobalFree(hg); CloseClipboard(); return false;
    }
    CloseClipboard();
    return true;
}

std::string ti_clipboard_get() {
    BOOL opened = FALSE;
    for (int attempt = 0; attempt < 3; ++attempt) {
        opened = OpenClipboard(nullptr);
        if (opened) break;
        Sleep(50);
    }
    if (!opened) return {};
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h == nullptr) { CloseClipboard(); return {}; }
    const wchar_t* w = static_cast<const wchar_t*>(GlobalLock(h));
    if (w == nullptr) { CloseClipboard(); return {}; }
    std::size_t wlen = 0;
    while (w[wlen] != L'\0') ++wlen;
    int blen = WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(wlen),
                                     nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(blen), '\0');
    if (blen > 0) {
        WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(wlen),
                              out.data(), blen, nullptr, nullptr);
    }
    GlobalUnlock(h);
    CloseClipboard();
    return out;
}

#else
bool ti_clipboard_set(const std::string&) { return false; }
std::string ti_clipboard_get() { return {}; }
#endif
}  // namespace

std::size_t TextInput::byte_pos_from_x(float widget_x) const {
    // 旧 API thin wrapper - 仅匹配第一行 (单行 widget caller 旧调用兼容)。
    return byte_pos_from_xy(PointF{widget_x, frame().y + 14.0F});
}

std::size_t TextInput::byte_pos_from_xy(PointF widget_pt) const {
    // v1.10 (2026-04-29): 鼠标 hit-test 改用 Application::measure_text_width 真实
    // DirectWrite 字宽测量, 替代 v1.5/v1.6 的 glyph_advance estimate。
    // 旧 estimate 把 ASCII 全按 7px 算, 实际 'm'=10 / 'i'=4 / ' '=3.5 / 标点 3-5
    // 差异大 → 长文本累积偏差几十像素 → 鼠标点击 server 后 mapped 到 weather_
    // 后, 输入文字插错位置。
    //
    // 实现: 先用 next_utf8_start 收集行内所有 codepoint 边界, 再二分 +
    // measure_text_width 找到累积宽度最接近 dx 的边界 (snap 到最近)。
    // 单次 byte_pos_from_xy 调用 = O(log N) 次 measure (每次 measure 单独
    // CreateTextLayout + GetMetrics, ~100us), 视野行最多 200 字符 ≈ 8 次 ≈ 1ms,
    // 鼠标拖选每帧能跑 60fps。
    const float padding   = 12.0F;
    const float slot_w    = 24.0F;
    constexpr float kTextTopPad = 12.0F;
    // v1.11 (2026-04-29): 动态 line height 跟 DirectWrite 真实测量, 替代 hardcode
    // 22。13pt Segoe UI / Microsoft YaHei 实测 line_h ≈ 17-18, 旧 22 跟 PARAGRAPH_
    // ALIGNMENT_CENTER 累积让 selection / cursor 偏移文字。
    const float kTextLineH = Application::measure_line_height(13.0F, false);

    const bool has_leading = !leading_.empty();
    const float text_left  = frame().x + padding + (has_leading ? slot_w + 6.0F : 0.0F);

    const auto& s = text();
    if (s.empty()) return 0;

    const auto lines = split_lines_utf8(s);

    const float dy = widget_pt.y - (frame().y + kTextTopPad);
    int li = static_cast<int>(dy / kTextLineH) + static_cast<int>(scroll_offset_lines_);
    if (li < 0) li = 0;
    if (li >= static_cast<int>(lines.size())) li = static_cast<int>(lines.size()) - 1;
    const auto& span = lines[static_cast<std::size_t>(li)];

    const float dx = widget_pt.x - text_left;
    if (dx <= 0.0F) return span.start;

    // 收集 codepoint 边界
    std::vector<std::size_t> bounds;
    bounds.reserve(32);
    bounds.push_back(span.start);
    {
        std::size_t i = span.start;
        while (i < span.end) {
            i = next_utf8_start(s, i);
            bounds.push_back(std::min(i, span.end));
        }
    }
    if (bounds.size() < 2) return span.start;

    // v1.14: 密码态 mask 化后再 measure, 让 hit-test 落点严格对齐 mask 字符
    // 视觉位置 (与 paint cursor 同款修复)。
    auto measure_for_pos = [this, &s](std::size_t a, std::size_t b) -> std::string {
        return password_ ? mask_substring(s, a, b)
                         : s.substr(a, b - a);
    };

    // 整行宽度: 超过则直接行末
    const float total_w = Application::measure_text_width(
        measure_for_pos(span.start, span.end), 13.0F, false);
    if (dx >= total_w) return span.end;

    // 二分找最大 idx 使 measure(text[span.start, bounds[idx]]) <= dx
    std::size_t lo = 0;
    std::size_t hi = bounds.size() - 1;
    while (lo + 1 < hi) {
        const std::size_t mid = (lo + hi) / 2;
        const float w = Application::measure_text_width(
            measure_for_pos(span.start, bounds[mid]), 13.0F, false);
        if (w <= dx) lo = mid;
        else hi = mid;
    }
    // snap 到 lo / hi 最近的那个
    const float lo_w = Application::measure_text_width(
        measure_for_pos(span.start, bounds[lo]), 13.0F, false);
    const float hi_w = Application::measure_text_width(
        measure_for_pos(span.start, bounds[hi]), 13.0F, false);
    return (dx - lo_w < hi_w - dx) ? bounds[lo] : bounds[hi];
}

RectF TextInput::context_menu_rect() const noexcept {
    // v1.3 fix (2026-04-28): 横向 3 列 (复制 / 剪切 / 粘贴), 高度 28px ≤ TextInput
    // 一行 38px, 严格 clamp 在 widget frame 内, 让默认 hit_test (frame.contains)
    // 直接命中, 不被父级兄弟 widget (status_text / send button) paint 盖住。
    constexpr float MW = 192.0F;  // 3 * 64
    constexpr float MH = 28.0F;
    const auto f = frame();
    // x: 鼠标点位居中, clamp 到 frame 内
    float x = ctx_menu_anchor_.x - MW * 0.5F;
    if (x + MW > f.x + f.width)  x = f.x + f.width  - MW;
    if (x < f.x) x = f.x;
    // y: 优先弹 anchor 上方; 上方不够 fallback 在 frame 顶部 / 下方
    float y = ctx_menu_anchor_.y - MH - 4.0F;
    if (y < f.y) y = f.y;
    if (y + MH > f.y + f.height) y = f.y + f.height - MH;
    return RectF{x, y, MW, MH};
}

bool TextInput::handle_key_down(std::int32_t key_code, bool shift, bool ctrl) {
    if (!enabled()) {
        return false;
    }

    // v1.3 (2026-04-28): Ctrl 组合键 — 全选 / 复制 / 粘贴 / 剪切。Ctrl+C/X 在
    // 没选区时不消费事件 (让父级 / 全局快捷键有机会处理); Ctrl+A 即使空文本也
    // 消费 (语义清晰: 全选 = 把光标 + anchor 设到边界)。Ctrl+V 把剪贴板里的
    // 文本作为 UTF-8 直接当用户输入插入 (会先删选区), 与 IME / WM_CHAR 路径
    // 共用 replace_text 流程, 状态机一致。
    if (ctrl) {
        switch (key_code) {
        case 'A':
        case 'a': {
            const std::size_t end = text().size();
            selection_anchor_ = 0;
            cursor_pos_ = end;
            mark_dirty(DirtyFlags::Paint);
            return true;
        }
        case 'C':
        case 'c': {
            if (!has_selection()) return false;
            const std::size_t lo = selection_min();
            const std::size_t hi = selection_max();
            ti_clipboard_set(text().substr(lo, hi - lo));
            return true;
        }
        case 'X':
        case 'x': {
            if (!has_selection()) return false;
            const std::size_t lo = selection_min();
            const std::size_t hi = selection_max();
            ti_clipboard_set(text().substr(lo, hi - lo));
            delete_selection();
            return true;
        }
        case 'V':
        case 'v': {
            std::string clip = ti_clipboard_get();
            if (clip.empty()) return true;  // 仍消费, 防止 V 当字符插
            // 过滤掉控制字符 (除了换行 \n / 制表 \t), 防 \r 之类污染
            std::string filtered;
            filtered.reserve(clip.size());
            for (char c : clip) {
                const auto u = static_cast<unsigned char>(c);
                if (u == '\r') continue;
                if (u < 32 && u != '\n' && u != '\t') continue;
                filtered.push_back(c);
            }
            if (has_selection()) delete_selection();
            std::string next = text();
            next.insert(cursor_pos_, filtered);
            replace_text(std::move(next), cursor_pos_ + filtered.size());
            return true;
        }
        default:
            break;
        }
    }

    switch (key_code) {
    case kVkReturn:
        // v1.15 (2026-05-03): 单击 Enter (无 shift / ctrl) 触发 on_submit_。
        // Shift+Enter 留给业务做"换行"语义 (这里直接 return false 不消费,
        // 上层 / TextInputEvent 路径会插入 \n)。未设置 on_submit_ 时也 return
        // false, multi-line 业务用法不被破坏。
        if (!shift && !ctrl && on_submit_) {
            on_submit_();
            return true;
        }
        return false;

    case kVkBack:
        if (has_selection()) {
            delete_selection();
            return true;
        }
        if (cursor_pos_ == 0) {
            return false;
        }
        backspace();
        return true;

    case kVkDelete:
        if (has_selection()) {
            delete_selection();
            return true;
        }
        if (cursor_pos_ >= text().size()) {
            return false;
        }
        {
            std::string next = text();
            const std::size_t nxt = next_utf8_start(next, cursor_pos_);
            next.erase(cursor_pos_, nxt - cursor_pos_);
            replace_text(std::move(next), cursor_pos_);
        }
        return true;

    case kVkLeft: {
        const std::size_t target = prev_utf8_start(text(), cursor_pos_);
        set_cursor_pos(target, shift);
        return true;
    }

    case kVkRight: {
        const std::size_t target = next_utf8_start(text(), cursor_pos_);
        set_cursor_pos(target, shift);
        return true;
    }

    case kVkHome:
        set_cursor_pos(0, shift);
        return true;

    case kVkEnd:
        set_cursor_pos(text().size(), shift);
        return true;

    default:
        return false;
    }
}

void TextInput::paint(PaintContext& context) const {
    const auto& c = theme::colors();

    // 字段框体：根据 state 选边框色，先于 focus 态。
    Color border_default = c.field_border;
    Color border_focus = c.field_border_focus;
    Color helper_color = c.text_muted;
    switch (state_) {
    case TextInputState::Default: break;
    case TextInputState::Error:
        border_default = c.danger;
        border_focus = c.danger_hover;
        helper_color = c.danger;
        break;
    case TextInputState::Success:
        border_default = c.success;
        border_focus = c.success;
        helper_color = c.success;
        break;
    }

    // 主字段区域：如果有 label/helper，字段区只占中间一段 height。
    const float label_h = label_.empty() ? 0.0F : 18.0F;
    const float helper_h = helper_.empty() ? 0.0F : 16.0F;
    const RectF field{
        frame().x,
        frame().y + label_h,
        frame().width,
        frame().height - label_h - helper_h,
    };

    if (!label_.empty()) {
        const Color label_color = enabled() ? c.text_secondary : c.text_disabled;
        context.draw_text(RectF{frame().x, frame().y, frame().width, label_h},
                          label_, label_color, TextAlignment::Leading, 12.0F, true);
    }

    const Color border = focused() ? border_focus : border_default;
    const Color background = focused() ? c.field_bg_active
                                       : (hovered() ? c.field_bg_hover : c.field_bg);

    // v1.13 (2026-04-29): borderless 模式跳过 widget 自身的 fill + stroke。业务
    // 场景由父级 (CardSurface 等) 提供整体外观, 让 widget 透明嵌入, 焦点用 cursor
    // 闪烁表达。仅影响背板, 文字 / 选区 / cursor / 滚动条等照常画。
    if (!borderless_) {
        context.fill_rounded_rect(field, background, theme::radius().md);
        context.stroke_rounded_rect(field, border, theme::radius().md, 1.0F);
    }

    // leading/trailing icon 槽各占 24px，文本区居中夹在中间。
    const float slot_w = 24.0F;
    const float padding = 12.0F;
    const bool has_leading = !leading_.empty();
    const bool has_trailing = !trailing_.empty();

    if (has_leading) {
        context.draw_text(
            RectF{field.x + padding, field.y, slot_w, field.height},
            leading_, c.text_muted, TextAlignment::Center, 13.0F, false);
    }
    if (has_trailing) {
        context.draw_text(
            RectF{field.x + field.width - padding - slot_w, field.y, slot_w, field.height},
            trailing_, c.text_muted, TextAlignment::Center, 13.0F, false);
    }

    const float text_left = field.x + padding + (has_leading ? slot_w + 6.0F : 0.0F);
    const float text_right = field.x + field.width - padding
                           - (has_trailing ? slot_w + 6.0F : 0.0F);
    // v1.5 (2026-04-28): 文本与光标顶部对齐, 不依赖 field.height 居中。让大 frame
    // 高度 (>= 60px, 现代 LLM 输入卡片视觉占位) 时, 文字 / cursor / selection 仍
    // 紧贴顶部, 不在垂直方向漂浮。
    // v1.6 (2026-04-28): 多行支持 — text_rect 高度按 \n 拆分后的行数算, 让
    // DirectWrite 自动按 \n 拆行渲染 (NO_WRAP 不影响 \n 拆行)。粘贴含换行的
    // 多行文本, 不再压成一行被 D2D 内部 clip 看不到尾部。
    constexpr float kTextTopPad = 12.0F;
    // v1.11 (2026-04-29): 动态 line height 跟 DirectWrite 真实测量, 替代 hardcode
    // 22。13pt Segoe UI / Microsoft YaHei 实测 line_h ≈ 17-18, 旧 22 跟 PARAGRAPH_
    // ALIGNMENT_CENTER 累积让 selection / cursor 偏移文字。
    const float kTextLineH = Application::measure_line_height(13.0F, false);
    const std::string& display_for_lines = text().empty() ? placeholder_ : text();
    const auto preview_lines = split_lines_utf8(display_for_lines);
    const std::size_t line_count = std::max<std::size_t>(preview_lines.size(), 1);
    const float available_h = std::max(field.height - kTextTopPad - 8.0F, kTextLineH);

    // v1.7 cursor follow / scroll clamp; v1.8 (2026-04-28) 性能优化:
    // 只渲染视野内 [scroll_offset_lines_, end_line) 行的子串, 不再把整段长文本
    // 喂 DirectWrite 测量 + 排版。粘贴上千字时 paint 时间 50ms+ → 1-2ms。
    // cursor 也改自己 estimate 位置 draw_line, 省掉每帧 substr×2 大拷贝。
    const std::size_t visible_lines = std::max<std::size_t>(
        static_cast<std::size_t>(available_h / kTextLineH), 1U);
    // v1.8 关键修复 (2026-04-28): cursor follow 仅在 cursor_pos_ 真变化时触发。
    // 旧实现每帧 paint 都做 follow 检测, 鼠标 wheel 改 offset 后的下一帧 paint
    // 把 offset 拉回 cursor 行, wheel 滚动被立刻撤销 — 用户感觉"鼠标很硬, 滚不动,
    // 卡顿"。改为仅在 cursor_pos_ 变化 (键盘移动 / 输入 / 删除 / 拖选) 时拉回。
    if (!text().empty() && cursor_pos_ != last_cursor_for_follow_) {
        const auto cur_lp = line_pos_for_byte(preview_lines, cursor_pos_);
        if (cur_lp.line_index < scroll_offset_lines_) {
            scroll_offset_lines_ = cur_lp.line_index;
        }
        if (cur_lp.line_index >= scroll_offset_lines_ + visible_lines) {
            scroll_offset_lines_ = cur_lp.line_index - visible_lines + 1;
        }
    }
    last_cursor_for_follow_ = cursor_pos_;
    if (line_count > visible_lines) {
        const std::size_t max_offset = line_count - visible_lines;
        if (scroll_offset_lines_ > max_offset) scroll_offset_lines_ = max_offset;
    } else {
        scroll_offset_lines_ = 0;
    }

    // 视野范围内的 byte 区段
    const std::size_t end_line = std::min(scroll_offset_lines_ + visible_lines, line_count);
    const std::size_t vis_byte_start = (scroll_offset_lines_ < preview_lines.size())
        ? preview_lines[scroll_offset_lines_].start : 0;
    const std::size_t vis_byte_end = (end_line > 0 && end_line <= preview_lines.size())
        ? preview_lines[end_line - 1].end : 0;
    const std::size_t render_lines_count = (end_line > scroll_offset_lines_)
        ? (end_line - scroll_offset_lines_) : 1;

    // v1.19 (2026-05-14): 单行模式下让文字垂直居中 field —— 旧实现固定
    // visible_text_top = field.y + kTextTopPad (=12px)，在矮输入框（如 SearchInput
    // 36px 高）里文字会"贴底"。单行场景没有 cursor 跨行需求，居中视觉更对。
    // 多行保持顶对齐（cursor / selection 的 line_offset 算法依赖顶 anchor）。
    const float visible_text_top = (line_count == 1)
        ? field.y + (field.height - kTextLineH) * 0.5F
        : field.y + kTextTopPad;
    // v1.9 修视觉错位 (2026-04-29): text_rect.height 用 render_lines_count * kTextLineH
    // (实际渲染行数), 不再用 visible_lines * kTextLineH (容量上限)。
    // 旧实现内 box 比内容高时, DirectWrite 的 PARAGRAPH_ALIGNMENT_CENTER 把多行
    // 文字垂直居中到 box, 选区高亮按顶部对齐算 y → 高亮跟文字差几像素 (用户看
    // 到的"下面色块多, 上面色块少")。新实现 box 高度 = 内容高度, paragraph
    // center == top, selection y 和文字行 y 完全一致。
    const RectF text_rect{text_left, visible_text_top,
                            text_right - text_left,
                            static_cast<float>(render_lines_count) * kTextLineH};

    // push clip 防 selection 边缘越界
    const RectF clip_rect{field.x + 1.0F, field.y + kTextTopPad - 2.0F,
                            field.width - 2.0F,
                            available_h + 4.0F};
    context.push_clip(clip_rect);

    // 选区高亮：focused + has_selection 时画 accent_soft 矩形覆盖选区字符背景。
    // v1.5: 改用 glyph_advance_range 按 UTF-8 字符类型估算宽度。
    // v1.6: 多行选区 — 跨多行时每行独立画一段矩形 (头行 lo→行末; 中间行整行;
    //        末行 行首→hi)。同款 split_lines_utf8 拆行 + line_pos_for_byte 找
    //        起始 / 终止行号。
    // v1.8: selection 只画与视野相交的那几行, 用 preview_lines (上面已切) 而不
    // 是再 split_lines_utf8 一次。
    // v1.10 (2026-04-29): lo_x / hi_x 改用 Application::measure_text_width 真实测量,
    // 与 paint draw_text 的字宽完全一致, 选区高亮范围与文字位置精确对齐 (旧 estimate
    // 偏差导致高亮位置略偏)。
    if (focused() && has_selection() && !text().empty()) {
        const auto lo_pos = line_pos_for_byte(preview_lines, selection_min());
        const auto hi_pos = line_pos_for_byte(preview_lines, selection_max());
        const std::size_t draw_lo = std::max(lo_pos.line_index, scroll_offset_lines_);
        const std::size_t draw_hi = std::min(hi_pos.line_index + 1U, end_line);  // exclusive
        for (std::size_t li = draw_lo; li < draw_hi; ++li) {
            const auto& span = preview_lines[li];
            const std::size_t lo_b = (li == lo_pos.line_index) ? selection_min() : span.start;
            const std::size_t hi_b = (li == hi_pos.line_index) ? selection_max() : span.end;
            const float lo_x = text_left + Application::measure_text_width(
                text().substr(span.start, lo_b - span.start), 13.0F, false);
            const float hi_x = text_left + Application::measure_text_width(
                text().substr(span.start, hi_b - span.start), 13.0F, false);
            const float clamped_hi = std::min(hi_x, text_right);
            if (clamped_hi > lo_x) {
                // v1.9 (2026-04-29): selection 高度严格 kTextLineH, y 与文字行
                // 顶部对齐 (text_rect 已不被 PARAGRAPH_CENTER 偏移, 见上方修复)。
                // 旧实现 +-2 / +4 让高亮上下溢出文字行, 现在让它与文字行 1:1 包裹。
                const float y = visible_text_top
                              + static_cast<float>(li - scroll_offset_lines_) * kTextLineH;
                context.fill_rect(
                    RectF{lo_x, y, clamped_hi - lo_x, kTextLineH},
                    c.accent_soft);
            }
        }
    }

    // v1.11 (2026-04-29): per-line draw_text 替代单次整段渲染。
    // 旧实现单次 draw_text(text_rect 含整 N 行) 让 DirectWrite 自动按 \n 拆行,
    // 但 PARAGRAPH_ALIGNMENT_CENTER 把 paragraph 居中到 box, DirectWrite 内部
    // line spacing 与我们 measure_line_height 微差累积导致 selection / cursor
    // 与文字行位置漂移。
    // 新实现: 每行单独调 draw_text(line_rect 高 = 1 line_h), CENTER 在 1 行 box
    // 内等同顶部对齐, 每行文字 visual y = line_rect.y, 与 selection / cursor 用
    // 的 visible_text_top + (li - offset) * kTextLineH 完全一致。
    // 性能: 视野 4-5 行 → 每帧 4-5 次 draw_text, IDWriteTextFormat cached, 无影响。
    const Color text_color = text().empty() ? c.text_muted : c.text_primary;

    if (text().empty()) {
        // placeholder 单行
        context.draw_text(text_rect, placeholder_, text_color,
                          TextAlignment::Leading, 13.0F);
    } else if (password_) {
        std::string masked;
        masked.reserve((vis_byte_end - vis_byte_start) * 3);
        for (std::size_t i = vis_byte_start; i < vis_byte_end; ++i) {
            masked += "\xe2\x80\xa2";
        }
        // 密码态简化: 整段 mask 当一行画 (实际密码不应有 \n)
        context.draw_text(text_rect, masked, text_color,
                          TextAlignment::Leading, 13.0F);
    } else {
        for (std::size_t li = scroll_offset_lines_; li < end_line; ++li) {
            const auto& span = preview_lines[li];
            std::string line_text(text(), span.start, span.end - span.start);
            const float ly = visible_text_top
                + static_cast<float>(li - scroll_offset_lines_) * kTextLineH;
            const RectF line_rect{text_left, ly,
                                    text_right - text_left, kTextLineH};
            context.draw_text(line_rect, line_text, text_color,
                              TextAlignment::Leading, 13.0F);
        }
    }

    // cursor 自画 line, x 用真实 measure 算 (与 byte_pos_from_xy 同款), 仅在视野内画。
    if (focused()) {
        if (text().empty()) {
            context.line(PointF{text_left, visible_text_top},
                         PointF{text_left, visible_text_top + kTextLineH},
                         c.text_primary, 1.0F);
        } else {
            const auto cur_lp = line_pos_for_byte(preview_lines, cursor_pos_);
            if (cur_lp.line_index >= scroll_offset_lines_ &&
                cur_lp.line_index < end_line) {
                const auto& cur_span = preview_lines[cur_lp.line_index];
                // v1.14 (2026-05-03): 密码态用 mask 化的等长子串 measure,
                // 与 paint 写出的 '•' 字符串视觉宽度严格一致, 修复光标贴右
                // 偏离最后一个 mask 字符的旧 bug。
                const std::string cur_render = password_
                    ? mask_substring(text(), cur_span.start, cursor_pos_)
                    : text().substr(cur_span.start, cursor_pos_ - cur_span.start);
                float cx = text_left + Application::measure_text_width(
                    cur_render, 13.0F, false);
                cx = std::min(cx, text_right);
                const float cy_top = visible_text_top
                    + static_cast<float>(cur_lp.line_index - scroll_offset_lines_) * kTextLineH;
                const float cy_bot = cy_top + kTextLineH;
                context.line(PointF{cx, cy_top}, PointF{cx, cy_bot},
                             c.text_primary, 1.0F);
            }
        }
    }

    context.pop_clip();

    // v1.7: 右侧细滚动条 (line_count > visible_lines 时画)。颜色 muted, 圆角细条,
    // thumb 高度按可见比例算, 位置按 scroll_offset_lines_ / line_count 比例。
    // 不响应拖动, 视觉指示用; wheel 改 offset 让它移动。
    if (line_count > visible_lines) {
        constexpr float bar_w = 4.0F;
        const float track_y = field.y + kTextTopPad;
        const float track_h = available_h;
        const float bar_x   = field.x + field.width - bar_w - 6.0F;
        const float thumb_h = std::max(
            track_h * static_cast<float>(visible_lines) / static_cast<float>(line_count),
            16.0F);
        const float thumb_y = track_y + track_h
            * static_cast<float>(scroll_offset_lines_) / static_cast<float>(line_count);
        // 轨道 (淡)
        Color track_c = c.border;
        track_c.a *= 0.4F;
        context.fill_rounded_rect(RectF{bar_x, track_y, bar_w, track_h}, track_c, 2.0F);
        // 滑块
        context.fill_rounded_rect(RectF{bar_x, thumb_y, bar_w, thumb_h},
                                    c.text_muted, 2.0F);
    }

    if (!helper_.empty()) {
        context.draw_text(
            RectF{frame().x, field.y + field.height + 2.0F, frame().width, helper_h},
            helper_, helper_color, TextAlignment::Leading, 11.0F);
    }

    // v1.3 (2026-04-28): 右键浮层菜单 — 复制 / 剪切 / 粘贴 横向 3 列。
    // 横向 + 单行 28px 高紧贴 widget frame (38px), 不溢出, 父级兄弟 widget 不会
    // 盖住, hit_test 默认 frame.contains 直接命中点击。
    if (ctx_menu_open_) {
        const RectF menu = context_menu_rect();
        // 阴影
        context.fill_rounded_rect(
            RectF{menu.x + 1.0F, menu.y + 2.0F, menu.width, menu.height},
            Color{0.0F, 0.0F, 0.0F, 0.20F}, 8.0F);
        // 背景 + 边框
        context.fill_rounded_rect(menu, c.bg_raised, 8.0F);
        context.stroke_rounded_rect(menu, c.border, 8.0F, 1.0F);
        // 3 列文字 (有选区才高亮"复制"/"剪切", 否则灰)
        const float col_w = menu.width / 3.0F;
        const bool can_clip = has_selection();
        const Color disabled = c.text_disabled;
        const Color enabled_c = c.text_primary;
        context.draw_text(
            RectF{menu.x + col_w * 0.0F, menu.y, col_w, menu.height},
            std::string{"复制"}, can_clip ? enabled_c : disabled,
            TextAlignment::Center, 13.0F, false);
        context.draw_text(
            RectF{menu.x + col_w * 1.0F, menu.y, col_w, menu.height},
            std::string{"剪切"}, can_clip ? enabled_c : disabled,
            TextAlignment::Center, 13.0F, false);
        context.draw_text(
            RectF{menu.x + col_w * 2.0F, menu.y, col_w, menu.height},
            std::string{"粘贴"}, enabled_c,
            TextAlignment::Center, 13.0F, false);
        // 列间竖向分割线
        context.line(PointF{menu.x + col_w * 1.0F, menu.y + 4.0F},
                     PointF{menu.x + col_w * 1.0F, menu.y + menu.height - 4.0F},
                     c.border, 0.5F);
        context.line(PointF{menu.x + col_w * 2.0F, menu.y + 4.0F},
                     PointF{menu.x + col_w * 2.0F, menu.y + menu.height - 4.0F},
                     c.border, 0.5F);
    }
}

} // namespace hui
