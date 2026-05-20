#pragma once

// jtUI i18n —— 极简多语言字典。
// 维护：曾能混 <jwhna1@gmail.com>
//
// 设计取舍：
//   - 单例 Store（进程内全局），不依赖 Signal/Property/Theme，放在 foundation 层
//     是因为 widgets 和上层都可能要 tr() 显示文字。
//   - 不主动通知。set_locale 后业务自己驱动 widget 重建（root.clear_children() +
//     重新 build_*）。这套流程在 example 里只是一行调用，比维护订阅链路简单且不
//     容易漏 widget。
//   - catalog 是 key → {en, zh} 二元 map，未注册的 key 返回 empty string，
//     业务期间能立刻看到"漏译"，无需 silent fallback。
//   - tr() 返回 const std::string& —— 调用方可以零拷贝传给 set_content。
//
// 用法：
//   jtui::i18n::register_entries({
//       {"hero.title.line1", "Build native.", "原生构建。"},
//       {"hero.cta.primary", "Start Building", "立即开始"},
//   });
//   jtui::i18n::set_locale(jtui::i18n::Locale::Zh);
//   text->set_content(jtui::i18n::tr("hero.title.line1"));

#include <string>
#include <string_view>
#include <vector>

namespace hui::i18n {

enum class Locale {
    En,
    Zh,
};

struct Entry {
    std::string key;
    std::string en;
    std::string zh;
};

class Store {
public:
    [[nodiscard]] static Store& instance() noexcept;

    [[nodiscard]] Locale current() const noexcept;
    void set_current(Locale locale) noexcept;

    // 批量注册。重复 key 直接覆盖（example 重新加载场景）。
    void register_entries(const std::vector<Entry>& entries);

    // 未注册时返回 empty string —— 业务能立刻看到漏译。
    [[nodiscard]] const std::string& tr(std::string_view key) const;

private:
    Store() = default;
    ~Store() = default;
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
};

// 顶层便捷函数 —— 业务一般直接用这些，避免每次写 Store::instance()。
[[nodiscard]] inline const std::string& tr(std::string_view key) {
    return Store::instance().tr(key);
}

inline void register_entries(const std::vector<Entry>& entries) {
    Store::instance().register_entries(entries);
}

inline void set_locale(Locale locale) noexcept {
    Store::instance().set_current(locale);
}

[[nodiscard]] inline Locale current_locale() noexcept {
    return Store::instance().current();
}

}  // namespace hui::i18n
