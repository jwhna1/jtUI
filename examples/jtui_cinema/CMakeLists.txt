# jtui_cinema —— jtUI Cinema 子品牌（暖橘 accent，"World's First Native Cinematic UI." 调性）
# PDF p15 视频列表展示风格，3+ 张视频卡片横排 carousel + 真实点击播放 test.mp4
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_CINEMA_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_cinema WIN32 ${JTUI_CINEMA_SOURCES})
    if(MINGW)
        target_compile_options(jtui_cinema PRIVATE -municode)
        target_link_options(jtui_cinema PRIVATE -municode)
    endif()
else()
    add_executable(jtui_cinema ${JTUI_CINEMA_SOURCES})
endif()

target_include_directories(jtui_cinema PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)  # 让 #include "_shared/about_dialog.hpp" 生效

target_link_libraries(
    jtui_cinema
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_cinema)

# 把根目录下 test.mp4 拷到 exe 旁边；缺文件静默跳过（Linux 端无所谓）。
foreach(asset IN ITEMS test.mp4)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${asset}")
        add_custom_command(TARGET jtui_cinema POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${asset}"
                    "$<TARGET_FILE_DIR:jtui_cinema>/${asset}"
            VERBATIM)
    endif()
endforeach()
