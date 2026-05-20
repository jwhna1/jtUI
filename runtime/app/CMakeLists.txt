add_library(hui_app STATIC src/application.cpp)
add_library(hui::app ALIAS hui_app)

target_include_directories(hui_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
    hui_app
    PUBLIC
        hui::foundation
        hui::object
        hui::property
        hui::layout
        hui::event
        hui::render
        hui::media
)

if(WIN32)
    target_link_libraries(hui_app PRIVATE user32 gdi32 gdiplus dwmapi d2d1 dwrite winmm imm32)
    # Win32 平台层（媒体后端：MF + WASAPI）。link 在这里让 factory_win32.cpp 的强符号
    # 取代 runtime/media 里 null_decoder.cpp 用 #if !defined(_WIN32) 卡掉的 fallback。
    target_link_libraries(hui_app PUBLIC hui::platform_win32)
endif()

hui_configure_target(hui_app)
