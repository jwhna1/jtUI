#pragma once

// jtUI 轻量日志设施（v0.1，2026-05-15）。
//
// 目标：给框架 / example 埋点用 —— 关键路径（backdrop blur 截图、动画 tick、
// 布局测算）打一行带 tag 的日志，方便定位问题。
//
// 输出：stderr（mingw / Linux 终端可见）+ Windows 下同时 OutputDebugStringA
// （VS 输出窗口 / DebugView 可见）。tag 用来分类过滤，如 "backdrop"/"anim"。
//
// 维护：曾能混 <jwhna1@gmail.com>

#include <string>

namespace hui {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

// 写一行日志。tag 分类标签（如 "backdrop"），message 正文。
void log_write(LogLevel level, const char* tag, const std::string& message);

// 全局开关。release 构建可关掉做到零开销（除了一次 atomic load）。默认开。
void log_set_enabled(bool enabled);
[[nodiscard]] bool log_enabled() noexcept;

}  // namespace hui

// 便捷宏。message 可以是任意能拼进 std::string 的表达式。
#define HUI_LOG_D(tag, msg) ::hui::log_write(::hui::LogLevel::Debug, (tag), (msg))
#define HUI_LOG_I(tag, msg) ::hui::log_write(::hui::LogLevel::Info,  (tag), (msg))
#define HUI_LOG_W(tag, msg) ::hui::log_write(::hui::LogLevel::Warn,  (tag), (msg))
#define HUI_LOG_E(tag, msg) ::hui::log_write(::hui::LogLevel::Error, (tag), (msg))
