#pragma once

// jtui_hero 的 i18n 字典 —— en/zh 双语。
// 注册一次（启动时调 register_jtui_hero_strings()），之后用 jtui::i18n::tr(key) 取。
//
// 设计：标题两行各拆 {prefix, highlight, suffix} 三段，让 set_runs 在不同语言下
// 都能保持"中间高亮"的视觉范式 —— 英文高亮 "native"/"now"，中文高亮 "构建"/"交付"。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_hero {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.docs",     "DOCS",        "文档"},
        {"nav.samples",  "SAMPLES",     "示例"},
        {"nav.widgets",  "WIDGETS",     "组件"},
        {"nav.cta",      "Get the SDK", "获取 SDK"},

        // hero pill
        {"hero.pill.tag",  "v0.2 release",                "v0.2 发布"},
        {"hero.pill.text", "Retained-mode core landed",   "保留模式核心已上线"},

        // hero title line 1: "Build native."  /  "原生构建。"
        {"hero.title.l1.prefix",    "Build ",  "原生"},
        {"hero.title.l1.highlight", "native",  "构建"},
        {"hero.title.l1.suffix",    ".",       "。"},

        // hero title line 2: "Ship now. No bullshit."  /  "立即交付。不啰嗦。"
        {"hero.title.l2.prefix",    "Ship ",          "立即"},
        {"hero.title.l2.highlight", "now",            "交付"},
        {"hero.title.l2.suffix",    ". No bullshit.", "。不啰嗦。"},

        // hero sub
        {"hero.sub.line1",
         "Pure C++ widgets. Zero Electron. Zero V8.",
         "纯 C++ 控件。零 Electron。零 V8。"},
        {"hero.sub.line2",
         "One static binary. Two platforms. Yours to ship today.",
         "一个静态二进制。两端平台。今天就能交付。"},

        // hero CTA
        {"hero.cta.primary",   "Start Building",   "开始构建"},
        {"hero.cta.secondary", "Read the Source",  "阅读源码"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // code card
        {"code.title",   "hero.cpp",                                   "hero.cpp"},
        {"code.comment", "// that's it. no JSX, no bundler, no .d.ts.",
                         "// 就这样。没有 JSX、没有 bundler、没有 .d.ts。"},

        // window title
        {"window.title", "jtUI · Build native. Ship now.",
                         "jtUI · 原生构建。立即交付。"},
    });
}

}  // namespace jtui_hero
