// Windows 平台工厂聚合：把 MF 解码器和 WASAPI 输出绑到公共 factory 函数上。
// 如果 MF/WASAPI 初始化失败，回落到 null 实现。

#if defined(_WIN32)

#include <memory>

#include "hui/media/decoder.hpp"

namespace hui {

namespace detail {
std::unique_ptr<IVideoDecoder> make_mf_video_decoder();
std::unique_ptr<IAudioDecoder> make_mf_audio_decoder();
std::unique_ptr<IAudioOutput> make_wasapi_audio_output();

std::unique_ptr<IVideoDecoder> make_null_video_decoder();
std::unique_ptr<IAudioDecoder> make_null_audio_decoder();
std::unique_ptr<IAudioOutput> make_null_audio_output();
}

std::unique_ptr<IVideoDecoder> create_video_decoder() {
    if (auto mf = detail::make_mf_video_decoder()) return mf;
    return detail::make_null_video_decoder();
}

std::unique_ptr<IAudioDecoder> create_audio_decoder() {
    if (auto mf = detail::make_mf_audio_decoder()) return mf;
    return detail::make_null_audio_decoder();
}

std::unique_ptr<IAudioOutput> create_audio_output() {
    if (auto wa = detail::make_wasapi_audio_output()) return wa;
    return detail::make_null_audio_output();
}

}  // namespace hui

#endif  // _WIN32
