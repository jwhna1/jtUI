add_library(hui_foundation STATIC
    src/foundation.cpp
    src/embedded_codicon_font.cpp
    src/codicon_lookup.cpp
    src/i18n.cpp
    src/image_loader.cpp
    src/log.cpp
)
add_library(hui::foundation ALIAS hui_foundation)

target_include_directories(hui_foundation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# stb_image vendored header；只 image_loader.cpp 内部用，不暴露给 PUBLIC
target_include_directories(hui_foundation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
hui_configure_target(hui_foundation)
# stb_image 单文件库的 implementation 含 #pragma 类宽容 + 老式 K&R 风格代码，
# 关掉本 TU 的 -Wpedantic / -Wall / -Wextra 触发的若干无害告警，避免污染
# 全局 -Werror（如果将来开了）。
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(src/image_loader.cpp PROPERTIES
        COMPILE_FLAGS "-Wno-unused-but-set-variable -Wno-unused-parameter -Wno-implicit-fallthrough -Wno-cast-qual -Wno-unused-function -Wno-shift-negative-value -Wno-misleading-indentation")
endif()
