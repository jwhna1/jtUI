#pragma once

// jtui_pro i18n 字典 —— en/zh 双语。
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace jtui_pro {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.product",  "PRODUCT",         "产品"},
        {"nav.pricing",  "PRICING",         "定价"},
        {"nav.security", "SECURITY",        "安全"},
        {"nav.cta",      "Talk to Sales",   "联系销售"},

        // hero pill
        {"hero.pill.tag",  "Enterprise",                 "企业版"},
        {"hero.pill.text", "SSO + Audit logs available", "已支持 SSO 与审计日志"},

        // hero title 双段 highlight：
        // EN: "Built for {teams} that {ship}."
        // ZH: "为{团队}而生，助{出货}。"
        {"hero.title.seg1", "Built for ", "为"},
        {"hero.title.seg2", "teams",      "团队"},
        {"hero.title.seg3", " that ",     "而生，助"},
        {"hero.title.seg4", "ship",       "出货"},
        {"hero.title.seg5", ".",          "。"},

        // hero sub
        {"hero.sub.line1",
         "SSO. Audit logs. Priority support.",
         "单点登录。审计日志。优先支持。"},
        {"hero.sub.line2",
         "The missing enterprise tier for native UI.",
         "原生 UI 的企业级补全。"},

        // hero CTA
        {"hero.cta.primary",   "Talk to Sales", "联系销售"},
        {"hero.cta.secondary", "View Plans",    "查看方案"},

        // trust 徽记墙（保留英文徽记词，业务里它们是国际通用合规标识）
        {"trust.heading", "Compliance & SLA", "合规与 SLA"},
        {"trust.soc2",    "SOC 2 Type II",    "SOC 2 Type II"},
        {"trust.gdpr",    "GDPR",             "GDPR"},
        {"trust.saml",    "SAML SSO",         "SAML SSO"},
        {"trust.sla",     "99.9% SLA",        "99.9% SLA"},

        // footer
        {"footer.author",
         "Crafted by 曾能混  ·  jwhna1@gmail.com",
         "曾能混 制作  ·  jwhna1@gmail.com"},

        // 背景模式（按钮 tooltip 用，目前 icon-only 切换不显示但保留 i18n key）
        {"bg.streams",   "Bezier streams",  "贝塞尔流线"},
        {"bg.generated", "Generated glow",  "程序化光晕"},
        {"bg.image",     "PNG image",       "PNG 图片"},

        // window title
        {"window.title", "jtUI Pro · Built for teams that ship.",
                         "jtUI Pro · 为出货的团队而生"},
    });
}

}  // namespace jtui_pro
