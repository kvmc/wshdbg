#pragma once

#include "debug_site.hpp"
#include <activdbg.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <wrl/client.h>

namespace wshdbg::windows {

class ApplicationDebugger final : public IApplicationDebugger {
public:
    ApplicationDebugger(DebugSiteBridge& bridge, std::filesystem::path script) noexcept;

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
    std::atomic<ULONG> refs_{1};
    DebugSiteBridge& bridge_;
    std::filesystem::path script_;
    mutable std::mutex mutex_;
    Microsoft::WRL::ComPtr<IRemoteDebugApplicationThread> paused_thread_;
    Microsoft::WRL::ComPtr<IRemoteDebugApplication> paused_application_;
};

} // namespace wshdbg::windows
