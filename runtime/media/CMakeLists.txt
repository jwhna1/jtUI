add_library(hui_media STATIC
    src/media.cpp
    src/null_decoder.cpp
)
add_library(hui::media ALIAS hui_media)

target_include_directories(hui_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# realtime_source.hpp 已下沉到 hui::property 并通过转发头暴露，需要 PUBLIC link
# 让 widgets/media 等下游也能直接用 hui::property::RealtimeSource。
target_link_libraries(hui_media PUBLIC hui::foundation hui::property)

# 平台特定实现（Windows MF + WASAPI）已搬到 platform/win32/。runtime/media 这一层
# 只剩接口 + null fallback，runtime/app 在 WIN32 时另外 link hui::platform_win32
# 让强符号取胜。详见 platform/win32/CMakeLists.txt。

hui_configure_target(hui_media)
