#include "breakpoint_binder.hpp"
#include "wshdbg/core/logging.hpp"
#include <wrl/client.h>

namespace wshdbg::windows {
using Microsoft::WRL::ComPtr;

bool BreakpointBinder::bind(
    IActiveScript* engine,
    const DebugDocument& document,
    const DebugDocumentHelper& helper,
    Breakpoint breakpoint,
    DebugSession& session,
    DebugSiteBridge& bridge,
    std::wstring& error) noexcept {
    if (!engine || !helper.registered()) {
        error = L"Script engine or debug document is unavailable";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

    const auto offset = document.offset_for_line_column(
        breakpoint.location.line,
        breakpoint.location.column);
    if (!offset) {
        error = L"Breakpoint location is outside the source document";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

#ifdef _WIN64
    ComPtr<IActiveScriptDebug64> script_debug;
#else
    ComPtr<IActiveScriptDebug32> script_debug;
#endif
    HRESULT hr = engine->QueryInterface(IID_PPV_ARGS(&script_debug));
    if (FAILED(hr) || !script_debug) {
        error = L"Script engine does not expose IActiveScriptDebug";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

    ComPtr<IEnumDebugCodeContexts> contexts;
    hr = script_debug->EnumCodeContextsOfPosition(
        helper.source_context(),
        *offset,
        1,
        &contexts);
    if (FAILED(hr) || !contexts) {
        error = L"EnumCodeContextsOfPosition failed";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

    ComPtr<IDebugCodeContext> context;
    ULONG fetched = 0;
    hr = contexts->Next(1, &context, &fetched);
    if (FAILED(hr) || fetched == 0 || !context) {
        error = L"No executable code context exists at the requested source location";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

    hr = context->SetBreakPoint(BREAKPOINT_ENABLED);
    if (FAILED(hr)) {
        error = L"IDebugCodeContext::SetBreakPoint failed";
        session.breakpoints().set_state(breakpoint.id, BreakpointState::Error, error);
        return false;
    }

    session.breakpoints().set_state(breakpoint.id, BreakpointState::Bound);
    bridge.breakpoint_bound(breakpoint.id, breakpoint.location);
    Logger::instance().write(LogLevel::Debug, L"breakpoint", L"Breakpoint bound to Active Script code context");
    return true;
}

} // namespace wshdbg::windows
