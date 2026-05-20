#include "hui/foundation/log.hpp"

#include <atomic>
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace hui {

namespace {

std::atomic<bool> g_log_enabled{true};

// 文件输出：WIN32 子系统程序没有控制台，stderr / OutputDebugString 用户不易
// 看到，所以同时落一份 jtui.log（写到当前工作目录，通常是 exe 所在目录）。
// 首次 log_write 时以 truncate 模式打开 —— 每次运行清空，方便只看本次。
std::FILE* g_log_file = nullptr;
bool g_log_file_tried = false;

[[nodiscard]] std::FILE* log_file() {
    if (!g_log_file_tried) {
        g_log_file_tried = true;
        g_log_file = std::fopen("jtui.log", "w");
    }
    return g_log_file;
}

[[nodiscard]] const char* level_str(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug: return "D";
    case LogLevel::Info:  return "I";
    case LogLevel::Warn:  return "W";
    case LogLevel::Error: return "E";
    }
    return "?";
}

}  // namespace

void log_set_enabled(bool enabled) {
    g_log_enabled.store(enabled, std::memory_order_relaxed);
}

bool log_enabled() noexcept {
    return g_log_enabled.load(std::memory_order_relaxed);
}

void log_write(LogLevel level, const char* tag, const std::string& message) {
    if (!g_log_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    std::string line;
    line.reserve(message.size() + 32U);
    line += "[jtUI/";
    line += level_str(level);
    line += "][";
    line += (tag != nullptr ? tag : "-");
    line += "] ";
    line += message;
    line += '\n';

    std::fputs(line.c_str(), stderr);
    std::fflush(stderr);

    if (std::FILE* f = log_file(); f != nullptr) {
        std::fputs(line.c_str(), f);
        std::fflush(f);
    }

#if defined(_WIN32)
    OutputDebugStringA(line.c_str());
#endif
}

}  // namespace hui
