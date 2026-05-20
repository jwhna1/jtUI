// Package: hui::foundation
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// codicon 名称 -> codepoint 查找。kCodiconCodepoints 按 (codepoint, name)
// 排序，但查找接口走 name，所以这里实现 O(log N) 线性 + 二分混合：
// 先对 name 做一次性 hash O(1)，未命中再回退线性 O(N)。
// 总条目 649，单次查找 < 1us，足够 paint 路径调用。
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace hui::foundation {

// 查表：给定 codicon 名称 (如 "folder" / "plus" / "git-fetch")，返回 codepoint。
// 找不到返回 nullopt。调用方常见 fallback：画一个占位 rect / 空操作。
[[nodiscard]] std::optional<char32_t> codicon_codepoint_for(std::string_view name) noexcept;

}  // namespace hui::foundation
