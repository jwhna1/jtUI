set(GALLERY_SOURCES
    main.cpp
    sections/buttons.cpp
    sections/button_styles.cpp
    sections/inputs.cpp
    sections/gauges.cpp
    sections/video.cpp
    sections/audio.cpp
    sections/palette.cpp
    sections/vscode_palette.cpp
    sections/icons.cpp
)

if(WIN32)
    add_executable(gallery WIN32 ${GALLERY_SOURCES})
    if(MINGW)
        target_compile_options(gallery PRIVATE -municode)
        target_link_options(gallery PRIVATE -municode)
    endif()
else()
    add_executable(gallery ${GALLERY_SOURCES})
endif()

target_include_directories(gallery PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    gallery
    PRIVATE
        jtui::api
)

hui_configure_target(gallery)

# 把根目录下可选的演示素材拷到 gallery 可执行文件旁边，这样 Windows 运行时
# 无论在哪个目录双击都能读到。文件不存在时静默跳过（开发者没放素材也能跑）。
foreach(asset IN ITEMS test.mp4 test.mp3)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${asset}")
        add_custom_command(TARGET gallery POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${asset}"
                    "$<TARGET_FILE_DIR:gallery>/${asset}"
            VERBATIM)
    endif()
endforeach()

