add_library(hui_event STATIC
    src/event.cpp
    src/dispatcher.cpp
)
add_library(hui::event ALIAS hui_event)

target_include_directories(hui_event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hui_event PUBLIC hui::foundation hui::object)
hui_configure_target(hui_event)
