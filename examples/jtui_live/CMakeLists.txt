# jtui_live —— jtUI Live 子产品 hero example（钢青 accent，"Edit Code. Reload in Seconds." 调性）
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_LIVE_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_live WIN32 ${JTUI_LIVE_SOURCES})
    if(MINGW)
        target_compile_options(jtui_live PRIVATE -municode)
        target_link_options(jtui_live PRIVATE -municode)
    endif()
else()
    add_executable(jtui_live ${JTUI_LIVE_SOURCES})
endif()

target_include_directories(jtui_live PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    jtui_live
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_live)
