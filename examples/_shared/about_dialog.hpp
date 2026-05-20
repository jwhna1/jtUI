#pragma once

// jtUI About 对话框 facade —— v1.4 起实际实现已搬到 jtUI 主框架
// (widgets/common/include/hui/widgets/common/about_card.hpp)。
// 本文件保留作为旧业务侧 namespace jtui_about::* 调用兼容层。
//
// 新业务（包括 gallery）建议直接用 hui:: namespace 路径调用：
//   hui::install_about_card(root, open, hui::palette_to_about(brand::active()),
//                           on_close, w, h);
//
// 维护：曾能混 <jwhna1@gmail.com>

#include "hui/widgets/common/about_card.hpp"

namespace jtui_about {

using AboutColors = hui::AboutColors;

template <typename PaletteT>
[[nodiscard]] inline AboutColors palette_to_about(const PaletteT& p) {
    return hui::palette_to_about(p);
}

inline void register_about_strings() {
    hui::register_about_i18n();
}

inline hui::Panel* build_about_modal(
    hui::Panel& root,
    bool open,
    const AboutColors& c,
    std::function<void()> on_close,
    float window_w,
    float window_h)
{
    return hui::install_about_card(root, open, c, std::move(on_close),
                                   window_w, window_h);
}

}  // namespace jtui_about
