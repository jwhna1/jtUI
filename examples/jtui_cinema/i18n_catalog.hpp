#pragma once

// jtui_cinema 的 i18n 字典 —— en/zh 双语。
// hero 标题两行：
//   line1 "World's First"
//   line2 "Native Cinematic UI"  —— "Cinematic" 染暖橘 accent
// 主题：反 Electron / 反 HTML5 <video> / Pure C++ / Real frames.
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_cinema {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.films",   "FILMS",       "影片"},
        {"nav.stack",   "STACK",       "技术栈"},
        {"nav.specs",   "SPECS",       "规格"},
        {"nav.cta",     "Get Beta",    "申请内测"},

        // hero pill
        {"hero.pill.tag",  "v0.4",                              "v0.4"},
        {"hero.pill.text", "Real frames on Windows · D2D 1.1",  "Windows 原生帧 · D2D 1.1"},

        // hero title line 1: "World's First"
        {"hero.title.l1.prefix",    "World's ",       "全球"},
        {"hero.title.l1.highlight", "First",          "首个",},
        {"hero.title.l1.suffix",    "",               ""},

        // hero title line 2: "Native Cinematic UI."
        {"hero.title.l2.prefix",    "Native ",        "原生"},
        {"hero.title.l2.highlight", "Cinematic",      "影像",},
        {"hero.title.l2.suffix",    " UI.",           " UI。"},

        // hero 副文
        {"hero.sub.line1",
         "Pure C++. Real frames. Zero JS bridge.",
         "纯 C++。真实帧。无 JS 桥。"},
        {"hero.sub.line2",
         "Decoded by Media Foundation. Painted by Direct2D 1.1.",
         "Media Foundation 解码，Direct2D 1.1 绘制。"},

        // 卡片标题（视频 reel 用，主题：原生工程师故事 5 段；地名走真实地理）
        // 旧版本曾用 PDF p15 第三方短片名，已统一为 jtUI 自身的"原生开发"主题文案。
        {"reel.item1.title",  "Hello jtUI",            "你好，jtUI"},
        {"reel.item1.region", "China",                 "中国"},
        {"reel.item2.title",  "Render Real",           "真实渲染"},
        {"reel.item2.region", "Mongolia",              "蒙古"},
        {"reel.item3.title",  "Frame by Frame",        "逐帧打磨"},
        {"reel.item3.region", "Vietnam",               "越南"},
        {"reel.item4.title",  "No Bridge, No JS",      "无桥无 JS"},
        {"reel.item4.region", "Japan",                 "日本"},
        {"reel.item5.title",  "Ship Native",           "原生出货"},
        {"reel.item5.region", "Taiwan",                "台湾"},

        // 卡片状态文字
        {"reel.playing", "PLAYING",     "正在播放"},
        {"reel.click_to_play", "Click play to start",   "点击播放"},

        // 翻页/CTA
        {"reel.prev",    "Previous",    "上一段"},
        {"reel.next",    "Next",        "下一段"},

        // bottom watermark
        {"hero.watermark", "COMING SOON",  "COMING SOON"},
        {"hero.launching", "LAUNCHING 2026", "2026 上线"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // 窗口标题
        {"window.title", "jtUI Cinema · World's First Native Cinematic UI",
                         "jtUI Cinema · 全球首个原生影像 UI"},

        // About / 关于 i18n 条目全部抽到 examples/_shared/about_dialog.hpp 的
        // register_about_strings() —— 所有 example 共用同一套 about.* keys。
    });
}

}  // namespace jtui_cinema
