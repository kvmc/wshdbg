#include "wshdbg/platform/windows_backend.hpp"
#include "com_runtime.hpp"
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
    if (!in) { error = L"Unable to open script: " + path.wstring(); return {}; }
    std::wstringstream ss; ss << in.rdbuf(); return ss.str();
}

class WScriptObject final : public IDispatch {
public:
    explicit WScriptObject(DebugSession& session) : session_(session) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER; *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IDispatch) { *object = static_cast<IDispatch*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override { auto n = --refs_; if (!n) delete this; return n; }
    STDMETHODIMP GetTypeInfoCount(UINT* count) override { if (!count) return E_POINTER; *count = 0; return S_OK; }
    STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR* names, UINT count, LCID, DISPID* ids) override {
        if (!names || !ids) return E_POINTER;
        for (UINT i = 0; i < count; ++i) { if (_wcsicmp(names[i], L"Echo") != 0) return DISP_E_UNKNOWNNAME; ids[i] = 1; }
        return S_OK;
    }
    STDMETHODIMP Invoke(DISPID id, REFIID, LCID, WORD flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO*, UINT*) override {
        if (result) VariantInit(result);
        if (id != 1 || !(flags & DISPATCH_METHOD)) return DISP_E_MEMBERNOTFOUND;
        std::wstring line;
        if (params) for (UINT i = 0; i < params->cArgs; ++i) {
            VARIANT converted; VariantInit(&converted);
            if (SUCCEEDED(VariantChangeType(&converted, &params->rgvarg[params->cArgs - 1 - i], 0, VT_BSTR)) && converted.bstrVal) {
                if (!line.empty()) line += L" "; line += converted.bstrVal;
            }
            VariantClear(&converted);
        }
        line += L"\n"; session_.output(line); return S_OK;
    }
private:
    std::atomic<ULONG> refs_{1};
    DebugSession& session_;
};

class ScriptSite final : public IActiveScriptSite {
public:
    ScriptSite(DebugSession& session, std::filesystem::path path) : session_(session), path_(std::move(path)) { wscript_.Attach(new WScriptObject(session_)); }
    STDMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER; *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IActiveScriptSite) { *object = static_cast<IActiveScriptSite*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override { auto n = --refs_; if (!n) delete this; return n; }
    STDMETHODIMP GetLCID(LCID* lcid) override { if (!lcid) return E_POINTER; *lcid = GetUserDefaultLCID(); return S_OK; }
    STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** item, ITypeInfo** type) override {
        if (item) *item = nullptr; if (type) *type = nullptr;
        if (!name || _wcsicmp(name, L"WScript") != 0) return TYPE_E_ELEMENTNOTFOUND;
        if (mask & SCRIPTINFO_IUNKNOWN) { if (!item) return E_POINTER; *item = wscript_.Get(); (*item)->AddRef(); }
        return S_OK;
    }
    STDMETHODIMP GetDocVersionString(BSTR* version) override { if (!version) return E_POINTER; *version = SysAllocString(L"wshdbg-1"); return *version ? S_OK : E_OUTOFMEMORY; }
    STDMETHODIMP OnScriptTerminate(const VARIANT*, const EXCEPINFO*) override { return S_OK; }
    STDMETHODIMP OnStateChange(SCRIPTSTATE state) override { if (state == SCRIPTSTATE_STARTED || state == SCRIPTSTATE_CONNECTED) session_.transition(SessionState::Running); return S_OK; }
    STDMETHODIMP OnScriptError(IActiveScriptError* error) override {
        if (!error) return E_POINTER;
        EXCEPINFO info{}; DWORD context = 0; ULONG line = 0; LONG col = 0;
        error->GetExceptionInfo(&info); error->GetSourcePosition(&context, &line, &col);
        std::wstring text = info.bstrDescription ? info.bstrDescription : L"Script error";
        session_.stopped({.reason=StopReason::ScriptError,.location=SourceLocation{.file=path_,.line=static_cast<std::uint32_t>(line+1),.column=static_cast<std::uint32_t>(col+1)},.description=text});
        SysFreeString(info.bstrSource); SysFreeString(info.bstrDescription); SysFreeString(info.bstrHelpFile);
        failed_ = true; return S_OK;
    }
    STDMETHODIMP OnEnterScript() override { return S_OK; }
    STDMETHODIMP OnLeaveScript() override { return S_OK; }
    bool failed() const noexcept { return failed_; }
private:
    std::atomic<ULONG> refs_{1}; DebugSession& session_; std::filesystem::path path_; ComPtr<IDispatch> wscript_; bool failed_{false};
};
}

struct ActiveScriptHost::Impl { ComRuntime com; };
ActiveScriptHost::ActiveScriptHost() : impl_(std::make_unique<Impl>()) {}
ActiveScriptHost::~ActiveScriptHost() = default;
ActiveScriptHost::ActiveScriptHost(ActiveScriptHost&&) noexcept = default;
ActiveScriptHost& ActiveScriptHost::operator=(ActiveScriptHost&&) noexcept = default;

bool ActiveScriptHost::run(const LaunchOptions& options, DebugSession& session, std::wstring& error) {
    if (!impl_->com.usable()) { error = L"COM initialization failed"; session.transition(SessionState::Failed); return false; }
    session.transition(SessionState::Launching);
    if (options.break_on_entry) { error = L"Break-on-entry is not implemented yet"; session.transition(SessionState::Failed); return false; }
    auto source = read_script(options.script_path, error); if (!error.empty()) { session.transition(SessionState::Failed); return false; }
    CLSID clsid{}; if (FAILED(CLSIDFromProgID(options.language == ScriptLanguage::VBScript ? L"VBScript" : L"JScript", &clsid))) { error = L"Scripting engine is not registered"; session.transition(SessionState::Failed); return false; }
    ComPtr<IActiveScript> engine; if (FAILED(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&engine)))) { error = L"Unable to create scripting engine"; session.transition(SessionState::Failed); return false; }
    ComPtr<IActiveScriptParse> parser; if (FAILED(engine.As(&parser))) { error = L"Engine does not expose IActiveScriptParse"; session.transition(SessionState::Failed); return false; }
    auto* raw_site = new ScriptSite(session, options.script_path); ComPtr<IActiveScriptSite> site; site.Attach(raw_site);
    if (FAILED(engine->SetScriptSite(site.Get())) || FAILED(engine->AddNamedItem(L"WScript", SCRIPTITEM_ISVISIBLE)) || FAILED(parser->InitNew())) { error = L"Failed to initialize scripting host"; session.transition(SessionState::Failed); return false; }
    EXCEPINFO ex{}; if (FAILED(parser->ParseScriptText(source.c_str(), nullptr, nullptr, nullptr, 0, 0, SCRIPTTEXT_ISVISIBLE, nullptr, &ex))) { error = L"ParseScriptText failed"; session.transition(SessionState::Failed); return false; }
    if (FAILED(engine->SetScriptState(SCRIPTSTATE_CONNECTED))) { error = L"Failed to start script"; session.transition(SessionState::Failed); return false; }
    engine->Close();
    if (raw_site->failed()) { session.transition(SessionState::Failed); return false; }
    if (session.state() != SessionState::Failed) session.transition(SessionState::Stopped);
    return true;
}
}
