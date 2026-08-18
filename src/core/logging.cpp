#include "wshdbg/core/logging.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace wshdbg {
namespace {
std::mutex g_log_mutex;

const wchar_t* level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Off: return L"off";
    case LogLevel::Error: return L"error";
    case LogLevel::Debug: return L"debug";
    case LogLevel::Trace: return L"trace";
    }
    return L"unknown";
}

bool enabled(LogLevel configured, LogLevel event) noexcept {
    if (configured == LogLevel::Off || event == LogLevel::Off) return false;
    return static_cast<int>(event) <= static_cast<int>(configured);
}
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::set_level(LogLevel level) {
    std::scoped_lock lock(g_log_mutex);
    level_ = level;
}

LogLevel Logger::level() const {
    std::scoped_lock lock(g_log_mutex);
    return level_;
}

void Logger::write(LogLevel level, std::wstring_view component, std::wstring_view message) {
    std::scoped_lock lock(g_log_mutex);
    if (!enabled(level_, level)) return;

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif

    std::wcerr << std::put_time(&local, L"%Y-%m-%d %H:%M:%S")
               << L" [" << level_name(level) << L"]"
               << L" [" << component << L"] " << message << L'\n';
}

} // namespace wshdbg
