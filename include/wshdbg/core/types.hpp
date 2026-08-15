#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace wshdbg {

using BreakpointId = std::uint64_t;

struct SourceLocation {
    std::filesystem::path file;
    std::uint32_t line{1};
    std::uint32_t column{1};

    friend bool operator==(const SourceLocation&, const SourceLocation&) = default;
};

enum class BreakpointState { Pending, Bound, Disabled, Error };

struct Breakpoint {
    BreakpointId id{};
    SourceLocation location;
    std::optional<std::wstring> condition;
    BreakpointState state{BreakpointState::Pending};
    std::optional<std::wstring> diagnostic;
};

enum class SessionState { Created, Launching, Running, Paused, Stopping, Stopped, Failed };
enum class StopReason { Entry, Breakpoint, Step, Exception, Pause, ScriptError, Unknown };

struct StopInfo {
    StopReason reason{StopReason::Unknown};
    std::optional<SourceLocation> location;
    std::wstring description;
};

} // namespace wshdbg
