#pragma once

#include <activdbg.h>
#include <wrl/client.h>
#include <string>

namespace wshdbg::windows {

#ifdef _WIN64
using ProcessDebugManagerInterface = IProcessDebugManager64;
using DebugApplicationInterface = IDebugApplication64;
using DebugDocumentHelperInterface = IDebugDocumentHelper64;
#else
using ProcessDebugManagerInterface = IProcessDebugManager32;
using DebugApplicationInterface = IDebugApplication32;
using DebugDocumentHelperInterface = IDebugDocumentHelper32;
#endif

// Owns the optional Windows Active Debugging service (PDM). The debugger can
// still host scripts when this component is unavailable; source-level debugging
// is reported as unavailable instead of silently falling back to a system IDE.
class ActiveDebugServices {
public:
    ActiveDebugServices() = default;
    ~ActiveDebugServices();

    ActiveDebugServices(const ActiveDebugServices&) = delete;
    ActiveDebugServices& operator=(const ActiveDebugServices&) = delete;

    bool initialize(const std::wstring& application_name, std::wstring& error);
    void shutdown() noexcept;

    bool available() const noexcept { return manager_ && application_; }
    DebugApplicationInterface* application() const noexcept { return application_.Get(); }

    HRESULT create_document_helper(IUnknown* outer, DebugDocumentHelperInterface** helper) const noexcept;

private:
    Microsoft::WRL::ComPtr<ProcessDebugManagerInterface> manager_;
    Microsoft::WRL::ComPtr<DebugApplicationInterface> application_;
    DWORD application_cookie_{0};
};

} // namespace wshdbg::windows
