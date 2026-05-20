cmake_minimum_required(VERSION 3.25)

project(
    jtUI
    VERSION 1.0.0
    DESCRIPTION "jtUI - A C++ desktop UI framework with Vue-like simplicity"
    HOMEPAGE_URL "https://jtai.cc"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HUI_BUILD_EXAMPLES "Build example applications" ON)
option(HUI_BUILD_TESTS "Build test targets" ON)
option(HUI_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HuiWarnings)
include(HuiOptions)

hui_configure_global_options()

add_subdirectory(runtime/foundation)
add_subdirectory(runtime/object)
add_subdirectory(runtime/property)
add_subdirectory(runtime/theme)
add_subdirectory(runtime/layout)
add_subdirectory(runtime/event)
add_subdirectory(runtime/render)
add_subdirectory(runtime/media)

# Platform 层（spec §5）：Windows 平台特定实现。Linux 不参与编译。
if(WIN32)
    add_subdirectory(platform/win32)
endif()

add_subdirectory(widgets/basic)
add_subdirectory(widgets/common)
add_subdirectory(widgets/media)
add_subdirectory(runtime/app)
add_subdirectory(api)
add_subdirectory(ffi/c_api)

if(HUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(HUI_BUILD_EXAMPLES)
    add_subdirectory(examples/gallery)
    add_subdirectory(examples/folders_app)
    add_subdirectory(examples/jtui_hero)
    add_subdirectory(examples/jtui_live)
    add_subdirectory(examples/jtui_pro)
    add_subdirectory(examples/jtui_pulse)
    add_subdirectory(examples/jtui_studio)
    add_subdirectory(examples/jtui_atlas)
    add_subdirectory(examples/jtui_invest)
    add_subdirectory(examples/jtui_cinema)
endif()
