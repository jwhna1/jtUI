#pragma once

// jtui_pulse i18n 字典 —— en/zh 双语。
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_pulse {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.metrics",  "METRICS",          "指标"},
        {"nav.alerts",   "ALERTS",           "告警"},
        {"nav.docs",     "DOCS",             "文档"},
        {"nav.cta",      "Start Watching",   "立即监控"},

        // hero rating
        {"rating.text",  "99.99% uptime  ·  trusted by 4,000+ teams",
                         "99.99% 在线  ·  4,000+ 团队信赖"},

        // hero title 双段 highlight
        // EN: "See what's happening,\n{right} {now}."
        // ZH: "看见每件事，\n{此刻}{发生}。"
        {"hero.title.l1",           "See what's happening,",  "看见每件事，"},
        {"hero.title.l2.prefix",    "",                       ""},
        {"hero.title.l2.highlight1","right",                  "此刻"},
        {"hero.title.l2.middle",    " ",                      ""},
        {"hero.title.l2.highlight2","now",                    "发生"},
        {"hero.title.l2.suffix",    ".",                      "。"},

        // hero sub
        {"hero.sub.line1",
         "Real-time dashboards, alerts, and",
         "实时仪表盘、告警与"},
        {"hero.sub.line2",
         "incident timelines for native apps.",
         "原生应用的事件时间线。"},

        // email + cta
        {"hero.email.placeholder", "Enter your email",  "输入邮箱地址"},
        {"hero.cta",               "Start Watching",    "立即监控"},
        {"hero.privacy",           "We care about your data — see our privacy policy.",
                                   "我们重视你的数据 — 参见隐私协议。"},

        // hero check features
        {"hero.feat.1", "Sub-second alerts",       "亚秒级告警"},
        {"hero.feat.2", "Free up to 5 services",   "5 个服务以下免费"},
        {"hero.feat.3", "Open metrics format",     "Open Metrics 格式"},

        // chart 装饰右下角小标
        {"chart.label",    "live · 24h",     "实时 · 24 小时"},
        {"chart.endpoint", "api.example",    "api.example"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // window
        {"window.title", "jtUI Pulse · See what's happening, right now.",
                         "jtUI Pulse · 看见每件事，此刻发生。"},
    });
}

}  // namespace jtui_pulse
