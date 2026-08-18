#pragma once

#include "wshdbg/core/breakpoint_store.hpp"
#include "debug_code_context.hpp"

namespace wshdbg::windows {

class BreakpointBinder {
public:
    // The Active Script implementation will provide the real code-context
    // binding. This layer owns the translation from core breakpoints into
    // Windows debugger objects.
    bool bind(const Breakpoint& breakpoint, const DebugCodeContext& context) noexcept;
};

} // namespace wshdbg::windows
