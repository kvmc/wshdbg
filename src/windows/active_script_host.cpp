#include "wshdbg/platform/windows_backend.hpp"
#include "active_script_debug_site.hpp"
#include "application_debugger.hpp"
#include "breakpoint_binder.hpp"
#include "com_runtime.hpp"
#include "debug_application.hpp"
#include "debug_document.hpp"
#include "debug_document_helper.hpp"
#include "debug_site.hpp"
#include "wshdbg/core/logging.hpp"

#include <activscp.h>
#include <wrl/client.h>
#include <atomic>
#include <cwchar>
#include <fstream>
#include <sstream>

namespace wshdbg::windows {
using Microsoft::WRL::ComPtr;

namespace {
std::wstring read_script(const std::filesystem::path& path, std::wstring& error) {
    std::wifstream in(path);
    if (!in) {
        error = L"Unable to open script: " + path.wstring();
        return {};
    }
    std::wstringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

class WScriptObject final : public IDispatch {
public:
    explicit WScriptObject(DebugSession& session) : session_(session) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IDispatch) {
            *object = static_cast<IDispatch*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        const auto n = --refs_;
        if (!n) delete this;
        return n;
    }
    STDMETHODIMP GetTypeInfoCount(UINT* count) override {
        if (!count) return E_POINTER;
        *count = 0;
        return S_OK;
    }
    STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR* names, UINT count, LCID, DISPID* ids) override {
        if (!names || !ids) return E_POINTER;
        for (UINT i = 0; i < count; ++i) {
            if (_wcsicmp(names[i], L"Echo") != 0) return DISP_E_UNKNOWNNAME;
            ids[i] = 1;
        }
        return S_OK;
    }
    STDMETHODIMP Invoke(
        DISPID id,
        REFIID,
        LCID,
        WORD flags,
        DISPPARAMS* params,
        VARIANT* result,
        EXCEPINFO*,
        UINT*) override {
        if (result) VariantInit(result);
        if (id != 1 || !(flags & DISPATCH_METHOD)) return DISP_E_MEMBERNOTFOUND;
        std::wstring line;
        if (params) {
            for (UINT i = 0; i < params->cArgs; ++i) {
                VARIANT converted;
                VariantInit(&converted);
                if (SUCCEEDED(VariantChangeType(
                        &converted,
                        &params->rgvarg[params->cArgs - 1 - i],
                        0,
                        VT_BSTR)) && converted.bstrVal) {
                    if (!line.empty()) line += L" ";
                    line += converted.bstrVal;
                }
                VariantClear(&converted);
            }
        }
        line += L"\n";
        session_.output(line);
        return S_OK;
    }
private:
    std::atomic<ULONG> refs_{1};
    DebugSession& session_;
};

class ScriptSite final : public IActiveScriptSite, public ActiveScriptSiteDebugInterface {
public:
    ScriptSite(
        DebugSession& session,
        std::filesystem::path path,
        DebugSiteBridge& bridge,
        DebugApplication& application,
        DebugDocumentHelper& document_helper,
        bool debug_enabled)
        : session_(session),
          path_(std::move(path)),
          debug_enabled_(debug_enabled),
          debug_site_(bridge, path_, application, document_helper) {
        wscript_.Attach(new WScriptObject(session_));
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IActiveScriptSite) {
            *object = static_cast<IActiveScriptSite*>(this);
            AddRef();
            return S_OK;
        }
#ifdef _WIN64
        if (debug_enabled_ && riid == IID_IActiveScriptSiteDebug64) {
#else
        if (debug_enabled_ && riid == IID_IActiveScriptSiteDebug32) {
#endif
            *object = static_cast<ActiveScriptSiteDebugInterface*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        const auto n = --refs_;
        if (!n) delete this;
        return n;
    }

    STDMETHODIMP GetLCID(LCID* lcid) override {
        if (!lcid) return E_POINTER;
        *lcid = GetUserDefaultLCID();
        return S_OK;
    }
    STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** item, ITypeInfo** type) override {
        if (item) *item = nullptr;
        if (type) *type = nullptr;
        if (!name || _wcsicmp(name, L"WScript") != 0) return TYPE_E_ELEMENTNOTFOUND;
        if (mask & SCRIPTINFO_IUNKNOWN) {
            if (!item) return E_POINTER;
            *item = wscript_.Get();
            (*item)->AddRef();
        }
        return S_OK;
    }
    STDMETHODIMP GetDocVersionString(BSTR* version) override {
        if (!version) return E_POINTER;
        *version = SysAllocString(L"wshdbg-1");
        return *version ? S_OK : E_OUTOFMEMORY;
    }
    STDMETHODIMP OnScriptTerminate(const VARIANT*, const EXCEPINFO*) override { return S_OK; }
    STDMETHODIMP OnStateChange(SCRIPTSTATE state) override {
        Logger::instance().write(LogLevel::Trace, L"active-script", L"script state changed");
        if (state == SCRIPTSTATE_STARTED || state == SCRIPTSTATE_CONNECTED) {
            session_.transition(SessionState::Running);
        }
        return S_OK;
    }
    STDMETHODIMP OnScriptError(IActiveScriptError* error) override {
        if (!error) return E_POINTER;
        EXCEPINFO info{};
        DWORD context = 0;
        ULONG line = 0;
        LONG col = 0;
        error->GetExceptionInfo(&info);
        error->GetSourcePosition(&context, &line, &col);
        std::wstring text = info.bstrDescription ? info.bstrDescription : L"Script error";
        session_.stopped({
            .reason = StopReason::ScriptError,
            .location = SourceLocation{
                .file = path_,
                .line = static_cast<std::uint32_t>(line + 1),
                .column = static_cast<std::uint32_t>(col + 1)},
            .description = text});
        SysFreeString(info.bstrSource);
        SysFreeString(info.bstrDescription);
        SysFreeString(info.bstrHelpFile);
        failed_ = true;
        return S_OK;
    }
    STDMETHODIMP OnEnterScript() override { return S_OK; }
    STDMETHODIMP OnLeaveScript() override { return S_OK; }

#ifdef _WIN64
    STDMETHODIMP GetDocumentContextFromPosition(
        DWORDLONG source_context,
        ULONG character_offset,
        ULONG character_count,
        IDebugDocumentContext** context) override {
        return debug_site_.GetDocumentContextFromPosition(
            source_context, character_offset, character_count, context);
    }
    STDMETHODIMP GetApplication(IDebugApplication64** application) override {
        return debug_site_.GetApplication(application);
    }
#else
    STDMETHODIMP GetDocumentContextFromPosition(
        DWORD source_context,
        ULONG character_offset,
        ULONG character_count,
        IDebugDocumentContext** context) override {
        return debug_site_.GetDocumentContextFromPosition(
            source_context, character_offset, character_count, context);
    }
    STDMETHODIMP GetApplication(IDebugApplication32** application) override {
        return debug_site_.GetApplication(application);
    }
#endif
    STDMETHODIMP GetRootApplicationNode(IDebugApplicationNode** root) override {
        return debug_site_.GetRootApplicationNode(root);
    }
    STDMETHODIMP OnScriptErrorDebug(
        IActiveScriptErrorDebug* error,
        BOOL* enter_debugger,
        BOOL* call_script_error_when_continuing) override {
        return debug_site_.OnScriptErrorDebug(
            error, enter_debugger, call_script_error_when_continuing);
    }

    bool failed() const noexcept { return failed_; }

private:
    std::atomic<ULONG> refs_{1};
    DebugSession& session_;
    std::filesystem::path path_;
    bool debug_enabled_{false};
    ActiveScriptDebugSite debug_site_;
    ComPtr<IDispatch> wscript_;
    bool failed_{false};
};
}

struct ActiveScriptHost::Impl { ComRuntime com; };
ActiveScriptHost::ActiveScriptHost() : impl_(std::make_unique<Impl>()) {}
ActiveScriptHost::~ActiveScriptHost() = default;
ActiveScriptHost::ActiveScriptHost(ActiveScriptHost&&) noexcept = default;
ActiveScriptHost& ActiveScriptHost::operator=(ActiveScriptHost&&) noexcept = default;

bool ActiveScriptHost::run(
    const LaunchOptions& options,
    DebugSession& session,
    std::wstring& error,
    DebugControl* control) {
    if (!impl_->com.usable()) {
        error = L"COM initialization failed";
        session.transition(SessionState::Failed);
        return false;
    }
    session.transition(SessionState::Launching);

    auto source = read_script(options.script_path, error);
    if (!error.empty()) {
        session.transition(SessionState::Failed);
        return false;
    }

    CLSID clsid{};
    if (FAILED(CLSIDFromProgID(
            options.language == ScriptLanguage::VBScript ? L"VBScript" : L"JScript",
            &clsid))) {
        error = L"Scripting engine is not registered";
        session.transition(SessionState::Failed);
        return false;
    }

    ComPtr<IActiveScript> engine;
    if (FAILED(CoCreateInstance(
            clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&engine)))) {
        error = L"Unable to create scripting engine";
        session.transition(SessionState::Failed);
        return false;
    }

    ComPtr<IActiveScriptParse> parser;
    if (FAILED(engine.As(&parser))) {
        error = L"Engine does not expose IActiveScriptParse";
        session.transition(SessionState::Failed);
        return false;
    }

    const bool debug_requested =
        options.debug || options.break_on_entry || !session.breakpoints().all().empty();
    if ((options.break_on_entry || !session.breakpoints().all().empty()) && !control) {
        error = L"Interactive debug control is required for breakpoints or break-on-entry";
        session.transition(SessionState::Failed);
        return false;
    }

    DebugSiteBridge bridge(session);
    DebugApplication debug_application;
    DebugDocument document(options.script_path, source);
    DebugDocumentHelper document_helper(document);
    ComPtr<IApplicationDebugger> application_debugger;
    bool debug_ready = false;

    if (debug_requested) {
        std::wstring debug_error;
        if (!debug_application.initialize(
                L"wshdbg: " + options.script_path.filename().wstring(), debug_error)) {
            error = L"Active Debugging initialization failed: " + debug_error;
            session.transition(SessionState::Failed);
            return false;
        }

        auto* callback = new ApplicationDebugger(bridge, document, control);
        application_debugger.Attach(callback);
        if (FAILED(debug_application.services().connect_debugger(application_debugger.Get()))) {
            error = L"Unable to connect wshdbg application debugger";
            session.transition(SessionState::Failed);
            return false;
        }
        debug_ready = true;
    }

    auto* raw_site = new ScriptSite(
        session,
        options.script_path,
        bridge,
        debug_application,
        document_helper,
        debug_ready);
    ComPtr<IActiveScriptSite> site;
    site.Attach(raw_site);

    if (FAILED(engine->SetScriptSite(site.Get())) ||
        FAILED(engine->AddNamedItem(L"WScript", SCRIPTITEM_ISVISIBLE)) ||
        FAILED(parser->InitNew())) {
        error = L"Failed to initialize scripting host";
        session.transition(SessionState::Failed);
        return false;
    }

    if (debug_ready) {
        std::wstring debug_error;
        if (!document_helper.register_document(
                debug_application.services(), engine.Get(), debug_error)) {
            error = L"Debug document registration failed: " + debug_error;
            session.transition(SessionState::Failed);
            return false;
        }
    }

    EXCEPINFO ex{};
    const DWORD_PTR source_context = debug_ready
        ? static_cast<DWORD_PTR>(document_helper.source_context())
        : 0;
    if (FAILED(parser->ParseScriptText(
            source.c_str(), nullptr, nullptr, nullptr, source_context, 0,
            SCRIPTTEXT_ISVISIBLE, nullptr, &ex))) {
        error = ex.bstrDescription ? ex.bstrDescription : L"ParseScriptText failed";
        SysFreeString(ex.bstrSource);
        SysFreeString(ex.bstrDescription);
        SysFreeString(ex.bstrHelpFile);
        session.transition(SessionState::Failed);
        return false;
    }

    bridge.script_loaded(options.script_path);

    if (debug_ready) {
        BreakpointBinder binder;
        for (const auto& breakpoint : session.breakpoints().for_file(options.script_path)) {
            std::wstring bind_error;
            binder.bind(
                engine.Get(), document, document_helper, breakpoint,
                session, bridge, bind_error);
            if (!bind_error.empty()) {
                Logger::instance().write(LogLevel::Error, L"breakpoint", bind_error);
            }
        }

        if (options.break_on_entry) {
            const HRESULT hr = debug_application.native()->CauseBreak();
            if (FAILED(hr)) {
                error = L"Break-on-entry request failed";
                session.transition(SessionState::Failed);
                return false;
            }
        }
    }

    if (FAILED(engine->SetScriptState(SCRIPTSTATE_CONNECTED))) {
        error = L"Failed to start script";
        session.transition(SessionState::Failed);
        return false;
    }

    engine->Close();
    if (raw_site->failed()) {
        session.transition(SessionState::Failed);
        return false;
    }
    if (session.state() != SessionState::Failed && session.state() != SessionState::Paused) {
        session.transition(SessionState::Stopped);
    }
    return true;
}

} // namespace wshdbg::windows
