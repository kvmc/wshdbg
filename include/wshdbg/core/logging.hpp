#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace wshdbg {

enum class LogLevel {
    Off,
    Error,
    Debug,
    Trace
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    LogLevel level() const;

    void set_file(std::filesystem::path path);
    void clear_file();
    const std::optional<std::filesystem::path>& file() const noexcept { return file_; }

    void write(LogLevel level, std::wstring_view component, std::wstring_view message);

private:
    Logger() = default;
    LogLevel level_{LogLevel::Error};
    std::optional<std::filesystem::path> file_;
};

} // namespace wshdbg
