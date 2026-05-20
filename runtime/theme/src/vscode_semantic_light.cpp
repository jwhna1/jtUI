// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// VS Code Default Light+ 主题色映射。色值来源：VS Code 官方 defaults
// (vs/platform/theme/common/colorRegistry.ts) 中 themable color 的默认值。
#include "hui/theme/vscode_tokens.hpp"

namespace hui::theme::vscode {

namespace {

constexpr VsSemantic make_light() {
    return VsSemantic {
        // StatusBar  -- Light+ 下 statusBar 仍保持 #007ACC 蓝，与 dark 一致
        .statusbar_bg              = Color::from_hex("#007ACC"),         // blue_06
        .statusbar_fg              = Color::from_hex("#FFFFFF"),
        .statusbar_no_folder_bg    = Color::from_hex("#68217A"),         // purple_04
        .statusbar_no_folder_fg    = Color::from_hex("#FFFFFF"),
        .statusbar_item_hover_bg   = Color::from_rgba8(255U, 255U, 255U, 31U),
        .statusbar_prominent_bg    = Color::from_rgba8(0U, 0U, 0U, 80U),
        .statusbar_prominent_fg    = Color::from_hex("#FFFFFF"),

        // ActivityBar  -- Light+ activityBar.background #2C2C2C (深色，对比强烈)
        .activitybar_bg            = Color::from_hex("#2C2C2C"),
        .activitybar_fg_active     = Color::from_hex("#FFFFFF"),
        .activitybar_fg_inactive   = Color::from_rgba8(255U, 255U, 255U, 102U),
        .activitybar_active_border = Color::from_hex("#FFFFFF"),
        .activitybar_badge_bg      = Color::from_hex("#007ACC"),
        .activitybar_badge_fg      = Color::from_hex("#FFFFFF"),

        // SideBar  -- Light+ sideBar.background #F3F3F3
        .sidebar_bg                = Color::from_hex("#F3F3F3"),
        .sidebar_fg                = Color::from_hex("#3C3C3C"),         // base_14
        .sidebar_section_header_bg = Color::from_rgba8(0U, 0U, 0U, 0U),
        .sidebar_section_header_fg = Color::from_hex("#6F6F6F"),         // 接近 base_11/12
        .sidebar_border            = Color::from_hex("#F3F3F3"),

        // List / Tree
        .list_hover_bg               = Color::from_hex("#E8E8E8"),
        .list_hover_fg               = Color::from_hex("#000000"),
        .list_active_selection_bg    = Color::from_hex("#0060C0"),
        .list_active_selection_fg    = Color::from_hex("#FFFFFF"),
        .list_inactive_selection_bg  = Color::from_hex("#E4E6F1"),       // gray_01
        .list_inactive_selection_fg  = Color::from_hex("#000000"),
        .list_focus_outline          = Color::from_hex("#0090F1"),
        .list_drop_bg                = Color::from_hex("#D6EBFF"),

        // Notifications
        .notification_bg          = Color::from_hex("#FFFFFF"),          // base_01
        .notification_fg          = Color::from_hex("#616161"),
        .notification_border      = Color::from_hex("#E5E5E5"),          // base_04
        .notification_info_icon   = Color::from_hex("#1A85FF"),
        .notification_warn_icon   = Color::from_hex("#BF8803"),          // yellow_04
        .notification_error_icon  = Color::from_hex("#E51400"),
        .notification_link        = Color::from_hex("#006AB1"),
    };
}

constexpr VsSemantic kLight = make_light();

}  // namespace

const VsSemantic& semantic_light() noexcept {
    return kLight;
}

}  // namespace hui::theme::vscode
