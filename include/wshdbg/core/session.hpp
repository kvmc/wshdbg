#pragma once
#include "wshdbg/core/breakpoint_store.hpp"
#include "wshdbg/core/debug_event.hpp"
#include "wshdbg/core/types.hpp"
#include <functional>
#include <optional>
#include <string_view>
#include <vector>
namespace wshdbg {
struct SessionEvent {
    enum class Kind { StateChanged, Stopped, Output, Debugger } kind{Kind::StateChanged};
    SessionState state{SessionState::Created};
    std::optional<StopInfo> stop;
    std::optional<DebugEvent> debug;
    std::wstring text;
};
class DebugSession {
public:
    using EventHandler = std::function<void(const SessionEvent&)>;
    [[nodiscard]] SessionState state() const noexcept { return state_; }
    [[nodiscard]] const std::optional<StopInfo>& last_stop() const noexcept { return last_stop_; }
    BreakpointStore& breakpoints() noexcept { return breakpoints_; }
    const BreakpointStore& breakpoints() const noexcept { return breakpoints_; }
    void subscribe(EventHandler handler);
    bool transition(SessionState next);
    void stopped(StopInfo info);
    void output(std::wstring_view text);
    void debugger_event(DebugEvent event);
    [[nodiscard]] static bool can_transition(SessionState from, SessionState to) noexcept;
private:
    void publish(const SessionEvent& event) const;
    SessionState state_{SessionState::Created};
    BreakpointStore breakpoints_;
    std::optional<StopInfo> last_stop_;
    std::vector<EventHandler> handlers_;
};
} // namespace wshdbg
