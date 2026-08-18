#include "wshdbg/core/logging.hpp"

#include <chrono>
#include <fstream>
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

std::wstring format_line(
    LogLevel level,
    std::wstring_view component,
    std::wstring_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif

    std::wostringstream line;
    line << std::put_time(&local, L"%Y-%m-%d %H:%M:%S")
         << L" [" << level_name(level) << L"]"
         << L" [" << component << L"] " << message;
    return line.str();
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

void Logger::set_file(std::filesystem::path path) {
    std::scoped_lock lock(g_log_mutex);
    file_ = std::move(path);
}

void Logger::clear_file() {
    std::scoped_lock lock(g_log_mutex);
    file_.reset();
}

void Logger::write(LogLevel level, std::wstring_view component, std::wstring_view message) {
    std::scoped_lock lock(g_log_mutex);
    if (!enabled(level_, level)) return;

    const auto line = format_line(level, component, message);
    std::wcerr << line << L'\n';

    if (file_) {
        std::wofstream out(*file_, std::ios::app);
        if (out) {
            out << line << L'\n';
        }
    }
}

} // namespace wshdbg
