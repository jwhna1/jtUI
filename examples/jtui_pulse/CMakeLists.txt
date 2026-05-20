# jtui_pulse —— jtUI Pulse 子产品 hero example（珊瑚红 #FB7185，"See what's happening, right now."）
# 维护：曾能混 <jwhna1@gmail.com>

set(JTUI_PULSE_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(jtui_pulse WIN32 ${JTUI_PULSE_SOURCES})
    if(MINGW)
        target_compile_options(jtui_pulse PRIVATE -municode)
        target_link_options(jtui_pulse PRIVATE -municode)
    endif()
else()
    add_executable(jtui_pulse ${JTUI_PULSE_SOURCES})
endif()

target_include_directories(jtui_pulse PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    jtui_pulse
    PRIVATE
        jtui::api
)

hui_configure_target(jtui_pulse)
