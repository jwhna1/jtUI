# jtui_atlas —— jtUI Atlas 子品牌（森林深绿 + 金，"Watch every signal." 调性）
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_ATLAS_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_atlas WIN32 ${JTUI_ATLAS_SOURCES})
    if(MINGW)
        target_compile_options(jtui_atlas PRIVATE -municode)
        target_link_options(jtui_atlas PRIVATE -municode)
    endif()
else()
    add_executable(jtui_atlas ${JTUI_ATLAS_SOURCES})
endif()

target_include_directories(jtui_atlas PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    jtui_atlas
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_atlas)
