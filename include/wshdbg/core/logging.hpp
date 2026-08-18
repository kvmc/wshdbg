#pragma once

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

    void write(LogLevel level, std::wstring_view component, std::wstring_view message);

private:
    Logger() = default;
    LogLevel level_{LogLevel::Error};
};

} // namespace wshdbg
