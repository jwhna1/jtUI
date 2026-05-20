#pragma once

#include "hui/foundation/color.hpp"

namespace hui::theme {

// 阴影描述。spec §13.2 目前 PaintContext 还没 shadow 命令，
// 先把 token 结构定好，等绘制端支持后按这里参数渲染。
struct Shadow {
    float offset_x;
    float offset_y;
    float blur;
    float spread;
    Color color;
};

struct ElevationTokens {
    Shadow level_0;  // 扁平
    Shadow level_1;  // 轻浮起（button hover / input focus）
    Shadow level_2;  // 卡片
    Shadow level_3;  // popover / dropdown
    Shadow level_4;  // dialog
};

[[nodiscard]] const ElevationTokens& default_elevation_dark() noexcept;
[[nodiscard]] const ElevationTokens& default_elevation_light() noexcept;

}  // namespace hui::theme
