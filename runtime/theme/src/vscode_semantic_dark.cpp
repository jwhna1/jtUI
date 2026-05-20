// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// VS Code Default Dark+ 主题色映射。色值来源：VS Code 官方 defaults
// (vs/platform/theme/common/colorRegistry.ts) 中 themable color 的默认值，
// 对应到本文件结构体字段。
#include "hui/theme/vscode_tokens.hpp"

namespace hui::theme::vscode {

namespace {

constexpr VsSemantic make_dark() {
    return VsSemantic {
        // StatusBar  -- statusBar.background #007ACC / fg #FFFFFF
        .statusbar_bg              = Color::from_hex("#007ACC"),         // blue_06
        .statusbar_fg              = Color::from_hex("#FFFFFF"),         // base_01
        .statusbar_no_folder_bg    = Color::from_hex("#68217A"),         // purple_04
        .statusbar_no_folder_fg    = Color::from_hex("#FFFFFF"),
        .statusbar_item_hover_bg   = Color::from_rgba8(255U, 255U, 255U, 31U),   // rgba(1,1,1,0.12)
        .statusbar_prominent_bg    = Color::from_rgba8(0U, 0U, 0U, 128U),        // 半透黑
        .statusbar_prominent_fg    = Color::from_hex("#FFFFFF"),

        // ActivityBar  -- activityBar.background #333333
        .activitybar_bg            = Color::from_hex("#333333"),         // base_16
        .activitybar_fg_active     = Color::from_hex("#FFFFFF"),
        .activitybar_fg_inactive   = Color::from_rgba8(255U, 255U, 255U, 102U),  // 0.4 alpha
        .activitybar_active_border = Color::from_hex("#FFFFFF"),
        .activitybar_badge_bg      = Color::from_hex("#007ACC"),         // blue_06
        .activitybar_badge_fg      = Color::from_hex("#FFFFFF"),

        // SideBar  -- sideBar.background #252526
        .sidebar_bg                = Color::from_hex("#252526"),         // base_19
        .sidebar_fg                = Color::from_hex("#CCCCCC"),         // base_06
        .sidebar_section_header_bg = Color::from_rgba8(0U, 0U, 0U, 0U),  // 透明
        .sidebar_section_header_fg = Color::from_hex("#FFFFFF"),
        .sidebar_border            = Color::from_hex("#252526"),

        // List / Tree
        .list_hover_bg               = Color::from_hex("#2A2D2E"),
        .list_hover_fg               = Color::from_hex("#FFFFFF"),
        .list_active_selection_bg    = Color::from_hex("#094771"),       // blue_09
        .list_active_selection_fg    = Color::from_hex("#FFFFFF"),
        .list_inactive_selection_bg  = Color::from_hex("#37373D"),
        .list_inactive_selection_fg  = Color::from_hex("#CCCCCC"),
        .list_focus_outline          = Color::from_hex("#007FD4"),
        .list_drop_bg                = Color::from_hex("#062F4A"),       // blue_10

        // Notifications
        .notification_bg          = Color::from_hex("#252526"),          // base_19
        .notification_fg          = Color::from_hex("#CCCCCC"),          // base_06
        .notification_border      = Color::from_hex("#303031"),          // base_17
        .notification_info_icon   = Color::from_hex("#75BEFF"),          // blue_01
        .notification_warn_icon   = Color::from_hex("#CCA700"),          // yellow_02
        .notification_error_icon  = Color::from_hex("#F14C4C"),          // red_01
        .notification_link        = Color::from_hex("#3794FF"),          // blue_04
    };
}

constexpr VsSemantic kDark = make_dark();

}  // namespace

const VsSemantic& semantic_dark() noexcept {
    return kDark;
}

}  // namespace hui::theme::vscode
