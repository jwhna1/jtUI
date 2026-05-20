# jtUI Win32 平台层
# 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）
#
# spec §5 / §2 要求的 platform 层独立。本目录收纳所有 Windows 平台特定实现：
# 当前包含媒体后端（MF 解码器 + WASAPI 输出）。后续如果做 SDL3 / Skia 接入或
# 抽离 application.cpp 的 D2D 渲染代码，也搬到这里。
#
# 链接拓扑：runtime/app 在 WIN32 时 link hui::platform_win32，让 factory_win32.cpp
# 提供的 create_video_decoder / create_audio_decoder / create_audio_output 强符号
# 取代 runtime/media 里 null_decoder.cpp 用 #if !defined(_WIN32) 卡掉的 fallback。
# Linux 仍走 null_decoder 兜底，runtime/app 不 link 本目标。

add_library(hui_platform_win32 STATIC
    src/media/factory_win32.cpp
    src/media/mf_decoder.cpp
    src/media/wasapi_audio.cpp
    src/font/codicon_font_loader.cpp
)
add_library(hui::platform_win32 ALIAS hui_platform_win32)

target_include_directories(hui_platform_win32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 接口：从 hui::media 拿到 IVideoDecoder / IAudioDecoder / IAudioOutput 定义
# + 通过 PRIVATE link 看到 detail::make_null_*（factory_win32.cpp 在 MF 失败时回落）
target_link_libraries(hui_platform_win32 PUBLIC hui::media)

# Windows 系统库：MF + COM + 音频 + DirectWrite (codicon 字体加载)
target_link_libraries(hui_platform_win32 PUBLIC
    mfplat
    mfreadwrite
    mfuuid
    mf
    ole32
    dwrite
)

# codicon_font_loader 需要 hui::foundation 拿 codicon_font_blob()
target_link_libraries(hui_platform_win32 PUBLIC hui::foundation)

hui_configure_target(hui_platform_win32)
