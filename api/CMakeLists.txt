# jtUI 公共 API facade
# 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）
#
# INTERFACE library，对外暴露 include/jtui/jtui.hpp 一个总入口头。host 应用 / examples
# / 未来 binding 层 link jtui::api 即可，无需关心 hui:: 内部模块的链接拓扑。
#
# 边界约束：jtui:: 命名空间 = 公共 API；hui:: 命名空间 = 内部实现。host 不该
# 在新代码里出现 #include "hui/..." 或 hui:: 限定符。

add_library(jtui_api INTERFACE)
add_library(jtui::api ALIAS jtui_api)

target_include_directories(jtui_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 把 hui:: 实际承载实现的所有公共 lib 一次性聚合：host link jtui::api 等同 link
# 整个 jtUI runtime + widgets。
target_link_libraries(jtui_api INTERFACE
    hui::foundation
    hui::object
    hui::property
    hui::theme
    hui::layout
    hui::event
    hui::render
    hui::media
    hui::widgets_basic
    hui::widgets_common
    hui::widgets_media
    hui::app
)
