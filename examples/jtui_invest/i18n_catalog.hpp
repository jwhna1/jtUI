#pragma once

// jtui_invest 的 i18n 字典 —— en/zh 双语。视觉范式借鉴 桌面UI.pdf p20 金融落地页。
// 金额 / 公司名 / 百分比这类数据值中英保持一致（不翻译），只翻译 UI 标签文案。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_invest {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.cta", "START FOR FREE", "免费开始"},

        // hero 标题：两行。第一行三段，"financial data" 段高亮（酸橙绿 + 下划线）
        {"hero.l1.pre",  "Access your ",   "畅享你的"},
        {"hero.l1.mark", "financial data", "财务数据"},
        {"hero.l1.post", " with minimal",  "，省心"},
        {"hero.l2",      "hassle. Made for you.", "无忧，为你而生。"},
        {"hero.sub.l1",
         "Providing instant access to your data anytime, anywhere. With a "
         "user-friendly interface and",
         "随时随地即刻访问你的数据。友好的界面、流畅的功能，"},
        {"hero.sub.l2",
         "seamless functionality, managing your data has never been easier.",
         "让数据管理从未如此简单。"},
        {"hero.cta", "Start Now For Free", "立即免费开始"},
        {"hero.feat.1", "Up to 300GB storage on free plan", "免费版最高 300GB 存储"},
        {"hero.feat.2", "Unlimited data points",            "数据点无上限"},
        {"hero.feat.3", "Set up in just 1 day",             "一天即可上手"},

        // dashboard 面板
        {"dash.title", "Dashboard",                          "仪表盘"},
        {"dash.sub",   "Track your portfolio value, gains and more.",
                       "追踪你的组合价值、收益及更多。"},
        {"dash.add",   "Add New",                            "新增"},

        // PORTFOLIO 卡片
        {"portfolio.label",    "PORTFOLIO",   "投资组合"},
        {"portfolio.invested", "INVESTED",    "已投入"},
        {"portfolio.return",   "RETURN",      "回报"},

        // QUARTERLY GAINS 卡片
        {"gains.label", "QUARTERLY GAINS", "季度收益"},

        // WATCHED COMPANIES 卡片
        {"watched.label",  "WATCHED COMPANIES", "关注的公司"},
        {"watched.col.company", "Company",      "公司"},
        {"watched.col.price",   "Share Price",  "股价"},
        {"watched.col.val",     "Valuation",    "估值"},

        // Spotify chart 卡片
        {"chart.tooltip.date",   "28 NOV 2023", "2023-11-28"},
        {"chart.tooltip.value",  "VALUE",       "价值"},
        {"chart.tooltip.return", "RETURN",      "回报"},
        {"chart.analysis",       "Chart Analysis", "图表分析"},

        // QUARTERLY TOP GAINERS
        {"gainers.label", "QUARTERLY TOP GAINERS", "季度涨幅榜"},
        {"gainers.shares", "shares", "股"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // 窗口标题
        {"window.title", "jtUI Invest  ·  Access your financial data.",
                         "jtUI Invest  ·  畅享你的财务数据。"},
    });
}

}  // namespace jtui_invest
