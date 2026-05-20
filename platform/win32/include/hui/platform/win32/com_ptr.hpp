#pragma once

#if defined(_WIN32)

#include <unknwn.h>
#include <utility>

namespace hui::detail {

// 最小 COM RAII 包装。ATL 的 CComPtr 在 MSVC 下有，但 MinGW 不带 ATL，
// 所以自己写一个兼容两种工具链。只覆盖 mf_decoder / wasapi_audio 要用的方法。
template <typename T>
class ComPtr {
public:
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ~ComPtr() { Release(); }

    T* Get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // MF/WASAPI 许多 API 参数是 T*，隐式转换省去到处 .Get()。
    operator T*() const noexcept { return ptr_; }

    // 解引用 & 取址：MF/WASAPI 典型用法  func(&ptr)  —— 会先 Release 旧值。
    T** operator&() noexcept {
        Release();
        return &ptr_;
    }

    void Release() noexcept {
        if (ptr_) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

    HRESULT CoCreateInstance(REFCLSID rclsid,
                              DWORD ctx = CLSCTX_ALL,
                              IUnknown* outer = nullptr) noexcept {
        Release();
        return ::CoCreateInstance(rclsid, outer, ctx, __uuidof(T),
                                   reinterpret_cast<void**>(&ptr_));
    }

private:
    T* ptr_ {nullptr};
};

}  // namespace hui::detail

#endif  // _WIN32
