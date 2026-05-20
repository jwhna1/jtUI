#pragma once

// jtui_studio 的 i18n 字典 —— en/zh 双语。
// hero 标题两段：line1 "Build Media-Native.", line2 "Native FAST." —— 第二行用
// set_runs 把 "FAST" 染成 accent 葡萄紫，呼应"快"的力道感。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_studio {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.video",    "VIDEO",       "视频"},
        {"nav.audio",    "AUDIO",       "音频"},
        {"nav.studio",   "STUDIO",      "工作台"},
        {"nav.cta",      "Open Project","打开项目"},

        // hero pill (右栏顶部小角标)
        {"hero.pill.tag",  "v0.3",                              "v0.3"},
        {"hero.pill.text", "Media pipeline shipped on Windows", "Windows 媒体管线已交付"},

        // hero title line 1: "Build Media-Native." / "原生媒体，开箱即用。"
        {"hero.title.l1.prefix",    "Build ",         "原生"},
        {"hero.title.l1.highlight", "Media-Native",   "媒体"},
        {"hero.title.l1.suffix",    ".",              "，开箱即用。"},

        // hero title line 2: "Native FAST." / "原生快。"
        {"hero.title.l2.prefix",    "Native ",        "原生"},
        {"hero.title.l2.highlight", "FAST",           "快"},
        {"hero.title.l2.suffix",    ".",              "。"},

        // hero 副文
        {"hero.sub.line1",
         "Decode, play, scrub in the same process as your UI.",
         "解码、播放、拖动 与 UI 同进程，零跨进程开销。"},
        {"hero.sub.line2",
         "No FFmpeg subprocess. No HTML5 <video>. Just Direct2D + WASAPI.",
         "无 FFmpeg 子进程。无 HTML5 <video>。仅 Direct2D + WASAPI。"},

        // hero feature bullets
        {"hero.feat.1", "VideoPlayer + frame-precise seek",   "VideoPlayer + 帧级 seek"},
        {"hero.feat.2", "AudioPlayer + WaveformView",         "AudioPlayer + 波形视图"},
        {"hero.feat.3", "Timeline + LevelMeter",              "Timeline + 电平表"},

        // hero CTA
        {"hero.cta.primary",   "Open Project",  "打开项目"},
        {"hero.cta.secondary", "Read the Docs", "查阅文档"},

        // 左栏 video 卡片
        {"card.video.title",   "test.mp4",                "test.mp4"},
        {"card.video.caption", "VideoPlayer · 1080p H.264", "VideoPlayer · 1080p H.264"},

        // 左栏 audio 卡片
        {"card.audio.title",   "test.mp3",                "test.mp3"},
        {"card.audio.caption", "AudioPlayer · 44.1 kHz",  "AudioPlayer · 44.1 kHz"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // 窗口标题
        {"window.title", "jtUI Studio · Build Media-Native. Native FAST.",
                         "jtUI Studio · 原生媒体，原生快。"},
    });
}

}  // namespace jtui_studio
