#include "debug_control_internal.hpp"
#include "wshdbg/core/logging.hpp"

#include <dbgprop.h>
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

class ScopedCom {
public:
    ScopedCom() noexcept {
        hr_ = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        owns_ = SUCCEEDED(hr_);
    }
    ~ScopedCom() { if (owns_) CoUninitialize(); }
    bool usable() const noexcept { return SUCCEEDED(hr_) || hr_ == RPC_E_CHANGED_MODE; }
private:
    HRESULT hr_{E_FAIL};
    bool owns_{false};
};

HRESULT wait_expression(IDebugExpression* expression, DWORD timeout_ms = 5000) noexcept {
    if (!expression) return E_POINTER;
    HRESULT hr = expression->Start(nullptr);
    if (FAILED(hr)) return hr;
    for (DWORD elapsed = 0; elapsed < timeout_ms; ++elapsed) {
        hr = expression->QueryIsComplete();
        if (hr == S_OK) return S_OK;
        if (FAILED(hr)) return hr;
        Sleep(1);
    }
    expression->Abort();
    return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
}
}

DebugControl::DebugControl() : impl_(std::make_shared<Impl>()) {}
DebugControl::~DebugControl() = default;
DebugControl::DebugControl(DebugControl&&) noexcept = default;
DebugControl& DebugControl::operator=(DebugControl&&) noexcept = default;

DebugWaitResult DebugControl::wait(std::chrono::milliseconds timeout) { return impl_->wait(timeout); }
bool DebugControl::paused() const noexcept { return impl_->paused(); }
std::vector<StackFrameInfo> DebugControl::stack() const { return impl_->stack(); }
std::vector<VariableInfo> DebugControl::variables() const { return impl_->variables(); }
EvaluationResult DebugControl::evaluate(std::wstring_view expression, bool allow_side_effects) {
    return impl_->evaluate(expression, allow_side_effects);
}
bool DebugControl::execute(std::wstring_view statement, std::wstring& error) {
    return impl_->execute(statement, error);
}
bool DebugControl::resume(ResumeAction action, std::wstring& error) { return impl_->resume(action, error); }

bool DebugControl::Impl::capture(IRemoteDebugApplicationThread* thread, std::wstring& error) noexcept {
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
    DWORD frame_cookie = 0;
    hr = git->RegisterInterfaceInGlobal(application.Get(), IID_IRemoteDebugApplication, &application_cookie);
    if (FAILED(hr)) {
        error = L"Unable to marshal debug application across threads";
        return false;
    }

    hr = git->RegisterInterfaceInGlobal(thread, IID_IRemoteDebugApplicationThread, &thread_cookie);
    if (FAILED(hr)) {
        git->RevokeInterfaceFromGlobal(application_cookie);
        error = L"Unable to marshal paused debug thread across threads";
        return false;
    }

    ComPtr<IEnumDebugStackFrames> frames;
    if (SUCCEEDED(thread->EnumStackFrames(&frames)) && frames) {
        DebugStackFrameDescriptor descriptor{};
        ULONG fetched = 0;
        if (SUCCEEDED(frames->Next(1, &descriptor, &fetched)) && fetched == 1 && descriptor.pdsf) {
            ComPtr<IDebugStackFrame> frame;
            frame.Attach(descriptor.pdsf);
            if (descriptor.punkFinal) descriptor.punkFinal->Release();
            const HRESULT frame_hr = git->RegisterInterfaceInGlobal(
                frame.Get(), IID_IDebugStackFrame, &frame_cookie);
            if (FAILED(frame_hr)) frame_cookie = 0;
        } else if (descriptor.punkFinal) {
            descriptor.punkFinal->Release();
        }
    }

    {
        std::scoped_lock lock(mutex_);
        clear_locked();
        application_cookie_ = application_cookie;
        thread_cookie_ = thread_cookie;
        frame_cookie_ = frame_cookie;
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
    if (application_cookie_ != 0 || thread_cookie_ != 0 || frame_cookie_ != 0) {
        ComPtr<IGlobalInterfaceTable> git;
        if (SUCCEEDED(create_git(&git)) && git) {
            if (frame_cookie_ != 0) git->RevokeInterfaceFromGlobal(frame_cookie_);
            if (thread_cookie_ != 0) git->RevokeInterfaceFromGlobal(thread_cookie_);
            if (application_cookie_ != 0) git->RevokeInterfaceFromGlobal(application_cookie_);
        }
    }
    frame_cookie_ = 0;
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
    return paused_ ? DebugWaitResult::Paused : DebugWaitResult::Completed;
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

EvaluationResult DebugControl::Impl::evaluate(
    std::wstring_view expression,
    bool allow_side_effects) {
    EvaluationResult result;
    result.expression = std::wstring{expression};

    ScopedCom com;
    if (!com.usable()) {
        result.error = L"Unable to initialize COM on evaluation thread";
        return result;
    }

    DWORD frame_cookie = 0;
    {
        std::scoped_lock lock(mutex_);
        if (!paused_ || frame_cookie_ == 0) {
            result.error = L"No paused stack frame is available for evaluation";
            return result;
        }
        frame_cookie = frame_cookie_;
    }

    ComPtr<IGlobalInterfaceTable> git;
    HRESULT hr = create_git(&git);
    ComPtr<IDebugStackFrame> frame;
    if (SUCCEEDED(hr)) hr = git->GetInterfaceFromGlobal(frame_cookie, IID_PPV_ARGS(&frame));
    if (FAILED(hr) || !frame) {
        result.error = L"Unable to access paused stack frame";
        return result;
    }

    ComPtr<IDebugExpression> debug_expression;
    DWORD flags = DEBUG_TEXT_ISEXPRESSION;
    if (!allow_side_effects) flags |= DEBUG_TEXT_NOSIDEEFFECTS;
    hr = frame->ParseLanguageText(
        result.expression.c_str(), 10, nullptr, flags, &debug_expression);
    if (FAILED(hr) || !debug_expression) {
        result.error = L"Expression could not be parsed in the current script context";
        return result;
    }

    hr = wait_expression(debug_expression.Get());
    if (FAILED(hr)) {
        result.error = hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT)
            ? L"Expression evaluation timed out"
            : L"Expression evaluation failed";
        return result;
    }

    HRESULT evaluation_hr = E_FAIL;
    BSTR value = nullptr;
    hr = debug_expression->GetResultAsString(&evaluation_hr, &value);
    if (SUCCEEDED(hr) && SUCCEEDED(evaluation_hr)) {
        if (value) result.value = value;
        result.success = true;
    } else {
        result.error = L"Expression returned an error";
    }
    SysFreeString(value);

    ComPtr<IDebugProperty> property;
    HRESULT property_hr = E_FAIL;
    if (SUCCEEDED(debug_expression->GetResultAsDebugProperty(&property_hr, &property)) &&
        SUCCEEDED(property_hr) && property) {
        DebugPropertyInfo info{};
        if (SUCCEEDED(property->GetPropertyInfo(DBGPROP_INFO_TYPE, 10, &info))) {
            if (info.m_bstrType) result.type = info.m_bstrType;
            SysFreeString(info.m_bstrName);
            SysFreeString(info.m_bstrType);
            SysFreeString(info.m_bstrValue);
            SysFreeString(info.m_bstrFullName);
            if (info.m_pDebugProp) info.m_pDebugProp->Release();
        }
    }

    return result;
}

bool DebugControl::Impl::execute(std::wstring_view statement, std::wstring& error) {
    ScopedCom com;
    if (!com.usable()) {
        error = L"Unable to initialize COM on execution thread";
        return false;
    }

    DWORD frame_cookie = 0;
    {
        std::scoped_lock lock(mutex_);
        if (!paused_ || frame_cookie_ == 0) {
            error = L"No paused stack frame is available";
            return false;
        }
        frame_cookie = frame_cookie_;
    }

    ComPtr<IGlobalInterfaceTable> git;
    HRESULT hr = create_git(&git);
    ComPtr<IDebugStackFrame> frame;
    if (SUCCEEDED(hr)) hr = git->GetInterfaceFromGlobal(frame_cookie, IID_PPV_ARGS(&frame));
    if (FAILED(hr) || !frame) {
        error = L"Unable to access paused stack frame";
        return false;
    }

    std::wstring code{statement};
    ComPtr<IDebugExpression> debug_expression;
    hr = frame->ParseLanguageText(code.c_str(), 10, nullptr, 0, &debug_expression);
    if (FAILED(hr) || !debug_expression) {
        error = L"Statement could not be parsed in the current script context";
        return false;
    }

    hr = wait_expression(debug_expression.Get());
    if (FAILED(hr)) {
        error = hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT)
            ? L"Statement execution timed out"
            : L"Statement execution failed";
        return false;
    }

    HRESULT evaluation_hr = E_FAIL;
    BSTR ignored = nullptr;
    hr = debug_expression->GetResultAsString(&evaluation_hr, &ignored);
    SysFreeString(ignored);
    if (FAILED(hr) || FAILED(evaluation_hr)) {
        error = L"Statement returned an error";
        return false;
    }
    return true;
}

bool DebugControl::Impl::resume(ResumeAction action, std::wstring& error) noexcept {
    ScopedCom com;
    if (!com.usable()) {
        error = L"Unable to initialize COM on debugger control thread";
        return false;
    }

    DWORD application_cookie = 0;
    DWORD thread_cookie = 0;
    {
        std::scoped_lock lock(mutex_);
        if (!paused_ || application_cookie_ == 0 || thread_cookie_ == 0) {
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
    if (SUCCEEDED(hr)) hr = git->GetInterfaceFromGlobal(application_cookie, IID_PPV_ARGS(&application));
    if (SUCCEEDED(hr)) hr = git->GetInterfaceFromGlobal(thread_cookie, IID_PPV_ARGS(&thread));
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
    return SUCCEEDED(hr);
}

} // namespace wshdbg::windows
