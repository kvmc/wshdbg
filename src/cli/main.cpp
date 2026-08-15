#include "wshdbg/core/session.hpp"
#ifdef WSHDBG_HAS_WINDOWS_BACKEND
#include "wshdbg/platform/windows_backend.hpp"
#endif
#include <filesystem>
#include <iostream>
#include <string>

namespace { void usage(){ std::wcout << L"wshdbg 0.1.0\nUsage:\n  wshdbg run <script.vbs|script.js>\n  wshdbg --version\n"; } }

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    if (argc == 2 && std::wstring{argv[1]} == L"--version") { std::wcout << L"wshdbg 0.1.0\n"; return 0; }
    if (argc < 3 || std::wstring{argv[1]} != L"run") { usage(); return argc == 1 ? 0 : 2; }
#ifndef WSHDBG_HAS_WINDOWS_BACKEND
    std::wcerr << L"The Active Scripting backend is unavailable.\n"; return 3;
#else
    const std::filesystem::path script_path{argv[2]};
    const auto ext = script_path.extension().wstring();
    wshdbg::windows::ScriptLanguage language;
    if (ext == L".vbs") language = wshdbg::windows::ScriptLanguage::VBScript;
    else if (ext == L".js") language = wshdbg::windows::ScriptLanguage::JScript;
    else { std::wcerr << L"Unsupported script extension: " << ext << L"\n"; return 2; }
    wshdbg::DebugSession session;
    session.subscribe([](const wshdbg::SessionEvent& event){
        if (event.kind == wshdbg::SessionEvent::Kind::Stopped && event.stop) std::wcout << L"[stopped] " << event.stop->description << L"\n";
        else if (event.kind == wshdbg::SessionEvent::Kind::Output) std::wcout << event.text;
    });
    wshdbg::windows::ActiveScriptHost host;
    std::wstring error;
    if (!host.run({.script_path=script_path,.language=language,.break_on_entry=false}, session, error)) { std::wcerr << L"error: " << error << L"\n"; return 1; }
    return 0;
#endif
}
#else
int main(int argc, char** argv) {
    if (argc == 2 && std::string{argv[1]} == "--version") { std::cout << "wshdbg 0.1.0\n"; return 0; }
    usage();
    return 0;
}
#endif
