#pragma once

#include <string>

namespace jtui::gallery {

enum class Locale {
    English,
    Chinese,
};

// 对照表：left 是 English，right 是 Chinese。sections 代码里用 L(en, zh) 宏写一对。
inline std::string L(Locale loc, const char* en, const char* zh) {
    return loc == Locale::Chinese ? std::string{zh} : std::string{en};
}

struct SectionLayout {
    float x;      // section 内容区左上
    float y;
    float width;
    float height;
};

}  // namespace jtui::gallery
