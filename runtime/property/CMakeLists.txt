add_library(hui_property STATIC src/property.cpp)
add_library(hui::property ALIAS hui_property)

target_include_directories(hui_property PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_property PUBLIC hui::foundation)
hui_configure_target(hui_property)
