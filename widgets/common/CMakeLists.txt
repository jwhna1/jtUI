add_library(hui_widgets_common STATIC
    src/common.cpp
    src/button.cpp
    src/text_input.cpp
    src/dialog.cpp
    src/tabs.cpp
    src/gauge.cpp
)
add_library(hui::widgets_common ALIAS hui_widgets_common)

target_include_directories(hui_widgets_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
    hui_widgets_common
    PUBLIC
        hui::widgets_basic
        hui::property
        hui::theme
        hui::event
        hui::app
)
hui_configure_target(hui_widgets_common)
