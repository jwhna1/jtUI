# jtui_hero —— jtUI 自身品牌的 hero example（暖橘 accent，"Build native. Ship now." 调性）
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_HERO_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_hero WIN32 ${JTUI_HERO_SOURCES})
    if(MINGW)
        target_compile_options(jtui_hero PRIVATE -municode)
        target_link_options(jtui_hero PRIVATE -municode)
    endif()
else()
    add_executable(jtui_hero ${JTUI_HERO_SOURCES})
endif()

target_include_directories(jtui_hero PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)  # 让 #include "_shared/about_dialog.hpp" 生效

target_link_libraries(
    jtui_hero
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_hero)
