#include "application_debugger.hpp"
#include "debug_control_internal.hpp"
#include "wshdbg/core/logging.hpp"
#include <objbase.h>
#include <wrl/client.h>

namespace wshdbg::windows {
using Microsoft::WRL::ComPtr;

ApplicationDebugger::ApplicationDebugger(
    DebugSiteBridge& bridge,
    std::filesystem::path script,
    DebugControl* control) noexcept
    : bridge_(bridge), script_(std::move(script)), control_(control) {}

ApplicationDebugger::ApplicationDebugger(
    DebugSiteBridge& bridge,
    DebugDocument& document,
    DebugControl* control) noexcept
    : bridge_(bridge), script_(document.path()), document_(&document), control_(control) {}

STDMETHODIMP ApplicationDebugger::QueryInterface(REFIID riid, void** object) {
    if (!object) return E_POINTER;
    *object = nullptr;
    if (riid == IID_IUnknown || riid == IID_IApplicationDebugger) {
        *object = static_cast<IApplicationDebugger*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ApplicationDebugger::AddRef() { return ++refs_; }

STDMETHODIMP_(ULONG) ApplicationDebugger::Release() {
    const auto refs = --refs_;
    if (!refs) delete this;
    return refs;
}

STDMETHODIMP ApplicationDebugger::QueryAlive() { return S_OK; }

STDMETHODIMP ApplicationDebugger::CreateInstanceAtDebugger(
    REFCLSID clsid,
    IUnknown* outer,
    DWORD cls_context,
    REFIID iid,
    IUnknown** object) {
    if (!object) return E_POINTER;
    *object = nullptr;
    return CoCreateInstance(clsid, outer, cls_context, iid, reinterpret_cast<void**>(object));
}

STDMETHODIMP ApplicationDebugger::onDebugOutput(LPCOLESTR text) {
    if (text) Logger::instance().write(LogLevel::Debug, L"script-debug", text);
    return S_OK;
}

std::optional<SourceLocation> ApplicationDebugger::source_location(
    IRemoteDebugApplicationThread* thread) const noexcept {
    if (!thread || !document_) return std::nullopt;

    ComPtr<IEnumDebugStackFrames> frames;
    if (FAILED(thread->EnumStackFrames(&frames)) || !frames) return std::nullopt;

    DebugStackFrameDescriptor descriptor{};
    ULONG fetched = 0;
    if (FAILED(frames->Next(1, &descriptor, &fetched)) || fetched == 0 || !descriptor.pdsf) {
        if (descriptor.punkFinal) descriptor.punkFinal->Release();
        return std::nullopt;
    }

    ComPtr<IDebugStackFrame> frame;
    frame.Attach(descriptor.pdsf);
    if (descriptor.punkFinal) descriptor.punkFinal->Release();

    ComPtr<IDebugCodeContext> code_context;
    if (FAILED(frame->GetCodeContext(&code_context)) || !code_context) return std::nullopt;

    ComPtr<IDebugDocumentContext> document_context;
    if (FAILED(code_context->GetDocumentContext(&document_context)) || !document_context) {
        return std::nullopt;
    }

    ComPtr<IDebugDocument> debug_document;
    if (FAILED(document_context->GetDocument(&debug_document)) || !debug_document) {
        return std::nullopt;
    }

    ComPtr<IDebugDocumentText> text;
    if (FAILED(debug_document.As(&text)) || !text) return std::nullopt;

    ULONG character_position = 0;
    ULONG character_count = 0;
    if (FAILED(text->GetPositionOfContext(
            document_context.Get(),
            &character_position,
            &character_count))) {
        return std::nullopt;
    }

    const auto source_position = document_->line_column_for_offset(character_position);
    if (!source_position) return std::nullopt;

    return SourceLocation{
        .file = document_->path(),
        .line = source_position->first,
        .column = source_position->second};
}

STDMETHODIMP ApplicationDebugger::onHandleBreakPoint(
    IRemoteDebugApplicationThread* thread,
    BREAKREASON reason,
    IActiveScriptErrorDebug* error) {
    {
        std::scoped_lock lock(mutex_);
        paused_thread_ = thread;
        paused_application_.Reset();
        if (thread) thread->GetApplication(&paused_application_);
    }

    if (control_ && control_->impl_) {
        std::wstring marshal_error;
        if (!control_->impl_->capture(thread, marshal_error)) {
            Logger::instance().write(LogLevel::Error, L"debug-control", marshal_error);
        }
    }

    StopReason stop_reason = StopReason::Breakpoint;
    std::wstring message = L"breakpoint";
    switch (reason) {
    case BREAKREASON_STEP:
        stop_reason = StopReason::Step;
        message = L"step complete";
        break;
    case BREAKREASON_ERROR:
        stop_reason = StopReason::ScriptError;
        message = L"script error";
        break;
    case BREAKREASON_DEBUGGER_HALT:
    case BREAKREASON_HOST_INITIATED:
        stop_reason = StopReason::Pause;
        message = L"execution paused";
        break;
    default:
        break;
    }

    SourceLocation location = source_location(thread).value_or(SourceLocation{
        .file = script_, .line = 1, .column = 1});

    if (error) {
        DWORD source_context = 0;
        ULONG line = 0;
        LONG column = 0;
        if (SUCCEEDED(error->GetSourcePosition(&source_context, &line, &column))) {
            location.line = static_cast<std::uint32_t>(line + 1);
            location.column = static_cast<std::uint32_t>(column + 1);
        }
    }

    if (stop_reason == StopReason::Step) {
        bridge_.step_complete(location);
    } else if (stop_reason == StopReason::ScriptError) {
        bridge_.script_error(location, message);
    } else {
        bridge_.breakpoint_hit(std::nullopt, location, message);
    }

    Logger::instance().write(LogLevel::Debug, L"application-debugger", message);
    return S_OK;
}

STDMETHODIMP ApplicationDebugger::onClose() {
    {
        std::scoped_lock lock(mutex_);
        paused_thread_.Reset();
        paused_application_.Reset();
    }
    if (control_ && control_->impl_) control_->impl_->complete();
    bridge_.process_stopped(L"debug application closed");
    return S_OK;
}

STDMETHODIMP ApplicationDebugger::onDebuggerEvent(REFIID, IUnknown*) {
    return E_NOTIMPL;
}

HRESULT ApplicationDebugger::resume(BREAKRESUMEACTION action) noexcept {
    std::scoped_lock lock(mutex_);
    if (!paused_thread_ || !paused_application_) return S_FALSE;
    const HRESULT hr = paused_application_->ResumeFromBreakPoint(
        paused_thread_.Get(),
        action,
        ERRORRESUMEACTION_SkipErrorStatement);
    if (SUCCEEDED(hr)) {
        paused_thread_.Reset();
        paused_application_.Reset();
    }
    return hr;
}

bool ApplicationDebugger::paused() const noexcept {
    std::scoped_lock lock(mutex_);
    return paused_thread_ != nullptr;
}

} // namespace wshdbg::windows
