#include "active_debug_services.hpp"
#include "wshdbg/core/logging.hpp"

#include <objbase.h>

namespace wshdbg::windows {

ActiveDebugServices::~ActiveDebugServices() {
    shutdown();
}

bool ActiveDebugServices::initialize(const std::wstring& application_name, std::wstring& error) {
    shutdown();

    HRESULT hr = CoCreateInstance(
        CLSID_ProcessDebugManager,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&manager_));
    if (FAILED(hr)) {
        error = L"Process Debug Manager is unavailable (HRESULT 0x" + std::to_wstring(static_cast<unsigned long>(hr)) + L")";
        Logger::instance().write(LogLevel::Debug, L"active-debug", error);
        return false;
    }

    hr = manager_->CreateApplication(&application_);
    if (FAILED(hr) || !application_) {
        error = L"IProcessDebugManager::CreateApplication failed";
        shutdown();
        return false;
    }

    hr = application_->SetName(application_name.c_str());
    if (FAILED(hr)) {
        error = L"IDebugApplication::SetName failed";
        shutdown();
        return false;
    }

    hr = manager_->AddApplication(application_.Get(), &application_cookie_);
    if (FAILED(hr)) {
        error = L"IProcessDebugManager::AddApplication failed";
        shutdown();
        return false;
    }

    Logger::instance().write(LogLevel::Debug, L"active-debug", L"Active Debugging application registered");
    return true;
}

void ActiveDebugServices::shutdown() noexcept {
    if (manager_ && application_cookie_ != 0) {
        manager_->RemoveApplication(application_cookie_);
    }
    application_cookie_ = 0;
    application_.Reset();
    manager_.Reset();
}

HRESULT ActiveDebugServices::create_document_helper(
    IUnknown* outer,
    DebugDocumentHelperInterface** helper) const noexcept {
    if (!helper) return E_POINTER;
    *helper = nullptr;
    if (!manager_) return E_UNEXPECTED;
    return manager_->CreateDebugDocumentHelper(outer, helper);
}

} // namespace wshdbg::windows
