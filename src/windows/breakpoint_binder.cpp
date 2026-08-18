#include "breakpoint_binder.hpp"

namespace wshdbg::windows {

bool BreakpointBinder::bind(const Breakpoint&, const DebugCodeContext&) noexcept {
    // Real binding will occur after IDebugDocumentContext and IActiveScriptDebug
    // code contexts are connected. Keeping this boundary now prevents the core
    // breakpoint model from depending on COM interfaces.
    return false;
}

} // namespace wshdbg::windows
