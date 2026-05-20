#!/usr/bin/env python3
# Package: hui::resources::codicon
# Author:   jtai 团队 :（曾能混&tang先森）<jwhna1@gmail.com>
# https://jtai.cc
#
# 把 resources/fonts/codicon.ttf + codicon.css 转成两份 C++ 源文件：
#   embedded_codicon_font.cpp  -- 字体 ttf 字节数组 (constexpr 静态数据)
#   codicon_codepoints.hpp     -- 名称 -> codepoint 映射 (constexpr 表)
#
# 触发时机：升级 @vscode/codicons npm 版本后，把新 ttf+css 放到 resources/fonts/
# 然后跑 python3 scripts/codicon/generate_codicon_sources.py
# 生成产物 check-in，构建链路不依赖 Python 运行时。
#
# 用法：从 jtUI-v1.0/ 根目录跑，无参数。
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TTF_PATH = ROOT / "resources" / "fonts" / "codicon.ttf"
CSS_PATH = ROOT / "resources" / "fonts" / "codicon.css"

OUT_DIR = ROOT / "runtime" / "foundation"
EMBED_CPP = OUT_DIR / "src" / "embedded_codicon_font.cpp"
CODEPOINTS_HPP = OUT_DIR / "include" / "hui" / "foundation" / "codicon_codepoints.hpp"


def parse_css(css_text: str) -> list[tuple[str, int]]:
    """从 codicon.css 抽出 (icon_name, codepoint) 列表。
    一个 codepoint 可能有多个别名 (codicon-plus / codicon-add 都映射到 0xEA60)。"""
    pattern = re.compile(
        r"\.codicon-([a-z0-9][a-z0-9-]*)" r":before\s*\{\s*content:\s*\"\\([0-9a-fA-F]+)\"\s*\}"
    )
    out: list[tuple[str, int]] = []
    for m in pattern.finditer(css_text):
        name = m.group(1)
        cp = int(m.group(2), 16)
        out.append((name, cp))
    return out


def write_embed_cpp(ttf_bytes: bytes, out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    # 每行 16 字节，避免编辑器一行过长。
    rows: list[str] = []
    for i in range(0, len(ttf_bytes), 16):
        chunk = ttf_bytes[i:i + 16]
        rows.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    body = "\n".join(rows)

    src = f"""// Package: hui::resources::codicon
// Author:   jtai 团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// 此文件由 scripts/codicon/generate_codicon_sources.py 生成，不要手改。
// 源资源：resources/fonts/codicon.ttf (总 {len(ttf_bytes)} 字节)
// 源 npm 包：@vscode/codicons (MIT)
#include "hui/foundation/codicon_font.hpp"

#include <cstddef>
#include <cstdint>

namespace hui::foundation {{

namespace {{

// codicon.ttf 原始字节。constexpr static 保证只走 .rodata。
constexpr std::uint8_t kCodiconTtfData[] = {{
{body}
}};

constexpr std::size_t kCodiconTtfSize = sizeof(kCodiconTtfData);

}}  // namespace

CodiconFontBlob codicon_font_blob() noexcept {{
    return CodiconFontBlob {{
        .data = static_cast<const void*>(&kCodiconTtfData[0]),
        .size = kCodiconTtfSize,
    }};
}}

}}  // namespace hui::foundation
"""
    out_path.write_text(src, encoding="utf-8")


def write_codepoints_hpp(mappings: list[tuple[str, int]], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    # 排序：先按 codepoint 升序，再按 name 字母序（同 codepoint 的别名相邻）。
    mappings_sorted = sorted(mappings, key=lambda x: (x[1], x[0]))
    rows: list[str] = []
    for name, cp in mappings_sorted:
        rows.append(f'    {{"{name}", 0x{cp:04X}U}},')
    body = "\n".join(rows)
    count = len(mappings_sorted)

    src = f"""// Package: hui::foundation
// Author:   jtai 团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// 此文件由 scripts/codicon/generate_codicon_sources.py 生成，不要手改。
// 源资源：resources/fonts/codicon.css。共 {count} 个 name->codepoint 映射，
// 多个 name 可能共享同一个 codepoint（plus / add / gist-new 都 = 0xEA60）。
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace hui::foundation {{

struct CodiconCodepointEntry {{
    std::string_view name;
    char32_t codepoint;
}};

inline constexpr std::array<CodiconCodepointEntry, {count}> kCodiconCodepoints = {{{{
{body}
}}}};

}}  // namespace hui::foundation
"""
    out_path.write_text(src, encoding="utf-8")


def main() -> int:
    if not TTF_PATH.exists():
        print(f"missing ttf: {TTF_PATH}", file=sys.stderr)
        return 1
    if not CSS_PATH.exists():
        print(f"missing css: {CSS_PATH}", file=sys.stderr)
        return 1

    ttf_bytes = TTF_PATH.read_bytes()
    css_text = CSS_PATH.read_text(encoding="utf-8")
    mappings = parse_css(css_text)

    if not mappings:
        print("CSS parse produced 0 mappings", file=sys.stderr)
        return 2

    write_embed_cpp(ttf_bytes, EMBED_CPP)
    write_codepoints_hpp(mappings, CODEPOINTS_HPP)

    unique_cps = {cp for _, cp in mappings}
    print(f"ttf bytes: {len(ttf_bytes)}")
    print(f"css aliases: {len(mappings)}")
    print(f"unique codepoints: {len(unique_cps)}")
    print(f"wrote: {EMBED_CPP.relative_to(ROOT)}")
    print(f"wrote: {CODEPOINTS_HPP.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
