// Windows Media Foundation 解码器实现。
// 只在 WIN32 且 HUI_MEDIA_WIN32 打开时参与编译；Linux native 不需要。
//
// 视频：IMFSourceReader → MFVideoFormat_RGB32（RGBA 内存布局）→ 包装成 VideoFrame
// 音频：IMFSourceReader → MFAudioFormat_Float → 包装成 AudioBuffer
//
// 这部分在本机（Linux）无法运行验证，依赖 Windows 构建 + Wine / native Windows
// 冒烟。接口设计参考 MSDN《Source Reader》官方示例。

#if defined(_WIN32)

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "hui/media/decoder.hpp"
#include "hui/media/frame_types.hpp"
#include "hui/platform/win32/com_ptr.hpp"

#if defined(_MSC_VER)
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#endif

using hui::detail::ComPtr;

namespace hui {

namespace {

// MF 全局 ref count：多个 decoder 并存时只 startup/shutdown 一次。
std::mutex g_mf_mutex;
int g_mf_refcount = 0;

bool mf_startup() {
    std::lock_guard lock(g_mf_mutex);
    if (g_mf_refcount == 0) {
        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) return false;
    }
    ++g_mf_refcount;
    return true;
}

void mf_shutdown() {
    std::lock_guard lock(g_mf_mutex);
    if (g_mf_refcount > 0) {
        --g_mf_refcount;
        if (g_mf_refcount == 0) {
            MFShutdown();
        }
    }
}

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(static_cast<std::size_t>(len > 0 ? len - 1 : 0), L'\0');
    if (len > 0) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    }
    return w;
}

class MFVideoDecoder : public IVideoDecoder {
public:
    MFVideoDecoder() { mf_startup(); }
    ~MFVideoDecoder() override {
        close();
        mf_shutdown();
    }

    bool open(const std::string& path) override {
        close();

        const std::wstring url = utf8_to_wide(path);
        ComPtr<IMFAttributes> attrs;
        MFCreateAttributes(&attrs, 2);
        attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        // 关键：H.264 / HEVC 解码器原生输出 YUV（NV12 / I420），SetCurrentMediaType
        // 强行要求 RGB32 时 MF 必须自动插入一个色彩转换 MFT。这个 attr 是放行许可——
        // 不开 SetCurrentMediaType(RGB32) 会失败，整条 video 链路打不开，外部表现
        // 就是"点 play 没反应"。
        attrs->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

        HRESULT hr = MFCreateSourceReaderFromURL(url.c_str(), attrs, &reader_);
        if (FAILED(hr)) return false;

        // 禁掉非视频流
        reader_->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        reader_->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);

        // 要求 RGB32 输出（BGRX 顺序，32bpp）
        ComPtr<IMFMediaType> out_type;
        MFCreateMediaType(&out_type);
        out_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        out_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        hr = reader_->SetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, out_type);
        if (FAILED(hr)) {
            reader_.Release();
            return false;
        }

        // 取宽高 + 默认 stride（query 当前生效的 media type，含色彩转换后的输出）
        ComPtr<IMFMediaType> native_type;
        if (SUCCEEDED(reader_->GetCurrentMediaType(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &native_type))) {
            UINT32 w = 0;
            UINT32 h = 0;
            MFGetAttributeSize(native_type, MF_MT_FRAME_SIZE, &w, &h);
            width_ = static_cast<int>(w);
            height_ = static_cast<int>(h);

            // MF_MT_DEFAULT_STRIDE 可能不存在（很多输出类型不带），缺就用 width*4
            UINT32 stride_attr = 0;
            if (SUCCEEDED(native_type->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride_attr))) {
                default_stride_ = static_cast<LONG>(stride_attr);
            } else {
                default_stride_ = width_ * 4;
            }
        }

        // 总时长（100ns 单位 → 秒）
        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(reader_->GetPresentationAttribute(
                static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),
                MF_PD_DURATION, &var))) {
            duration_ = static_cast<double>(var.uhVal.QuadPart) / 10000000.0;
            PropVariantClear(&var);
        }

        open_ = true;
        return true;
    }

    void close() override {
        reader_.Release();
        open_ = false;
        width_ = height_ = 0;
        default_stride_ = 0;
        duration_ = 0.0;
    }

    [[nodiscard]] bool is_open() const override { return open_; }

    std::optional<VideoFrame> read_frame() override {
        if (!open_ || !reader_) return std::nullopt;

        DWORD stream_index = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        ComPtr<IMFSample> sample;
        HRESULT hr = reader_->ReadSample(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0,
            &stream_index, &flags, &timestamp, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) || !sample) {
            return std::nullopt;
        }

        ComPtr<IMFMediaBuffer> media_buf;
        if (FAILED(sample->ConvertToContiguousBuffer(&media_buf))) return std::nullopt;

        PixelBuffer pb = PixelBuffer::allocate(width_, height_, PixelFormat::BGRA8);

        // 优先用 IMF2DBuffer：它直接给出 scanline0 + pitch，pitch 可能是负值
        // （bottom-up 位图，RGB32 在 MF 里很常见）。负 pitch 时 scanline0 指向
        // 逻辑顶行（实际位于 buffer 高地址），按 pitch 步进就能直接 top-down 拿。
        // 不用 IMF2DBuffer 直接 Lock 拷贝整段会出现：(a) stride padding 时画面错位、
        // (b) bottom-up 时画面上下颠倒。
        ComPtr<IMF2DBuffer> buf2d;
        if (SUCCEEDED(media_buf->QueryInterface(
                IID_IMF2DBuffer, reinterpret_cast<void**>(&buf2d)))) {
            BYTE* scanline0 = nullptr;
            LONG pitch = 0;
            if (SUCCEEDED(buf2d->Lock2D(&scanline0, &pitch))) {
                const LONG abs_pitch = pitch < 0 ? -pitch : pitch;
                const LONG copy_per_row = std::min(static_cast<LONG>(pb.stride), abs_pitch);
                for (int row = 0; row < height_; ++row) {
                    const BYTE* src = scanline0 + static_cast<std::ptrdiff_t>(row) * pitch;
                    BYTE* dst = pb.data->data() + static_cast<std::ptrdiff_t>(row) * pb.stride;
                    std::memcpy(dst, src, static_cast<std::size_t>(copy_per_row));
                }
                buf2d->Unlock2D();
            } else {
                return std::nullopt;
            }
        } else {
            // 回退：连续 buffer，按 default_stride_ 解释。
            BYTE* ptr = nullptr;
            DWORD size = 0;
            if (FAILED(media_buf->Lock(&ptr, nullptr, &size))) return std::nullopt;
            const LONG src_stride = default_stride_ != 0 ? std::abs(default_stride_)
                                                         : pb.stride;
            const LONG copy_per_row = std::min(static_cast<LONG>(pb.stride), src_stride);
            const bool bottom_up = default_stride_ < 0;
            for (int row = 0; row < height_; ++row) {
                const int src_row = bottom_up ? (height_ - 1 - row) : row;
                const BYTE* src = ptr + static_cast<std::ptrdiff_t>(src_row) * src_stride;
                BYTE* dst = pb.data->data() + static_cast<std::ptrdiff_t>(row) * pb.stride;
                if (src + copy_per_row > ptr + size) break;
                std::memcpy(dst, src, static_cast<std::size_t>(copy_per_row));
            }
            media_buf->Unlock();
        }

        VideoFrame frame;
        frame.pixels = std::move(pb);
        frame.presentation_time = static_cast<double>(timestamp) / 10000000.0;
        return frame;
    }

    void seek(double seconds) override {
        if (!open_ || !reader_) return;
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = static_cast<LONGLONG>(seconds * 10000000.0);
        reader_->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
    }

    [[nodiscard]] double duration() const override { return duration_; }
    [[nodiscard]] int width() const override { return width_; }
    [[nodiscard]] int height() const override { return height_; }

private:
    ComPtr<IMFSourceReader> reader_;
    bool open_ {false};
    int width_ {0};
    int height_ {0};
    LONG default_stride_ {0};  // 来自 MF_MT_DEFAULT_STRIDE，可能是负值
    double duration_ {0.0};
};

class MFAudioDecoder : public IAudioDecoder {
public:
    MFAudioDecoder() { mf_startup(); }
    ~MFAudioDecoder() override {
        close();
        mf_shutdown();
    }

    bool open(const std::string& path) override {
        close();

        const std::wstring url = utf8_to_wide(path);
        HRESULT hr = MFCreateSourceReaderFromURL(url.c_str(), nullptr, &reader_);
        if (FAILED(hr)) return false;

        reader_->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        reader_->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);

        ComPtr<IMFMediaType> out_type;
        MFCreateMediaType(&out_type);
        out_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        out_type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
        out_type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
        hr = reader_->SetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, out_type);
        if (FAILED(hr)) {
            reader_.Release();
            return false;
        }

        ComPtr<IMFMediaType> current;
        if (SUCCEEDED(reader_->GetCurrentMediaType(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &current))) {
            UINT32 rate = 0;
            UINT32 ch = 0;
            current->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
            current->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &ch);
            sample_rate_ = static_cast<int>(rate);
            channels_ = static_cast<int>(ch);
        }

        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(reader_->GetPresentationAttribute(
                static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),
                MF_PD_DURATION, &var))) {
            duration_ = static_cast<double>(var.uhVal.QuadPart) / 10000000.0;
            PropVariantClear(&var);
        }

        open_ = true;
        return true;
    }

    void close() override {
        reader_.Release();
        open_ = false;
        sample_rate_ = channels_ = 0;
        duration_ = 0.0;
    }

    [[nodiscard]] bool is_open() const override { return open_; }

    std::optional<AudioBuffer> read_buffer() override {
        if (!open_ || !reader_) return std::nullopt;

        DWORD stream_index = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        ComPtr<IMFSample> sample;
        HRESULT hr = reader_->ReadSample(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0,
            &stream_index, &flags, &timestamp, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) || !sample) {
            return std::nullopt;
        }

        ComPtr<IMFMediaBuffer> media_buf;
        if (FAILED(sample->ConvertToContiguousBuffer(&media_buf))) return std::nullopt;

        BYTE* ptr = nullptr;
        DWORD size = 0;
        if (FAILED(media_buf->Lock(&ptr, nullptr, &size))) return std::nullopt;

        const std::size_t float_count = static_cast<std::size_t>(size) / sizeof(float);
        auto samples = std::make_shared<std::vector<float>>(float_count);
        std::memcpy(samples->data(), ptr, static_cast<std::size_t>(size));
        media_buf->Unlock();

        AudioBuffer buf;
        buf.samples = std::move(samples);
        buf.channels = channels_;
        buf.sample_rate = sample_rate_;
        buf.presentation_time = static_cast<double>(timestamp) / 10000000.0;
        return buf;
    }

    void seek(double seconds) override {
        if (!open_ || !reader_) return;
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = static_cast<LONGLONG>(seconds * 10000000.0);
        reader_->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
    }

    [[nodiscard]] double duration() const override { return duration_; }
    [[nodiscard]] int channels() const override { return channels_; }
    [[nodiscard]] int sample_rate() const override { return sample_rate_; }

private:
    ComPtr<IMFSourceReader> reader_;
    bool open_ {false};
    int sample_rate_ {0};
    int channels_ {0};
    double duration_ {0.0};
};

}  // namespace

namespace detail {
std::unique_ptr<IVideoDecoder> make_mf_video_decoder() {
    return std::make_unique<MFVideoDecoder>();
}
std::unique_ptr<IAudioDecoder> make_mf_audio_decoder() {
    return std::make_unique<MFAudioDecoder>();
}
}  // namespace detail

}  // namespace hui

#endif  // _WIN32
