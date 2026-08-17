#pragma once

#include "wshdbg/core/types.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace wshdbg {

enum class DebugEventType {
    ScriptLoaded,
    BreakpointBound,
    BreakpointHit,
    StepComplete,
    ScriptError,
    ProcessStopped
};

struct DebugEvent {
    DebugEventType type{DebugEventType::ProcessStopped};
    std::optional<SourceLocation> location;
    std::optional<BreakpointId> breakpoint_id;
    std::filesystem::path script;
    std::wstring message;
    std::uint64_t native_thread_id{0};
};

} // namespace wshdbg
