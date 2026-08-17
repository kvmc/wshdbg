#include "active_script_debug_site.hpp"

namespace wshdbg::windows {

ActiveScriptDebugSite::ActiveScriptDebugSite(DebugSiteBridge& bridge, std::filesystem::path script) noexcept
    : bridge_(bridge), script_(std::move(script)) {}

STDMETHODIMP ActiveScriptDebugSite::QueryInterface(REFIID riid, void** object) {
    if (!object) return E_POINTER;
    *object = nullptr;

#ifdef _WIN64
    if (riid == IID_IUnknown || riid == IID_IActiveScriptSiteDebug64) {
#else
    if (riid == IID_IUnknown || riid == IID_IActiveScriptSiteDebug32) {
#endif
        *object = static_cast<ActiveScriptSiteDebugInterface*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ActiveScriptDebugSite::AddRef() { return ++refs_; }

STDMETHODIMP_(ULONG) ActiveScriptDebugSite::Release() {
    const auto refs = --refs_;
    if (!refs) delete this;
    return refs;
}

#ifdef _WIN64
STDMETHODIMP ActiveScriptDebugSite::GetDocumentContextFromPosition(
    DWORDLONG,
    ULONG,
    ULONG,
    IDebugDocumentContext** context) {
#else
STDMETHODIMP ActiveScriptDebugSite::GetDocumentContextFromPosition(
    DWORD,
    ULONG,
    ULONG,
    IDebugDocumentContext** context) {
#endif
    if (!context) return E_POINTER;
    *context = nullptr;
    // Implemented once IDebugDocumentHelper registration lands. Returning
    // E_NOTIMPL is intentional; fabricated document contexts would bind
    // breakpoints to the wrong source range.
    return E_NOTIMPL;
}

#ifdef _WIN64
STDMETHODIMP ActiveScriptDebugSite::GetApplication(IDebugApplication64** application) {
#else
STDMETHODIMP ActiveScriptDebugSite::GetApplication(IDebugApplication32** application) {
#endif
    if (!application) return E_POINTER;
    *application = nullptr;
    // The debug application will be injected by the Active Debugging session
    // owner. Do not silently create a Visual Studio/PDM dependency here.
    return E_NOTIMPL;
}

STDMETHODIMP ActiveScriptDebugSite::GetRootApplicationNode(IDebugApplicationNode** root) {
    if (!root) return E_POINTER;
    *root = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP ActiveScriptDebugSite::OnScriptErrorDebug(
    IActiveScriptErrorDebug* error,
    BOOL* enter_debugger,
    BOOL* call_script_error_when_continuing) {
    if (!enter_debugger || !call_script_error_when_continuing) return E_POINTER;

    *enter_debugger = TRUE;
    *call_script_error_when_continuing = TRUE;

    SourceLocation location{.file = script_, .line = 1, .column = 1};
    std::wstring description = L"script error";

    if (error) {
        DWORD source_context = 0;
        ULONG line = 0;
        LONG column = 0;
        if (SUCCEEDED(error->GetSourcePosition(&source_context, &line, &column))) {
            location.line = static_cast<std::uint32_t>(line + 1);
            location.column = static_cast<std::uint32_t>(column + 1);
        }

        EXCEPINFO info{};
        if (SUCCEEDED(error->GetExceptionInfo(&info))) {
            if (info.bstrDescription) description = info.bstrDescription;
            SysFreeString(info.bstrSource);
            SysFreeString(info.bstrDescription);
            SysFreeString(info.bstrHelpFile);
        }
    }

    bridge_.script_error(std::move(location), description);
    return S_OK;
}

} // namespace wshdbg::windows
