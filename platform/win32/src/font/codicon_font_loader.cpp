// Package: hui::platform::win32
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// IDWriteFactory5::CreateInMemoryFontFileLoader 路径实现。
//
// 流程:
//   1. QueryInterface 拿 IDWriteFactory5
//   2. CreateInMemoryFontFileLoader  ->  IDWriteInMemoryFontFileLoader
//   3. RegisterFontFileLoader        注册到 factory
//   4. CreateInMemoryFontFileReference(blob_ptr, blob_size)  ->  IDWriteFontFile
//   5. CreateFontSetBuilder          ->  IDWriteFontSetBuilder1
//   6. AddFontFile(fontFile)         加入字体文件
//   7. CreateFontSet                 ->  IDWriteFontSet
//   8. CreateFontCollectionFromFontSet  ->  IDWriteFontCollection1 (-> IDWriteFontCollection)
//
// 注: dwrite_3.h 提供 IDWriteFactory5 / IDWriteFontSet / IDWriteFontSetBuilder1 等;
// 当前 Win 10 SDK 默认包含。MinGW-w64 11+ 也有这些头。
#if defined(_WIN32)

#include "hui/platform/win32/codicon_font_loader.hpp"

#include <atomic>
#include <dwrite_3.h>

#include "hui/foundation/codicon_font.hpp"
#include "hui/platform/win32/com_ptr.hpp"

namespace hui::platform::win32 {

namespace {

using hui::detail::ComPtr;

// 全局状态。register 成功后持有 collection 强引用直到进程退出。
std::atomic<IDWriteFontCollection*> g_collection{nullptr};
std::atomic<bool> g_register_attempted{false};
std::atomic<bool> g_register_succeeded{false};

// 内部一次性注册流程。返回 collection, 失败返回 nullptr。
IDWriteFontCollection* try_register_impl(IDWriteFactory* base_factory) noexcept {
    if (base_factory == nullptr) {
        return nullptr;
    }

    // Step 1: 拿 IDWriteFactory5
    ComPtr<IDWriteFactory5> factory5;
    HRESULT hr = base_factory->QueryInterface(__uuidof(IDWriteFactory5),
                                              reinterpret_cast<void**>(&factory5));
    if (FAILED(hr) || !factory5) {
        return nullptr;
    }

    // Step 2: 创建 in-memory loader
    ComPtr<IDWriteInMemoryFontFileLoader> in_mem_loader;
    hr = factory5->CreateInMemoryFontFileLoader(&in_mem_loader);
    if (FAILED(hr) || !in_mem_loader) {
        return nullptr;
    }

    // Step 3: 注册 loader 到 factory
    hr = factory5->RegisterFontFileLoader(in_mem_loader.Get());
    if (FAILED(hr)) {
        return nullptr;
    }

    // Step 4: 从 codicon blob 创建 in-memory font file
    const auto blob = hui::foundation::codicon_font_blob();
    if (blob.data == nullptr || blob.size == 0) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }
    ComPtr<IDWriteFontFile> font_file;
    hr = in_mem_loader->CreateInMemoryFontFileReference(
        factory5.Get(), blob.data, static_cast<UINT32>(blob.size),
        /*ownerObject=*/nullptr, &font_file);
    if (FAILED(hr) || !font_file) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }

    // Step 5-6: builder + AddFontFile
    ComPtr<IDWriteFontSetBuilder1> set_builder;
    hr = factory5->CreateFontSetBuilder(&set_builder);
    if (FAILED(hr) || !set_builder) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }
    hr = set_builder->AddFontFile(font_file.Get());
    if (FAILED(hr)) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }

    // Step 7: CreateFontSet
    ComPtr<IDWriteFontSet> font_set;
    hr = set_builder->CreateFontSet(&font_set);
    if (FAILED(hr) || !font_set) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }

    // Step 8: CreateFontCollectionFromFontSet
    ComPtr<IDWriteFontCollection1> collection1;
    hr = factory5->CreateFontCollectionFromFontSet(font_set.Get(), &collection1);
    if (FAILED(hr) || !collection1) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }

    // 注: in-memory loader 和 fontFile 在 collection 内部被引用计数持有,
    // 我们这里释放 ComPtr 后它们仍然存活 (Release 走到 0 才销毁)。
    // collection1 直接转交所有权 -- 把指针 detach 出来给全局变量持有。
    IDWriteFontCollection* base_collection = nullptr;
    hr = collection1->QueryInterface(__uuidof(IDWriteFontCollection),
                                     reinterpret_cast<void**>(&base_collection));
    if (FAILED(hr) || base_collection == nullptr) {
        factory5->UnregisterFontFileLoader(in_mem_loader.Get());
        return nullptr;
    }
    return base_collection;
}

}  // namespace

bool codicon_register(IDWriteFactory* factory) noexcept {
    // 幂等: 已成功就直接返回 true; 已失败过也不再重试 (避免 RegisterFontFileLoader 重复操作 D2D 状态)。
    if (g_register_attempted.exchange(true, std::memory_order_acq_rel)) {
        return g_register_succeeded.load(std::memory_order_acquire);
    }

    IDWriteFontCollection* collection = try_register_impl(factory);
    if (collection == nullptr) {
        g_register_succeeded.store(false, std::memory_order_release);
        return false;
    }
    g_collection.store(collection, std::memory_order_release);
    g_register_succeeded.store(true, std::memory_order_release);
    return true;
}

IDWriteFontCollection* codicon_collection() noexcept {
    return g_collection.load(std::memory_order_acquire);
}

}  // namespace hui::platform::win32

#endif  // _WIN32
