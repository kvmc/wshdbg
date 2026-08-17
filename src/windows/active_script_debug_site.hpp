#pragma once

#include "debug_site.hpp"
#include <activdbg.h>
#include <atomic>
#include <filesystem>

namespace wshdbg::windows {

#ifdef _WIN64
using ActiveScriptSiteDebugInterface = IActiveScriptSiteDebug64;
using DebugApplicationInterface = IDebugApplication64;
#else
using ActiveScriptSiteDebugInterface = IActiveScriptSiteDebug32;
using DebugApplicationInterface = IDebugApplication32;
#endif

// SDK-facing half of the debugger site.  Source-context and debug-application
// ownership are deliberately explicit because those objects are supplied by the
// Active Debugging infrastructure rather than by wshdbg-core.
class ActiveScriptDebugSite final : public ActiveScriptSiteDebugInterface {
public:
    ActiveScriptDebugSite(DebugSiteBridge& bridge, std::filesystem::path script) noexcept;

    STDMETHODIMP QueryInterface(REFIID riid, void** object) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

#ifdef _WIN64
    STDMETHODIMP GetDocumentContextFromPosition(
        DWORDLONG source_context,
        ULONG character_offset,
        ULONG character_count,
        IDebugDocumentContext** context) override;
    STDMETHODIMP GetApplication(IDebugApplication64** application) override;
#else
    STDMETHODIMP GetDocumentContextFromPosition(
        DWORD source_context,
        ULONG character_offset,
        ULONG character_count,
        IDebugDocumentContext** context) override;
    STDMETHODIMP GetApplication(IDebugApplication32** application) override;
#endif

    STDMETHODIMP GetRootApplicationNode(IDebugApplicationNode** root) override;
    STDMETHODIMP OnScriptErrorDebug(
        IActiveScriptErrorDebug* error,
        BOOL* enter_debugger,
        BOOL* call_script_error_when_continuing) override;

private:
    std::atomic<ULONG> refs_{1};
    DebugSiteBridge& bridge_;
    std::filesystem::path script_;
};

} // namespace wshdbg::windows
