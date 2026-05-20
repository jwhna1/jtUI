add_executable(hui_unit_tests unit_tests.cpp)

target_link_libraries(
    hui_unit_tests
    PRIVATE
        hui::foundation
        hui::object
        hui::property
        hui::theme
        hui::event
        hui::app
        hui::widgets_basic
        hui::widgets_common
        hui::widgets_media
)

# RealtimeSource 多线程测试用 std::thread。Linux 下需要 -pthread。
find_package(Threads REQUIRED)
target_link_libraries(hui_unit_tests PRIVATE Threads::Threads)

hui_configure_target(hui_unit_tests)

add_test(NAME hui_unit_tests COMMAND hui_unit_tests)

# Codicon 字体加载冒烟测试。Win32 下走真实 DirectWrite Factory5 + InMemoryFontFileLoader,
# Linux native 下走 blob 字节存在性检查 (stub)。MinGW 交叉编译产物可在 wine 跑。
add_executable(codicon_smoke codicon_smoke/main.cpp)
target_link_libraries(
    codicon_smoke
    PRIVATE
        hui::foundation
)
if(WIN32)
    target_link_libraries(codicon_smoke PRIVATE hui::platform_win32 dwrite)
endif()
hui_configure_target(codicon_smoke)
add_test(NAME codicon_smoke COMMAND codicon_smoke)
