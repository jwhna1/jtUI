add_library(
    hui_theme STATIC
    src/theme.cpp
    src/tokens_dark.cpp
    src/tokens_light.cpp
    src/vscode_palette.cpp
    src/vscode_semantic.cpp
    src/vscode_semantic_dark.cpp
    src/vscode_semantic_light.cpp
    src/vscode_typography.cpp
)
add_library(hui::theme ALIAS hui_theme)

target_include_directories(hui_theme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_theme PUBLIC hui::foundation hui::property)
hui_configure_target(hui_theme)
