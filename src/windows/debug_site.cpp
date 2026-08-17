#include "debug_site.hpp"

namespace wshdbg::windows {

void DebugSiteBridge::script_loaded(const std::filesystem::path& script) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::ScriptLoaded,
        .script = script,
        .message = L"script loaded"
    });
}

void DebugSiteBridge::breakpoint_bound(BreakpointId id, SourceLocation location) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::BreakpointBound,
        .location = location,
        .breakpoint_id = id,
        .script = location.file,
        .message = L"breakpoint bound"
    });
}

void DebugSiteBridge::breakpoint_hit(std::optional<BreakpointId> id, SourceLocation location, std::wstring_view message) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::BreakpointHit,
        .location = location,
        .breakpoint_id = id,
        .script = location.file,
        .message = std::wstring{message}
    });
}

void DebugSiteBridge::step_complete(SourceLocation location) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::StepComplete,
        .location = location,
        .script = location.file,
        .message = L"step complete"
    });
}

void DebugSiteBridge::script_error(SourceLocation location, std::wstring_view message) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::ScriptError,
        .location = location,
        .script = location.file,
        .message = std::wstring{message}
    });
}

void DebugSiteBridge::process_stopped(std::wstring_view message) {
    session_.debugger_event(DebugEvent{
        .type = DebugEventType::ProcessStopped,
        .message = std::wstring{message}
    });
}

} // namespace wshdbg::windows
