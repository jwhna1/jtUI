# jtui_studio —— jtUI Studio 子品牌（葡萄紫 accent，"Build Media-Native. Native FAST." 调性）
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_STUDIO_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_studio WIN32 ${JTUI_STUDIO_SOURCES})
    if(MINGW)
        target_compile_options(jtui_studio PRIVATE -municode)
        target_link_options(jtui_studio PRIVATE -municode)
    endif()
else()
    add_executable(jtui_studio ${JTUI_STUDIO_SOURCES})
endif()

target_include_directories(jtui_studio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    jtui_studio
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_studio)

# 把根目录下 test.mp4 / test.mp3 拷到 exe 旁边；缺文件静默跳过（Linux 端无所谓）。
foreach(asset IN ITEMS test.mp4 test.mp3)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${asset}")
        add_custom_command(TARGET jtui_studio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${asset}"
                    "$<TARGET_FILE_DIR:jtui_studio>/${asset}"
            VERBATIM)
    endif()
endforeach()
