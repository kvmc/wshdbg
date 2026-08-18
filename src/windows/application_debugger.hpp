#pragma once

#include "debug_document.hpp"
#include "debug_site.hpp"
#include "wshdbg/platform/windows_backend.hpp"
#include <activdbg.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <vector>
#include <wrl/client.h>

namespace wshdbg::windows {

class ApplicationDebugger final : public IApplicationDebugger {
public:
    ApplicationDebugger(
        DebugSiteBridge& bridge,
        std::filesystem::path script,
        DebugControl* control = nullptr) noexcept;
    ApplicationDebugger(
        DebugSiteBridge& bridge,
        DebugDocument& document,
        DebugControl* control = nullptr) noexcept;

    STDMETHODIMP QueryInterface(REFIID riid, void** object) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP QueryAlive() override;
    STDMETHODIMP CreateInstanceAtDebugger(
        REFCLSID clsid,
        IUnknown* outer,
        DWORD cls_context,
        REFIID iid,
        IUnknown** object) override;
    STDMETHODIMP onDebugOutput(LPCOLESTR text) override;
    STDMETHODIMP onHandleBreakPoint(
        IRemoteDebugApplicationThread* thread,
        BREAKREASON reason,
        IActiveScriptErrorDebug* error) override;
    STDMETHODIMP onClose() override;
    STDMETHODIMP onDebuggerEvent(REFIID iid, IUnknown* object) override;

    HRESULT resume(BREAKRESUMEACTION action) noexcept;
    bool paused() const noexcept;

private:
    std::optional<SourceLocation> source_location(
        IRemoteDebugApplicationThread* thread) const noexcept;
    std::optional<SourceLocation> source_location(
        IDebugStackFrame* frame) const noexcept;
    std::vector<StackFrameInfo> capture_stack(
        IRemoteDebugApplicationThread* thread) const noexcept;

    std::atomic<ULONG> refs_{1};
    DebugSiteBridge& bridge_;
    std::filesystem::path script_;
    DebugDocument* document_{nullptr};
    DebugControl* control_{nullptr};
    mutable std::mutex mutex_;
    Microsoft::WRL::ComPtr<IRemoteDebugApplicationThread> paused_thread_;
    Microsoft::WRL::ComPtr<IRemoteDebugApplication> paused_application_;
};

} // namespace wshdbg::windows
