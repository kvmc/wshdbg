#pragma once

#include "wshdbg/core/session.hpp"
#include <filesystem>
#include <optional>
#include <string_view>

namespace wshdbg::windows {

// Thin translation boundary used by Active Scripting COM callbacks.  The COM
// implementation owns SDK-specific objects; this class converts their results
// into the platform-neutral debugger model consumed by the CLI and WinUI UI.
class DebugSiteBridge {
public:
    explicit DebugSiteBridge(DebugSession& session) noexcept : session_(session) {}

    void script_loaded(const std::filesystem::path& script);
    void breakpoint_bound(BreakpointId id, SourceLocation location);
    void breakpoint_hit(std::optional<BreakpointId> id, SourceLocation location, std::wstring_view message = {});
    void step_complete(SourceLocation location);
    void script_error(SourceLocation location, std::wstring_view message);
    void process_stopped(std::wstring_view message = {});

private:
    DebugSession& session_;
};

} // namespace wshdbg::windows
