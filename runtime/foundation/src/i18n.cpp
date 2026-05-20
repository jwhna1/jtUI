// jtUI i18n 实现 —— 见 i18n.hpp 设计说明。
// 维护：曾能混 <jwhna1@gmail.com>

#include "hui/foundation/i18n.hpp"

#include <string>
#include <unordered_map>
#include <utility>

namespace hui::i18n {

namespace {

struct StoreImpl {
    Locale current {Locale::En};
    std::unordered_map<std::string, std::pair<std::string, std::string>> catalog;
    std::string empty;
};

StoreImpl& impl() noexcept {
    static StoreImpl s;
    return s;
}

}  // namespace

Store& Store::instance() noexcept {
    static Store s;
    return s;
}

Locale Store::current() const noexcept {
    return impl().current;
}

void Store::set_current(Locale locale) noexcept {
    impl().current = locale;
}

void Store::register_entries(const std::vector<Entry>& entries) {
    auto& s = impl();
    for (const auto& e : entries) {
        s.catalog[e.key] = {e.en, e.zh};
    }
}

const std::string& Store::tr(std::string_view key) const {
    auto& s = impl();
    const auto it = s.catalog.find(std::string(key));
    if (it == s.catalog.end()) {
        return s.empty;
    }
    return (s.current == Locale::Zh) ? it->second.second : it->second.first;
}

}  // namespace hui::i18n
