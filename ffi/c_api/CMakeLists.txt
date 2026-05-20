add_library(hui_c_api STATIC src/hui.cpp)
add_library(hui::c_api ALIAS hui_c_api)

target_include_directories(hui_c_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_c_api PUBLIC hui::app)
hui_configure_target(hui_c_api)
