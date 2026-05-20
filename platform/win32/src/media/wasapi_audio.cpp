// WASAPI 共享模式音频输出。只在 WIN32 编译。
// 共享模式足够覆盖 gallery 的演示需求；要做超低延迟专业音频等 v2 再上独占模式。
//
// 线程模型：submit() 由 widget tick 调用（主线程）。内部做阻塞写入的话会拖慢
// UI，所以 submit 只把样本推进内部 ring buffer，一个独立 thread 以 packet 尺寸
// 为单位从 ring buffer 抽样喂给 render client。thread 退出条件：析构或 close()。

#if defined(_WIN32)

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "hui/media/decoder.hpp"
#include "hui/media/frame_types.hpp"
#include "hui/platform/win32/com_ptr.hpp"

#if defined(_MSC_VER)
#pragma comment(lib, "ole32.lib")
#endif

using hui::detail::ComPtr;

namespace hui {

namespace {

constexpr REFERENCE_TIME kBufferDuration = 2 * 10000000LL / 10;  // 200ms

// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT 定义在 ksmedia.h 里，MinGW 下链接侧不带实体。
// 直接内联 GUID，避免跨头文件排序问题。GUID 值是 KS/WAVEFORMATEX 的公开标准。
static const GUID kSubtypeIEEEFloat = {
    0x00000003, 0x0000, 0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

class WASAPIAudioOutput : public IAudioOutput {
public:
    ~WASAPIAudioOutput() override { close(); }

    bool open(int sample_rate, int channels) override {
        close();

        CoInitializeEx(nullptr, COINIT_MULTITHREADED);  // 容忍已初始化

        ComPtr<IMMDeviceEnumerator> enumerator;
        HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
        if (FAILED(hr)) return false;

        ComPtr<IMMDevice> device;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr)) return false;

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&client_));
        if (FAILED(hr)) return false;

        // 共享模式下 Windows mixer 要求 WAVE_FORMAT_EXTENSIBLE + float32
        WAVEFORMATEXTENSIBLE fmt {};
        fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        fmt.Format.nChannels = static_cast<WORD>(channels);
        fmt.Format.nSamplesPerSec = static_cast<DWORD>(sample_rate);
        fmt.Format.wBitsPerSample = 32;
        fmt.Format.nBlockAlign = fmt.Format.nChannels * fmt.Format.wBitsPerSample / 8;
        fmt.Format.nAvgBytesPerSec = fmt.Format.nSamplesPerSec * fmt.Format.nBlockAlign;
        fmt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        fmt.Samples.wValidBitsPerSample = 32;
        fmt.dwChannelMask = channels == 2 ? 0x3 /*L|R*/ : 0x4 /*C*/;
        fmt.SubFormat = kSubtypeIEEEFloat;

        // AUTOCONVERTPCM + SRC_DEFAULT_QUALITY：让 WASAPI 在源采样率（如 44.1k）
        // 和系统引擎 mix（通常 48k）不一致时自动重采样。少了这个 flag，44.1k 的
        // mp3 在 48k 引擎上 Initialize 会直接失败，open() 返回 false，外部表现就是
        // 播不出声 / 卡顿（但我们的代码已经把 client_ 当成有效继续推数据）。
        constexpr DWORD kStreamFlags =
            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, kStreamFlags, kBufferDuration, 0,
                                 reinterpret_cast<WAVEFORMATEX*>(&fmt), nullptr);
        if (FAILED(hr)) return false;

        hr = client_->GetBufferSize(&buffer_frames_);
        if (FAILED(hr)) return false;

        hr = client_->GetService(__uuidof(IAudioRenderClient),
                                 reinterpret_cast<void**>(&render_client_));
        if (FAILED(hr)) return false;

        sample_rate_ = sample_rate;
        channels_ = channels;
        open_ = true;
        running_ = true;
        paused_ = false;

        thread_ = std::thread([this]() { thread_main(); });

        client_->Start();
        return true;
    }

    void close() override {
        running_ = false;
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
        if (client_) client_->Stop();
        render_client_.Release();
        client_.Release();
        queue_.clear();
        open_ = false;
    }

    [[nodiscard]] bool is_open() const override { return open_; }

    std::size_t submit(const AudioBuffer& buffer) override {
        if (!open_ || !buffer.valid()) return 0;
        {
            std::lock_guard lock(mutex_);
            queue_.emplace_back(buffer.samples);
        }
        cv_.notify_one();
        return buffer.frame_count();
    }

    void set_volume(float linear) override {
        volume_.store(linear < 0.0F ? 0.0F : (linear > 1.0F ? 1.0F : linear));
    }

    void pause() override { paused_ = true; }
    void resume() override { paused_ = false; cv_.notify_all(); }

private:
    void thread_main() {
        while (running_) {
            UINT32 padding = 0;
            if (!client_ || FAILED(client_->GetCurrentPadding(&padding))) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            const UINT32 available = buffer_frames_ - padding;
            if (available == 0 || paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            std::vector<float> interleaved;
            {
                std::unique_lock lock(mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(5),
                             [this] { return !queue_.empty() || !running_; });
                if (!running_) break;
                if (queue_.empty()) continue;
                // 凑够 available 帧；不够就把现有队列全吃了
                const std::size_t needed_samples = static_cast<std::size_t>(available)
                                                   * static_cast<std::size_t>(channels_);
                interleaved.reserve(needed_samples);
                while (!queue_.empty() && interleaved.size() < needed_samples) {
                    auto& front = queue_.front();
                    const std::size_t copy_samples = std::min(
                        front->size(), needed_samples - interleaved.size());
                    interleaved.insert(interleaved.end(), front->begin(),
                                       front->begin() + static_cast<std::ptrdiff_t>(copy_samples));
                    if (copy_samples == front->size()) {
                        queue_.pop_front();
                    } else {
                        front->erase(front->begin(),
                                     front->begin() + static_cast<std::ptrdiff_t>(copy_samples));
                    }
                }
            }

            if (interleaved.empty()) continue;

            const UINT32 frames_to_write = static_cast<UINT32>(
                interleaved.size() / static_cast<std::size_t>(channels_));
            BYTE* data = nullptr;
            if (FAILED(render_client_->GetBuffer(frames_to_write, &data))) continue;
            const float vol = volume_.load();
            float* dst = reinterpret_cast<float*>(data);
            for (std::size_t i = 0; i < interleaved.size(); ++i) {
                dst[i] = interleaved[i] * vol;
            }
            render_client_->ReleaseBuffer(frames_to_write, 0);
        }
    }

    ComPtr<IAudioClient> client_;
    ComPtr<IAudioRenderClient> render_client_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::shared_ptr<std::vector<float>>> queue_;
    std::atomic<bool> running_ {false};
    std::atomic<bool> paused_ {false};
    std::atomic<float> volume_ {1.0F};
    UINT32 buffer_frames_ {0};
    int sample_rate_ {0};
    int channels_ {0};
    bool open_ {false};
};

}  // namespace

namespace detail {
std::unique_ptr<IAudioOutput> make_wasapi_audio_output() {
    return std::make_unique<WASAPIAudioOutput>();
}
}  // namespace detail

}  // namespace hui

#endif  // _WIN32
