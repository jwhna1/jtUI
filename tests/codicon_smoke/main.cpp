// Package: jtUI tests
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// Codicon 字体加载端到端冒烟测试 (CLI)。
//
// 跑这个 exe 验证:
//   1. DirectWrite IDWriteFactory 创建 OK
//   2. codicon_register(factory) 注册流程 (Factory5 / InMemoryFontFileLoader
//      / FontSetBuilder1 / CreateFontCollectionFromFontSet) 成功返回 true
//   3. codicon_collection() 拿到非空 IDWriteFontCollection
//   4. collection 内确实有 family "codicon" (GetFontFamilyCount > 0,
//      FindFamilyName "codicon" 返回 exists=TRUE)
//
// 所有 printf 输出到 stdout, wine 跑也能看到。
//
// 这个 exe 不开窗口、不创 D2D, 是验证 codicon path 是否完整工作的最小桩。
// 业务功能 (StatusBar / Tree / Icon widget paint) 由 Commit #3+ 验证。
#include <cstdio>

#if defined(_WIN32)
#include <dwrite.h>
#include <windows.h>

#include "hui/foundation/codicon_font.hpp"
#include "hui/platform/win32/codicon_font_loader.hpp"

int main() {
    std::printf("[codicon-smoke] start\n");

    // 0. CoInitialize 不需要 (DirectWrite Factory 不依赖 STA / MTA, 与 D2D 一致)
    // 1. 创建 DWrite Factory
    IDWriteFactory* factory = nullptr;
    const HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&factory));
    if (FAILED(hr) || factory == nullptr) {
        std::printf("[codicon-smoke] FAIL DWriteCreateFactory hr=0x%08lX\n",
                    static_cast<unsigned long>(hr));
        return 1;
    }
    std::printf("[codicon-smoke] DWriteCreateFactory OK\n");

    // 2. 校验 blob 数据
    const auto blob = hui::foundation::codicon_font_blob();
    std::printf("[codicon-smoke] codicon_font_blob: data=%p size=%zu\n",
                blob.data, blob.size);
    if (blob.data == nullptr || blob.size == 0) {
        std::printf("[codicon-smoke] FAIL blob empty\n");
        factory->Release();
        return 2;
    }

    // 3. 注册 codicon
    const bool registered = hui::platform::win32::codicon_register(factory);
    std::printf("[codicon-smoke] codicon_register: %s\n",
                registered ? "OK" : "FAIL");
    if (!registered) {
        std::printf("[codicon-smoke] hint: 可能 IDWriteFactory5 不可用 (要求 Win10 1709+)\n");
        factory->Release();
        return 3;
    }

    // 4. collection 应该非空
    IDWriteFontCollection* collection = hui::platform::win32::codicon_collection();
    if (collection == nullptr) {
        std::printf("[codicon-smoke] FAIL codicon_collection() == nullptr\n");
        factory->Release();
        return 4;
    }
    const UINT32 family_count = collection->GetFontFamilyCount();
    std::printf("[codicon-smoke] collection family count = %u\n", family_count);

    // 5. 在 collection 里查找 "codicon" family
    UINT32 family_index = 0;
    BOOL exists = FALSE;
    const HRESULT find_hr =
        collection->FindFamilyName(L"codicon", &family_index, &exists);
    std::printf("[codicon-smoke] FindFamilyName(codicon): hr=0x%08lX index=%u exists=%d\n",
                static_cast<unsigned long>(find_hr), family_index,
                static_cast<int>(exists));

    // 6. 尝试用 codicon 创建 IDWriteTextFormat (业务最终路径)
    IDWriteTextFormat* fmt = nullptr;
    const HRESULT fmt_hr = factory->CreateTextFormat(
        L"codicon", collection, DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        16.0F, L"en-us", &fmt);
    std::printf("[codicon-smoke] CreateTextFormat(codicon,16): hr=0x%08lX fmt=%p\n",
                static_cast<unsigned long>(fmt_hr), static_cast<void*>(fmt));
    if (fmt != nullptr) {
        fmt->Release();
    }

    factory->Release();
    std::printf("[codicon-smoke] all checks finished\n");

    const bool all_ok = registered && collection != nullptr && family_count > 0
                        && exists == TRUE && SUCCEEDED(fmt_hr) && fmt_hr == S_OK;
    return all_ok ? 0 : 10;
}
#else
// 非 Win32: 没有 DirectWrite。仅做基本数据检查 (blob 字节非空)。
#include "hui/foundation/codicon_font.hpp"

int main() {
    const auto blob = hui::foundation::codicon_font_blob();
    std::printf("[codicon-smoke] non-Win32 stub. blob.size=%zu\n", blob.size);
    return blob.size > 0 ? 0 : 1;
}
#endif
