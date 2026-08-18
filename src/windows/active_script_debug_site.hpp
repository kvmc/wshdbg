#pragma once

#include "debug_application.hpp"
#include "debug_document_helper.hpp"
#include "debug_site.hpp"
#include <activdbg.h>
#include <atomic>
#include <filesystem>

namespace wshdbg::windows {

#ifdef _WIN64
using ActiveScriptSiteDebugInterface = IActiveScriptSiteDebug64;
#else
using ActiveScriptSiteDebugInterface = IActiveScriptSiteDebug32;
#endif

class ActiveScriptDebugSite final : public ActiveScriptSiteDebugInterface {
public:
    ActiveScriptDebugSite(
        DebugSiteBridge& bridge,
        std::filesystem::path script,
        DebugApplication& application,
        DebugDocumentHelper& document_helper) noexcept;

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
    DebugApplication& application_;
    DebugDocumentHelper& document_helper_;
};

} // namespace wshdbg::windows
