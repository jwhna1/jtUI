#pragma once

// jtui_atlas 的 i18n 字典 —— en/zh 双语。
// 数据大屏文案：观察 / 告警 / 流量。"Watch every signal." / "看见每一个信号。"
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_atlas {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.overview", "OVERVIEW", "总览"},
        {"nav.alerts",   "ALERTS",   "告警"},
        {"nav.streams",  "STREAMS",  "数据流"},
        {"nav.cta",      "Account",  "账户"},

        // 顶部 hero 简介条（datavis 不做 hero 大字，给个小标题 + 副文行）
        {"hero.title",   "Watch every signal.",
                         "看见每一个信号。"},
        {"hero.sub",
         "Real-time observability across users, latency, errors and uptime.",
         "实时观测用户、延时、错误与可用性，一屏掌握。"},

        // KPI 卡片 4 个
        {"kpi.users.label",   "USERS",                "用户"},
        {"kpi.users.value",   "124k",                 "12.4 万"},
        {"kpi.users.delta",   "+12% this week",       "本周 +12%"},

        {"kpi.latency.label", "LATENCY",              "延时"},
        {"kpi.latency.value", "42 ms",                "42 ms"},
        {"kpi.latency.delta", "p99 215 ms",           "p99 215 ms"},

        {"kpi.errors.label",  "ERRORS",               "错误率"},
        {"kpi.errors.value",  "0.03%",                "0.03%"},
        {"kpi.errors.delta",  "last 24h",             "近 24 小时"},

        {"kpi.uptime.label",  "UPTIME",               "可用性"},
        {"kpi.uptime.value",  "99.98%",               "99.98%"},
        {"kpi.uptime.delta",  "rolling 30 days",      "近 30 日"},

        // chart 卡片
        {"chart.volume.title",  "Request Volume",     "请求量"},
        {"chart.volume.range",  "Past 24h",           "过去 24 小时"},
        {"chart.latency.title", "P95 Latency",        "P95 延时"},
        {"chart.latency.range", "Past 24h",           "过去 24 小时"},

        // 事件流
        {"events.title",        "Live events",        "实时事件"},
        {"events.row.1",        "03:14   Alert resolved   payments-gw",
                                "03:14   告警解除          payments-gw"},
        {"events.row.2",        "03:12   Deploy ok        api v2.41",
                                "03:12   发布成功          api v2.41"},
        {"events.row.3",        "03:10   Probe ok         all regions",
                                "03:10   探测正常          全区域"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // 窗口标题
        {"window.title", "jtUI Atlas  ·  Watch every signal.",
                         "jtUI Atlas  ·  看见每一个信号。"},
    });
}

}  // namespace jtui_atlas
