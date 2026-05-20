#pragma once

// folders_app 全局状态。用 jtUI Property<T> 承接，widget 订阅 changed() 自动响应。
// mock 数据流贯通的核心入口：sidebar 切 active、search 输入、folder 选中
// 全部写到这里，下游 widget 只读不藏副本。

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "jtui/jtui.hpp"

namespace folders_app {

// 单个文件夹的 mock 数据
// 注意：title 字段保存的是 i18n key（如 "folder.marketing"），渲染时 tr() 取
// 当前语言的字符串。这样 i18n 切换时 mock 文件夹名能跟着变（中文 "市场营销"）。
struct FolderItem {
    std::string title;    // i18n key，渲染时通过 jtui::i18n::tr() 取
    int photo_count;
    bool locked;          // 是否私密（右下角小锁）
    bool shared;          // 是否分享（右下角小头像组）
    int color_hint;       // 缩略图色彩 hint：0=灰、1=橙、2=洋红、3=青、4=紫
    std::string icon;     // codicon 名 (如 "device-camera"/"lock"/"flag"/"droplet")
};

// 侧边栏导航项
struct NavItem {
    std::string label;
    std::string icon;     // codicon 名
};

// 用户信息（mock）
struct UserProfile {
    std::string name;
    std::string status;   // "online" / "away"
};

class FoldersAppState {
public:
    // 主导航：active index
    jtui::Property<std::size_t>& active_nav() noexcept { return active_nav_; }

    // 搜索框文字（双向：search widget 写，folder grid 订阅过滤）
    jtui::Property<std::string>& search_query() noexcept { return search_query_; }

    // 选中的文件夹索引（-1 = 未选）
    jtui::Property<int>& selected_folder() noexcept { return selected_folder_; }

    // 用量进度 0..1（demo 里固定 0.8，可触发 Upgrade）
    jtui::Property<float>& used_space() noexcept { return used_space_; }

    [[nodiscard]] const std::vector<NavItem>& nav_items() const noexcept { return nav_items_; }
    [[nodiscard]] const std::vector<FolderItem>& folders() const noexcept { return folders_; }
    [[nodiscard]] const UserProfile& user() const noexcept { return user_; }

    FoldersAppState() {
        active_nav_.set(0);
        selected_folder_.set(-1);
        used_space_.set(0.8F);

        nav_items_ = {
            {"My Folders", "folder"},
            {"Favorite",   "star"},
            {"History",    "history"},
            {"Invite",     "person-add"},
        };

        // title 字段是 i18n key；渲染时 tr() 取当前语言字符串
        folders_ = {
            {"folder.marketing",     15,  false, false, 0, "device-camera"},
            {"folder.family",        325, false, true,  0, "heart"},
            {"folder.personal",      15,  true,  false, 0, "lock"},
            {"folder.family2",       142, false, true,  1, "device-camera"},
            {"folder.phone_backup",  5,   true,  false, 0, "device-camera"},
            {"folder.company",       84,  false, true,  3, "heart"},
            {"folder.vacation",      25,  false, false, 1, "flag"},
            {"folder.swatches",      3,   false, false, 2, "droplet"},
            {"folder.random",        12,  false, false, 0, "question"},
            {"folder.unicorn",       7,   false, false, 4, "rocket"},
            {"folder.ideas",         21,  false, false, 1, "lightbulb"},
            {"folder.drafts",        9,   false, false, 0, "edit"},
        };

        user_ = {"Olivia Rhye", "online"};
    }

private:
    jtui::Property<std::size_t> active_nav_ {};
    jtui::Property<std::string> search_query_ {std::string{}};
    jtui::Property<int> selected_folder_ {};
    jtui::Property<float> used_space_ {};

    std::vector<NavItem> nav_items_;
    std::vector<FolderItem> folders_;
    UserProfile user_;
};

}  // namespace folders_app
