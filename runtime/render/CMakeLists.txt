add_library(hui_render STATIC src/render.cpp)
add_library(hui::render ALIAS hui_render)

target_include_directories(hui_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_render PUBLIC hui::foundation)
hui_configure_target(hui_render)
