add_library(hui_widgets_media STATIC
    src/media.cpp
    src/audio_player.cpp
    src/video_player.cpp
)
add_library(hui::widgets_media ALIAS hui_widgets_media)

target_include_directories(hui_widgets_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
    hui_widgets_media
    PUBLIC
        hui::widgets_basic
        hui::media
        hui::theme
        hui::render
)
hui_configure_target(hui_widgets_media)
