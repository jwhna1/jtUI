add_library(hui_widgets_basic STATIC
    src/basic.cpp
    src/flex_box.cpp
)
add_library(hui::widgets_basic ALIAS hui_widgets_basic)

target_include_directories(hui_widgets_basic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
    hui_widgets_basic
    PUBLIC
        hui::foundation
        hui::object
        hui::property
        hui::theme
        hui::layout
        hui::render
)
hui_configure_target(hui_widgets_basic)
