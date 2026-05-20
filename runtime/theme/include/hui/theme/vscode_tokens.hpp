// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#pragma once

#include "hui/foundation/color.hpp"

namespace hui::theme::vscode {

// VS Code 风格调色板。与 Tailwind palette (color_tokens.hpp) 完全独立，
// 服务于 widgets/ide、widgets/data、widgets/feedback 三组新组件。
// 现有 widgets/common / widgets/basic 不受影响，仍走 SemanticColor。
//
// 阶梯设计完整复刻 VS Code 官方 Type/Colors/Icons spec：
//   Base 21 级灰阶 (FFFFFF -> 000000)
//   Accents: Blue 11, Red 4, Green 5, Purple 4, Yellow 5, Orange 3, Gray 3
//   Seti 11 种文件类型色
//
// 取值规则：编译期 constexpr，与现有 Palette 同种风格。

struct VsBase {
    Color base_01;  // #FFFFFF
    Color base_02;  // #F0F0F0
    Color base_03;  // #E7E7E7
    Color base_04;  // #E5E5E5
    Color base_05;  // #D4D4D4
    Color base_06;  // #CCCCCC
    Color base_07;  // #C6C6C6
    Color base_08;  // #BBBBBB
    Color base_09;  // #A0A0A0
    Color base_10;  // #808080
    Color base_11;  // #7F7F7F
    Color base_12;  // #606060
    Color base_13;  // #454545
    Color base_14;  // #3C3C3C
    Color base_15;  // #3A3D41
    Color base_16;  // #333333
    Color base_17;  // #303031
    Color base_18;  // #292929
    Color base_19;  // #252526
    Color base_20;  // #1E1E1E
    Color base_21;  // #000000
};

struct VsAccentBlue {
    Color blue_01;  // #75BEFF
    Color blue_02;  // #40A6FF
    Color blue_03;  // #3399CC
    Color blue_04;  // #3794FF
    Color blue_05;  // #0097FB
    Color blue_06;  // #007ACC  status bar / activity badge 主用色
    Color blue_07;  // #0E639C
    Color blue_08;  // #264F78
    Color blue_09;  // #094771  list 选中底色
    Color blue_10;  // #062F4A
    Color blue_11;  // #001F33
};

struct VsAccentRed {
    Color red_01;  // #F14C4C  notification 错误图标
    Color red_02;  // #C74E39
    Color red_03;  // #FF0000
    Color red_04;  // #264F78  注：PDF 写的就是 #264F78（与 blue_08 重复，疑似官方笔误，
                   //         保留 spec 原值。需要 danger 色时优先用 red_01/red_02）
};

struct VsAccentGreen {
    Color green_01;  // #73C991
    Color green_02;  // #40C8AE
    Color green_03;  // #16825D
    Color green_04;  // #327E36
    Color green_05;  // #28632B
};

struct VsAccentPurple {
    Color purple_01;  // #6C6CC4
    Color purple_02;  // #B180D7
    Color purple_03;  // #BC3FBC
    Color purple_04;  // #68217A  status bar no-folder 底色
};

struct VsAccentYellow {
    Color yellow_01;  // #D7BA7D
    Color yellow_02;  // #CCA700  notification 警告图标
    Color yellow_03;  // #B89500
    Color yellow_04;  // #BF8803
    Color yellow_05;  // #FFFF00
};

struct VsAccentOrange {
    Color orange_01;  // #CC6633
    Color orange_02;  // #EE9D28
    Color orange_03;  // #EA5C00
};

struct VsAccentGray {
    Color gray_01;  // #E4E6F1  light list inactive selection
    Color gray_02;  // #5F6A79
    Color gray_03;  // #424750
};

struct VsAccent {
    VsAccentBlue blue;
    VsAccentRed red;
    VsAccentGreen green;
    VsAccentPurple purple;
    VsAccentYellow yellow;
    VsAccentOrange orange;
    VsAccentGray gray;
};

// Seti UI 文件类型色。供 Tree 等数据视图按文件类型上色。
// 颜色 key 参考 seti-ui 官方 _fonts/seti/seti.ttf 配色映射。
struct VsSeti {
    Color blue;    // #519ABA  ts / json / config
    Color green;   // #8DC149  vue / html / less
    Color orange;  // #E37933  java / build
    Color pink;    // #F55385  css / sass
    Color red;     // #CC3E44  git / lock
    Color steel;   // #7494A3  py / sql
    Color yellow;  // #CBCB41  js / babel
    Color purple;  // #A074C4  go / rust
    Color ignore;  // #41535B  gitignore / npmrc
    Color white;   // #D4D7D6  text / md
    Color gray;    // #6D8086  binary / lock
};

struct VsPalette {
    VsBase base;
    VsAccent accent;
    VsSeti seti;
};

// 派生语义层 —— 复刻 VS Code 官方 Default Dark+ / Light+ 主题中
// 高频组件 (StatusBar / ActivityBar / SideBar / List / Notifications) 的色彩契约。
// 未来扩展 (Tabs / EditorGroup / Breadcrumb 等) 直接在此结构体追加字段。
struct VsSemantic {
    // StatusBar (底部状态条)
    Color statusbar_bg;
    Color statusbar_fg;
    Color statusbar_no_folder_bg;
    Color statusbar_no_folder_fg;
    Color statusbar_item_hover_bg;
    Color statusbar_prominent_bg;
    Color statusbar_prominent_fg;

    // ActivityBar (左侧主导航纵条)
    Color activitybar_bg;
    Color activitybar_fg_active;
    Color activitybar_fg_inactive;
    Color activitybar_active_border;
    Color activitybar_badge_bg;
    Color activitybar_badge_fg;

    // SideBar (左侧次级面板)
    Color sidebar_bg;
    Color sidebar_fg;
    Color sidebar_section_header_bg;
    Color sidebar_section_header_fg;
    Color sidebar_border;

    // List / Tree
    Color list_hover_bg;
    Color list_hover_fg;
    Color list_active_selection_bg;
    Color list_active_selection_fg;
    Color list_inactive_selection_bg;
    Color list_inactive_selection_fg;
    Color list_focus_outline;
    Color list_drop_bg;

    // Notifications
    Color notification_bg;
    Color notification_fg;
    Color notification_border;
    Color notification_info_icon;
    Color notification_warn_icon;
    Color notification_error_icon;
    Color notification_link;
};

// Palette 与 ThemeMode 无关 (色阶固定)，VsSemantic 与 ThemeMode 相关。
// palette() 返回全局唯一实例。semantic() 跟随 Theme::mode() 切换。
[[nodiscard]] const VsPalette& palette() noexcept;
[[nodiscard]] const VsSemantic& semantic() noexcept;

// 直接拿指定模式的语义色 (调试 / gallery 多主题对比预览用)。
[[nodiscard]] const VsSemantic& semantic_dark() noexcept;
[[nodiscard]] const VsSemantic& semantic_light() noexcept;

}  // namespace hui::theme::vscode
