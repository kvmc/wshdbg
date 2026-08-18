#include "wshdbg/core/session.hpp"
#include "wshdbg/platform/windows_backend.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

int main() {
    using namespace std::chrono_literals;
    using namespace wshdbg;
    using namespace wshdbg::windows;

#ifndef WSHDBG_SOURCE_DIR
#error WSHDBG_SOURCE_DIR must be defined
#endif

    const std::filesystem::path script =
        std::filesystem::path{WSHDBG_SOURCE_DIR} / "tests/data/breakpoints/simple.vbs";

    DebugSession session;
    session.breakpoints().add(SourceLocation{.file = script, .line = 4, .column = 1});

    bool saw_breakpoint = false;
    std::uint32_t stopped_line = 0;
    std::wstring output;
    session.subscribe([&](const SessionEvent& event) {
        if (event.kind == SessionEvent::Kind::Debugger && event.debug &&
            event.debug->type == DebugEventType::BreakpointHit) {
            saw_breakpoint = true;
            if (event.debug->location) stopped_line = event.debug->location->line;
        }
        if (event.kind == SessionEvent::Kind::Output) output += event.text;
    });

    DebugControl control;
    std::atomic<bool> finished{false};
    bool run_ok = false;
    std::wstring run_error;

    std::thread worker([&] {
        ActiveScriptHost host;
        run_ok = host.run(
            LaunchOptions{
                .script_path = script,
                .language = ScriptLanguage::VBScript,
                .debug = true,
                .break_on_entry = false},
            session,
            run_error,
            &control);
        finished.store(true, std::memory_order_release);
    });

    const auto deadline = std::chrono::steady_clock::now() + 15s;
    bool resumed = false;
    while (!finished.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        if (control.wait(100ms) == DebugWaitResult::Paused) {
            std::wstring resume_error;
            if (!control.resume(ResumeAction::Continue, resume_error)) {
                std::wcerr << L"resume failed: " << resume_error << L"\n";
                control.resume(ResumeAction::Abort, resume_error);
                break;
            }
            resumed = true;
        }
    }

    if (!finished.load(std::memory_order_acquire) && control.paused()) {
        std::wstring ignored;
        control.resume(ResumeAction::Abort, ignored);
    }
    worker.join();

    if (!run_ok) {
        std::wcerr << L"debug run failed: " << run_error << L"\n";
        return 1;
    }
    if (!resumed || !saw_breakpoint) {
        std::wcerr << L"expected breakpoint was not observed\n";
        return 2;
    }
    if (stopped_line != 4) {
        std::wcerr << L"expected stop at line 4, got line " << stopped_line << L"\n";
        return 3;
    }
    if (output.find(L"2") == std::wstring::npos) {
        std::wcerr << L"expected WScript.Echo output was not observed\n";
        return 4;
    }

    std::wcout << L"breakpoint integration test passed\n";
    return 0;
}
