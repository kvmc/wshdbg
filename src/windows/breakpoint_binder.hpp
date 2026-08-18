#pragma once

#include "debug_document.hpp"
#include "debug_document_helper.hpp"
#include "debug_site.hpp"
#include "wshdbg/core/breakpoint_store.hpp"
#include <activdbg.h>
#include <string>

namespace wshdbg::windows {

class BreakpointBinder {
public:
    bool bind(
        IActiveScript* engine,
        const DebugDocument& document,
        const DebugDocumentHelper& helper,
        Breakpoint breakpoint,
        DebugSession& session,
        DebugSiteBridge& bridge,
        std::wstring& error) noexcept;
};

} // namespace wshdbg::windows
