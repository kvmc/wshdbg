#include "wshdbg/core/logging.hpp"
#include "wshdbg/core/session.hpp"
#ifdef WSHDBG_HAS_WINDOWS_BACKEND
#include "wshdbg/platform/windows_backend.hpp"
#endif
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {
void usage() {
    std::wcout
        << L"wshdbg 0.1.0\n"
        << L"Usage:\n"
        << L"  wshdbg run [--log-level off|error|debug|trace] [--log-file path] <script.vbs|script.js>\n"
        << L"  wshdbg debug [--break line] [--break-on-entry] [--log-level off|error|debug|trace] [--log-file path] <script.vbs|script.js>\n"
        << L"  wshdbg --version\n";
}

std::optional<wshdbg::LogLevel> parse_log_level(std::wstring_view value) {
    if (value == L"off") return wshdbg::LogLevel::Off;
    if (value == L"error" || value == L"errors") return wshdbg::LogLevel::Error;
    if (value == L"debug") return wshdbg::LogLevel::Debug;
    if (value == L"trace") return wshdbg::LogLevel::Trace;
    return std::nullopt;
}

std::optional<std::uint32_t> parse_line(std::wstring_view value) {
    try {
        std::size_t consumed = 0;
        const auto line = std::stoul(std::wstring{value}, &consumed, 10);
        if (consumed != value.size() || line == 0 || line > UINT32_MAX) return std::nullopt;
        return static_cast<std::uint32_t>(line);
    } catch (...) {
        return std::nullopt;
    }
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    using namespace std::chrono_literals;

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
    std::filesystem::path log_file;
    std::filesystem::path script_path;
    std::vector<std::uint32_t> breakpoint_lines;

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
        } else if (arg == L"--log-file") {
            if (++i >= argc) {
                std::wcerr << L"--log-file requires a path\n";
                return 2;
            }
            log_file = argv[i];
        } else if (arg == L"--break") {
            if (++i >= argc) {
                std::wcerr << L"--break requires a line number\n";
                return 2;
            }
            const auto line = parse_line(argv[i]);
            if (!line) {
                std::wcerr << L"Invalid breakpoint line: " << argv[i] << L"\n";
                return 2;
            }
            breakpoint_lines.push_back(*line);
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

    auto& logger = wshdbg::Logger::instance();
    logger.set_level(log_level);
    if (!log_file.empty()) logger.set_file(log_file);

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
    for (const auto line : breakpoint_lines) {
        session.breakpoints().add(wshdbg::SourceLocation{
            .file = script_path,
            .line = line,
            .column = 1});
    }

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

    const bool debug = command == L"debug" || !breakpoint_lines.empty() || break_on_entry;
    const wshdbg::windows::LaunchOptions options{
        .script_path = script_path,
        .language = language,
        .debug = debug,
        .break_on_entry = debug && break_on_entry};

    if (!debug) {
        wshdbg::windows::ActiveScriptHost host;
        std::wstring error;
        if (!host.run(options, session, error)) {
            std::wcerr << L"error: " << error << L"\n";
            return 1;
        }
        return 0;
    }

    wshdbg::windows::DebugControl control;
    std::atomic<bool> finished{false};
    bool run_ok = false;
    std::wstring run_error;

    std::thread worker([&] {
        wshdbg::windows::ActiveScriptHost host;
        run_ok = host.run(options, session, run_error, &control);
        finished.store(true, std::memory_order_release);
    });

    while (!finished.load(std::memory_order_acquire)) {
        const auto wait_result = control.wait(100ms);
        if (wait_result != wshdbg::windows::DebugWaitResult::Paused) continue;

        bool resumed = false;
        while (!resumed && !finished.load(std::memory_order_acquire)) {
            std::wcout << L"wshdbg> " << std::flush;
            std::wstring input;
            if (!std::getline(std::wcin, input)) input = L"quit";

            wshdbg::windows::ResumeAction action;
            if (input == L"c" || input == L"continue") {
                action = wshdbg::windows::ResumeAction::Continue;
            } else if (input == L"s" || input == L"step" || input == L"in") {
                action = wshdbg::windows::ResumeAction::StepInto;
            } else if (input == L"n" || input == L"next" || input == L"over") {
                action = wshdbg::windows::ResumeAction::StepOver;
            } else if (input == L"o" || input == L"out") {
                action = wshdbg::windows::ResumeAction::StepOut;
            } else if (input == L"q" || input == L"quit") {
                action = wshdbg::windows::ResumeAction::Abort;
            } else if (input == L"h" || input == L"help" || input == L"?") {
                std::wcout << L"c/continue  s/step  n/next  o/out  q/quit\n";
                continue;
            } else {
                std::wcout << L"Unknown command. Type 'help'.\n";
                continue;
            }

            std::wstring resume_error;
            if (!control.resume(action, resume_error)) {
                std::wcerr << L"debug-control error: " << resume_error << L"\n";
            } else {
                resumed = true;
            }
        }
    }

    worker.join();
    if (!run_ok) {
        std::wcerr << L"error: " << (run_error.empty() ? L"debug session failed" : run_error) << L"\n";
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
