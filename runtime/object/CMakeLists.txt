add_library(hui_object STATIC src/object.cpp)
add_library(hui::object ALIAS hui_object)

target_include_directories(hui_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_object PUBLIC hui::foundation)
hui_configure_target(hui_object)
