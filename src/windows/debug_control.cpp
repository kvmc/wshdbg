#include "debug_control_internal.hpp"
#include "wshdbg/core/logging.hpp"

#include <objbase.h>
#include <objidl.h>
#include <wrl/client.h>

namespace wshdbg::windows {
using Microsoft::WRL::ComPtr;

namespace {
HRESULT create_git(IGlobalInterfaceTable** git) noexcept {
    if (!git) return E_POINTER;
    *git = nullptr;
    return CoCreateInstance(
        CLSID_StdGlobalInterfaceTable,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(git));
}

BREAKRESUMEACTION native_action(ResumeAction action) noexcept {
    switch (action) {
    case ResumeAction::Continue: return BREAKRESUMEACTION_CONTINUE;
    case ResumeAction::StepInto: return BREAKRESUMEACTION_STEP_INTO;
    case ResumeAction::StepOver: return BREAKRESUMEACTION_STEP_OVER;
    case ResumeAction::StepOut: return BREAKRESUMEACTION_STEP_OUT;
    case ResumeAction::Abort: return BREAKRESUMEACTION_ABORT;
    }
    return BREAKRESUMEACTION_CONTINUE;
}
}

DebugControl::DebugControl() : impl_(std::make_shared<Impl>()) {}
DebugControl::~DebugControl() = default;
DebugControl::DebugControl(DebugControl&&) noexcept = default;
DebugControl& DebugControl::operator=(DebugControl&&) noexcept = default;

DebugWaitResult DebugControl::wait(std::chrono::milliseconds timeout) {
    return impl_->wait(timeout);
}

bool DebugControl::paused() const noexcept {
    return impl_->paused();
}

std::vector<StackFrameInfo> DebugControl::stack() const {
    return impl_->stack();
}

std::vector<VariableInfo> DebugControl::variables() const {
    return impl_->variables();
}

bool DebugControl::resume(ResumeAction action, std::wstring& error) {
    return impl_->resume(action, error);
}

bool DebugControl::Impl::capture(
    IRemoteDebugApplicationThread* thread,
    std::wstring& error) noexcept {
    if (!thread) {
        error = L"Breakpoint callback did not provide a debug thread";
        return false;
    }

    ComPtr<IRemoteDebugApplication> application;
    HRESULT hr = thread->GetApplication(&application);
    if (FAILED(hr) || !application) {
        error = L"Unable to obtain debug application for paused thread";
        return false;
    }

    ComPtr<IGlobalInterfaceTable> git;
    hr = create_git(&git);
    if (FAILED(hr) || !git) {
        error = L"Unable to create COM Global Interface Table";
        return false;
    }

    DWORD application_cookie = 0;
    DWORD thread_cookie = 0;
    hr = git->RegisterInterfaceInGlobal(
        application.Get(), IID_IRemoteDebugApplication, &application_cookie);
    if (FAILED(hr)) {
        error = L"Unable to marshal debug application across threads";
        return false;
    }

    hr = git->RegisterInterfaceInGlobal(
        thread, IID_IRemoteDebugApplicationThread, &thread_cookie);
    if (FAILED(hr)) {
        git->RevokeInterfaceFromGlobal(application_cookie);
        error = L"Unable to marshal paused debug thread across threads";
        return false;
    }

    {
        std::scoped_lock lock(mutex_);
        clear_locked();
        application_cookie_ = application_cookie;
        thread_cookie_ = thread_cookie;
        stack_.clear();
        variables_.clear();
        paused_ = true;
        completed_ = false;
    }
    cv_.notify_all();
    Logger::instance().write(LogLevel::Trace, L"debug-control", L"Paused interfaces marshaled to controller");
    return true;
}

void DebugControl::Impl::set_stack(std::vector<StackFrameInfo> frames) {
    std::scoped_lock lock(mutex_);
    stack_ = std::move(frames);
}

void DebugControl::Impl::set_variables(std::vector<VariableInfo> variables) {
    std::scoped_lock lock(mutex_);
    variables_ = std::move(variables);
}

void DebugControl::Impl::clear_locked() noexcept {
    if (application_cookie_ != 0 || thread_cookie_ != 0) {
        ComPtr<IGlobalInterfaceTable> git;
        if (SUCCEEDED(create_git(&git)) && git) {
            if (thread_cookie_ != 0) git->RevokeInterfaceFromGlobal(thread_cookie_);
            if (application_cookie_ != 0) git->RevokeInterfaceFromGlobal(application_cookie_);
        }
    }
    thread_cookie_ = 0;
    application_cookie_ = 0;
}

void DebugControl::Impl::complete() noexcept {
    {
        std::scoped_lock lock(mutex_);
        clear_locked();
        paused_ = false;
        completed_ = true;
    }
    cv_.notify_all();
}

DebugWaitResult DebugControl::Impl::wait(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] { return paused_ || completed_; })) {
        return DebugWaitResult::Timeout;
    }
    if (paused_) return DebugWaitResult::Paused;
    return DebugWaitResult::Completed;
}

bool DebugControl::Impl::paused() const noexcept {
    std::scoped_lock lock(mutex_);
    return paused_;
}

std::vector<StackFrameInfo> DebugControl::Impl::stack() const {
    std::scoped_lock lock(mutex_);
    return stack_;
}

std::vector<VariableInfo> DebugControl::Impl::variables() const {
    std::scoped_lock lock(mutex_);
    return variables_;
}

bool DebugControl::Impl::resume(ResumeAction action, std::wstring& error) noexcept {
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool owns_com = SUCCEEDED(init);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
        error = L"Unable to initialize COM on debugger control thread";
        return false;
    }

    DWORD application_cookie = 0;
    DWORD thread_cookie = 0;
    {
        std::scoped_lock lock(mutex_);
        if (!paused_ || application_cookie_ == 0 || thread_cookie_ == 0) {
            if (owns_com) CoUninitialize();
            error = L"Debugger is not paused";
            return false;
        }
        application_cookie = application_cookie_;
        thread_cookie = thread_cookie_;
    }

    ComPtr<IGlobalInterfaceTable> git;
    HRESULT hr = create_git(&git);
    ComPtr<IRemoteDebugApplication> application;
    ComPtr<IRemoteDebugApplicationThread> thread;
    if (SUCCEEDED(hr)) {
        hr = git->GetInterfaceFromGlobal(application_cookie, IID_PPV_ARGS(&application));
    }
    if (SUCCEEDED(hr)) {
        hr = git->GetInterfaceFromGlobal(thread_cookie, IID_PPV_ARGS(&thread));
    }
    if (SUCCEEDED(hr) && application && thread) {
        hr = application->ResumeFromBreakPoint(
            thread.Get(), native_action(action), ERRORRESUMEACTION_SkipErrorStatement);
    }

    if (SUCCEEDED(hr)) {
        std::scoped_lock lock(mutex_);
        clear_locked();
        paused_ = false;
        cv_.notify_all();
        Logger::instance().write(LogLevel::Debug, L"debug-control", L"Execution resumed");
    } else {
        error = L"ResumeFromBreakPoint failed";
    }

    if (owns_com) CoUninitialize();
    return SUCCEEDED(hr);
}

} // namespace wshdbg::windows
