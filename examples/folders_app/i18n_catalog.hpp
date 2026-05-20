#pragma once

// folders_app i18n 字典 —— en/zh 双语。
// 启动时调 register_strings()，之后用 jtui::i18n::tr(key) 取。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "jtui/jtui.hpp"

namespace folders_app {

inline void register_strings() {
    jtui::i18n::register_entries({
        // navbar
        {"nav.pricing",   "PRICING",         "价格"},
        {"nav.features",  "FEATURES",        "功能"},
        {"nav.cta",       "START FOR FREE",  "免费开始"},

        // hero
        {"hero.pill.tag",  "News",            "新闻"},
        {"hero.pill.text", "Series A Funding", "A 轮融资"},
        {"hero.title.line1", "Access your folders at any time with",
                             "随时随地访问你的文件夹"},
        {"hero.title.line2", "minimal hassle. Made for you.",
                             "无忧无虑。为你而生。"},
        {"hero.sub.line1", "Providing instant access to your folders anytime, anywhere.",
                           "为你提供随时随地的文件夹即时访问。"},
        {"hero.sub.line2",
         "With a user-friendly interface and seamless functionality, managing your digital files has never been easier.",
         "友好的界面与无缝的功能，管理你的数字文件从未如此简单。"},
        {"hero.cta",      "Create Folder For Free", "免费创建文件夹"},
        {"hero.feat.1",   "Up to 300GB storage on free plan", "免费版最多 300GB 存储"},
        {"hero.feat.2",   "Unlimited folders",                 "文件夹数量无上限"},
        {"hero.feat.3",   "Set up in just 1 day",              "一天即可完成设置"},

        // dashboard header
        {"dash.welcome.prefix", "Welcome back, ",       "欢迎回来，"},
        {"dash.welcome.sub",    "Access your favorite orders",
                                "访问你的收藏"},
        {"dash.title",     "My Folders",        "我的文件夹"},
        {"dash.sort.label","Sorty By",          "排序"},
        {"dash.sort.value","Most Recent  v",    "最新优先  v"},
        {"dash.select.empty", "Click a folder to select…",
                              "点击文件夹进行选择…"},
        {"dash.select.prefix", "Selected: ",    "已选：" },
        {"dash.photos.suffix", " photos",       " 张照片"},

        // sidebar
        {"sidebar.user.role", "Personal",   "个人"},
        {"sidebar.my_folders","My Folders", "我的文件夹"},
        {"sidebar.favorite",  "Favorite",   "收藏"},
        {"sidebar.history",   "History",    "历史"},
        {"sidebar.invite",    "Invite",     "邀请"},
        {"sidebar.support",   "Support",    "支持"},
        {"sidebar.settings",  "Settings",   "设置"},
        {"sidebar.storage.label", "Used space",  "已用空间"},
        {"sidebar.storage.desc",
         "You have used 80% of your\navailable space. Need more?",
         "你已使用 80% 的\n可用空间。需要更多？"},
        {"sidebar.upgrade",   "Upgrade plan", "升级套餐"},
        {"sidebar.search.placeholder", "Search folders…", "搜索文件夹…"},

        // mock folder titles（FolderItem.title 字段直接用作 key）
        {"folder.marketing",    "Marketing",      "市场营销"},
        {"folder.family",       "Family",         "家庭"},
        {"folder.personal",     "Personal",       "个人"},
        {"folder.family2",      "Family 2",       "家庭 2"},
        {"folder.phone_backup", "Phone backup",   "手机备份"},
        {"folder.company",      "Company photos", "公司照片"},
        {"folder.vacation",     "Vacation",       "假期"},
        {"folder.swatches",     "Fav Swatches",   "色板收藏"},
        {"folder.random",       "Random",         "随手"},
        {"folder.unicorn",      "Unicorn",        "独角兽"},
        {"folder.ideas",        "Ideas",          "灵感"},
        {"folder.drafts",       "Drafts",         "草稿"},

        // 空结果提示（搜索没匹配时）
        {"dash.empty.prefix", "No folders match \"", "未找到匹配 \""},
        {"dash.empty.suffix", "\"",                  "\" 的文件夹"},

        // window title
        {"window.title", "jtUI Folders", "jtUI Folders"},
    });
}

}  // namespace folders_app
