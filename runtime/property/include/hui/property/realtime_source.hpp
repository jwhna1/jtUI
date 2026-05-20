#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

// jtUI 高频数据通道
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）
//
// spec §14.3 / bootstrap §20：producer 在任意线程 publish/push，consumer
// （典型场景：主线程 paint / tick）取 snapshot()/latest()/pop()。publish 不触发
// mark_dirty —— 高频流量（音视频解码 / 传感器 60-100Hz 数据）轰炸主 dirty
// 系统会让整树扫描和 paint 失效不停被唤醒。
//
// 约定：每个 RealtimeSource<T> 暴露单调递增的 atomic generation()，每次 publish
// 自增。consumer widget 在 tick 里比较与上一次记下的 generation，不等就 mark
// 自己 paint dirty。这一对约定让 widget 零侵入接入任何 producer，且 widget
// 自己决定 sample 频率（典型 60Hz tick = 自动限流到 60fps，不管 producer 多快）。

namespace hui {

template <typename T>
class RealtimeSource {
public:
    virtual ~RealtimeSource() = default;
    [[nodiscard]] virtual std::optional<T> latest() const = 0;
    [[nodiscard]] virtual std::uint64_t generation() const noexcept = 0;
};

// Single-slot 最新值。VideoView 这类"只看最新一帧"的 consumer 用。用 mutex
// 保护拷贝避免 producer 写到一半 consumer 读到撕裂 T。T 不要求 trivially copyable。
template <typename T>
class LatestValue : public RealtimeSource<T> {
public:
    void publish(T value) {
        {
            std::lock_guard lock(mutex_);
            value_ = std::move(value);
            has_value_ = true;
        }
        gen_.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] std::optional<T> latest() const override {
        std::lock_guard lock(mutex_);
        if (!has_value_) {
            return std::nullopt;
        }
        return value_;
    }

    [[nodiscard]] std::uint64_t generation() const noexcept override {
        return gen_.load(std::memory_order_acquire);
    }

    void clear() {
        {
            std::lock_guard lock(mutex_);
            has_value_ = false;
        }
        gen_.fetch_add(1, std::memory_order_release);
    }

private:
    mutable std::mutex mutex_;
    T value_{};
    bool has_value_{false};
    std::atomic<std::uint64_t> gen_{0};
};

// 环形缓冲：保留最近 N 个值。WaveformView / Timeline 这类要"近期窗口"的 consumer 用。
// generation() 用 push 计数 —— 即便 buffer 满之后 push 覆盖旧值也照样自增。
template <typename T>
class RealtimeRingBuffer : public RealtimeSource<T> {
public:
    explicit RealtimeRingBuffer(std::size_t capacity)
        : buffer_(capacity), capacity_(capacity) {}

    void push(T value) {
        {
            std::lock_guard lock(mutex_);
            if (capacity_ == 0) {
                return;
            }
            buffer_[write_ % capacity_] = std::move(value);
            ++write_;
            if (write_ - read_ > capacity_) {
                // 覆盖旧数据；读指针追上来
                read_ = write_ - capacity_;
            }
        }
        gen_.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] std::optional<T> pop() {
        std::lock_guard lock(mutex_);
        if (read_ >= write_) {
            return std::nullopt;
        }
        T value = std::move(buffer_[read_ % capacity_]);
        ++read_;
        return value;
    }

    [[nodiscard]] std::optional<T> latest() const override {
        std::lock_guard lock(mutex_);
        if (read_ >= write_) {
            return std::nullopt;
        }
        return buffer_[(write_ - 1U) % capacity_];
    }

    [[nodiscard]] std::uint64_t generation() const noexcept override {
        return gen_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard lock(mutex_);
        return write_ - read_;
    }

    void clear() {
        {
            std::lock_guard lock(mutex_);
            read_ = write_ = 0;
        }
        gen_.fetch_add(1, std::memory_order_release);
    }

private:
    mutable std::mutex mutex_;
    std::vector<T> buffer_;
    std::size_t capacity_;
    std::size_t read_{0};
    std::size_t write_{0};
    std::atomic<std::uint64_t> gen_{0};
};

}  // namespace hui
