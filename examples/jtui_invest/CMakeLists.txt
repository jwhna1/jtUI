# jtui_invest —— jtUI Invest 金融投资子品牌落地页（视觉范式借鉴 桌面UI.pdf p20）
# 毛玻璃 NavBar + count-up / draw-in 动画 + ScrollView 长页面
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_INVEST_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_invest WIN32 ${JTUI_INVEST_SOURCES})
    if(MINGW)
        target_compile_options(jtui_invest PRIVATE -municode)
        target_link_options(jtui_invest PRIVATE -municode)
    endif()
else()
    add_executable(jtui_invest ${JTUI_INVEST_SOURCES})
endif()

target_include_directories(jtui_invest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    jtui_invest
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_invest)
