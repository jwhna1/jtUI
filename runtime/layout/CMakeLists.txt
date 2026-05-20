add_library(hui_layout STATIC src/layout.cpp)
add_library(hui::layout ALIAS hui_layout)

target_include_directories(hui_layout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_layout PUBLIC hui::foundation)
hui_configure_target(hui_layout)
