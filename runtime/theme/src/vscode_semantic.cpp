// Package: hui::theme::vscode
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// VsSemantic 入口 -- 根据 Theme::mode() 派发到 dark / light 实现。
// 主题切换由 Theme::set() 触发，Theme::on_changed() 信号会让上层
// widget 自然重绘，不需要在此处缓存。
#include "hui/theme/theme.hpp"
#include "hui/theme/vscode_tokens.hpp"

namespace hui::theme::vscode {

const VsSemantic& semantic() noexcept {
    return Theme::mode() == ThemeMode::Dark ? semantic_dark() : semantic_light();
}

}  // namespace hui::theme::vscode
