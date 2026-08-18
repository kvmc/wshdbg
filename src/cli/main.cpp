#include "wshdbg/core/logging.hpp"
#include "wshdbg/core/session.hpp"
#ifdef WSHDBG_HAS_WINDOWS_BACKEND
#include "wshdbg/platform/windows_backend.hpp"
#endif
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace {
void usage() {
    std::wcout
        << L"wshdbg 0.1.0\n"
        << L"Usage:\n"
        << L"  wshdbg run [--log-level off|error|debug|trace] <script.vbs|script.js>\n"
        << L"  wshdbg debug [--break-on-entry] [--log-level off|error|debug|trace] <script.vbs|script.js>\n"
        << L"  wshdbg --version\n";
}

std::optional<wshdbg::LogLevel> parse_log_level(std::wstring_view value) {
    if (value == L"off") return wshdbg::LogLevel::Off;
    if (value == L"error" || value == L"errors") return wshdbg::LogLevel::Error;
    if (value == L"debug") return wshdbg::LogLevel::Debug;
    if (value == L"trace") return wshdbg::LogLevel::Trace;
    return std::nullopt;
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    if (argc == 2 && std::wstring{argv[1]} == L"--version") {
        std::wcout << L"wshdbg 0.1.0\n";
        return 0;
    }
    if (argc < 3) {
        usage();
        return argc == 1 ? 0 : 2;
    }

    const std::wstring command{argv[1]};
    if (command != L"run" && command != L"debug") {
        usage();
        return 2;
    }

    wshdbg::LogLevel log_level = wshdbg::LogLevel::Error;
    bool break_on_entry = false;
    std::filesystem::path script_path;
    for (int i = 2; i < argc; ++i) {
        const std::wstring arg{argv[i]};
        if (arg == L"--log-level") {
            if (++i >= argc) {
                std::wcerr << L"--log-level requires a value\n";
                return 2;
            }
            const auto parsed = parse_log_level(argv[i]);
            if (!parsed) {
                std::wcerr << L"Invalid log level: " << argv[i] << L"\n";
                return 2;
            }
            log_level = *parsed;
        } else if (arg == L"--break-on-entry") {
            break_on_entry = true;
        } else if (script_path.empty()) {
            script_path = arg;
        } else {
            std::wcerr << L"Unexpected argument: " << arg << L"\n";
            return 2;
        }
    }

    if (script_path.empty()) {
        std::wcerr << L"No script path supplied.\n";
        return 2;
    }

    wshdbg::Logger::instance().set_level(log_level);

#ifndef WSHDBG_HAS_WINDOWS_BACKEND
    std::wcerr << L"The Active Scripting backend is unavailable.\n";
    return 3;
#else
    const auto ext = script_path.extension().wstring();
    wshdbg::windows::ScriptLanguage language;
    if (ext == L".vbs") language = wshdbg::windows::ScriptLanguage::VBScript;
    else if (ext == L".js") language = wshdbg::windows::ScriptLanguage::JScript;
    else {
        std::wcerr << L"Unsupported script extension: " << ext << L"\n";
        return 2;
    }

    wshdbg::DebugSession session;
    session.subscribe([](const wshdbg::SessionEvent& event) {
        if (event.kind == wshdbg::SessionEvent::Kind::Stopped && event.stop) {
            std::wcout << L"[stopped] " << event.stop->description;
            if (event.stop->location) {
                std::wcout << L" at " << event.stop->location->file.wstring()
                           << L":" << event.stop->location->line
                           << L":" << event.stop->location->column;
            }
            std::wcout << L"\n";
        } else if (event.kind == wshdbg::SessionEvent::Kind::Output) {
            std::wcout << event.text;
        } else if (event.kind == wshdbg::SessionEvent::Kind::Debugger && event.debug) {
            wshdbg::Logger::instance().write(
                wshdbg::LogLevel::Debug,
                L"debugger",
                event.debug->message.empty() ? L"debugger event" : event.debug->message);
        }
    });

    wshdbg::windows::ActiveScriptHost host;
    std::wstring error;
    const bool debug = command == L"debug";
    if (!host.run({
            .script_path = script_path,
            .language = language,
            .debug = debug,
            .break_on_entry = debug && break_on_entry},
        session,
        error)) {
        std::wcerr << L"error: " << error << L"\n";
        return 1;
    }
    return 0;
#endif
}
#else
int main(int argc, char** argv) {
    if (argc == 2 && std::string{argv[1]} == "--version") {
        std::cout << "wshdbg 0.1.0\n";
        return 0;
    }
    usage();
    return 0;
}
#endif
