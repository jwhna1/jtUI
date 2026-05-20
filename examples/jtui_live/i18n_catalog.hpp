#pragma once

// jtui_live i18n 字典 —— en/zh 双语。
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_live {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.docs",     "DOCS",          "文档"},
        {"nav.samples",  "SAMPLES",       "示例"},
        {"nav.api",      "API",           "API"},
        {"nav.cta",      "Try jtUI Live", "立即体验"},

        // hero pill
        {"hero.pill.tag",  "v0.2 release",          "v0.2 发布"},
        {"hero.pill.text", "Hot reload landed",     "热重载已上线"},

        // hero title line 1: "Edit Code."  /  "编辑代码。"
        {"hero.title.l1.prefix",    "Edit ",  "编辑"},
        {"hero.title.l1.highlight", "Code",   "代码"},
        {"hero.title.l1.suffix",    ".",      "。"},

        // hero title line 2: "Reload in Seconds."  /  "瞬间重载。"
        {"hero.title.l2.prefix",    "Reload in ", "瞬间"},
        {"hero.title.l2.highlight", "Seconds",    "重载"},
        {"hero.title.l2.suffix",    ".",          "。"},

        // hero sub
        {"hero.sub.line1",
         "Pure C++ widget framework with hot reload.",
         "纯 C++ 控件框架，原生支持热重载。"},
        {"hero.sub.line2",
         "No restart. No rebuild. No JS.",
         "不重启。不重编译。不要 JS。"},

        // hero CTA
        {"hero.cta.primary",   "Get Live Now",     "立即体验"},
        {"hero.cta.secondary", "See How It Works", "工作原理"},

        // hero check features
        {"hero.feat.1", "Hot reload",        "热重载"},
        {"hero.feat.2", "Stateful",          "状态保持"},
        {"hero.feat.3", "< 100ms",           "< 100 毫秒"},

        // 右侧编辑器 mock
        {"editor.title",   "ProfileCard.cpp",  "ProfileCard.cpp"},
        {"editor.comment", "// edit, save, see it live",
                           "// 编辑、保存、立即生效"},

        // 右下角浮卡 reload status
        {"reload.title",    "Reloaded",          "已重载"},
        {"reload.duration", "47ms",              "47 毫秒"},
        {"reload.file",     "ProfileCard.cpp",   "ProfileCard.cpp"},
        {"reload.diff",     "View Diff",         "查看差异"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // window
        {"window.title", "jtUI Live · Edit Code. Reload in Seconds.",
                         "jtUI Live · 编辑代码。瞬间重载。"},
    });
}

}  // namespace jtui_live
