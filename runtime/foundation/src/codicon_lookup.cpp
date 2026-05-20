// Package: hui::foundation
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#include "hui/foundation/codicon_lookup.hpp"

#include <algorithm>
#include <array>

#include "hui/foundation/codicon_codepoints.hpp"

namespace hui::foundation {

namespace {

// 名字字典序辅助索引。kCodiconCodepoints 按 codepoint 排序，对 name 查找
// 不利。这里把全部 entry 复制成"按 name 排序"的索引，二分查找。
// 索引初始化在首次调用 codicon_codepoint_for 时懒构建，避免静态对象之间的
// 初始化顺序不确定问题（也避免无人调用时白白构造）。
using NameIndex = std::array<const CodiconCodepointEntry*, kCodiconCodepoints.size()>;

NameIndex build_name_index() noexcept {
    NameIndex idx{};
    for (std::size_t i = 0; i < kCodiconCodepoints.size(); ++i) {
        idx[i] = &kCodiconCodepoints[i];
    }
    std::sort(idx.begin(), idx.end(),
              [](const CodiconCodepointEntry* a, const CodiconCodepointEntry* b) noexcept {
                  return a->name < b->name;
              });
    return idx;
}

const NameIndex& name_index() noexcept {
    static const NameIndex kIndex = build_name_index();
    return kIndex;
}

}  // namespace

std::optional<char32_t> codicon_codepoint_for(std::string_view name) noexcept {
    if (name.empty()) {
        return std::nullopt;
    }
    const auto& idx = name_index();
    const auto it = std::lower_bound(
        idx.begin(), idx.end(), name,
        [](const CodiconCodepointEntry* entry, std::string_view target) noexcept {
            return entry->name < target;
        });
    if (it == idx.end() || (*it)->name != name) {
        return std::nullopt;
    }
    return (*it)->codepoint;
}

}  // namespace hui::foundation
