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
    bool saw_step = false;
    std::uint32_t stopped_line = 0;
    std::uint32_t stepped_line = 0;
    std::wstring output;
    session.subscribe([&](const SessionEvent& event) {
        if (event.kind == SessionEvent::Kind::Debugger && event.debug) {
            if (event.debug->type == DebugEventType::BreakpointHit) {
                saw_breakpoint = true;
                if (event.debug->location) stopped_line = event.debug->location->line;
            } else if (event.debug->type == DebugEventType::StepComplete) {
                saw_step = true;
                if (event.debug->location) stepped_line = event.debug->location->line;
            }
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
    bool inspected = false;
    bool stepped = false;
    bool continued = false;

    while (!finished.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        if (control.wait(100ms) != DebugWaitResult::Paused) continue;

        if (!inspected) {
            const auto stack = control.stack();
            if (stack.empty()) {
                std::wcerr << L"expected at least one stack frame\n";
                break;
            }

            const auto variables = control.variables();
            bool found_x = false;
            for (const auto& variable : variables) {
                if (variable.name == L"x") {
                    found_x = true;
                    break;
                }
            }
            if (!found_x) {
                std::wcerr << L"expected local variable x\n";
                break;
            }

            const auto evaluation = control.evaluate(L"x", false);
            if (!evaluation.success || evaluation.value.find(L"1") == std::wstring::npos) {
                std::wcerr << L"expected x to evaluate to 1 before line 4\n";
                break;
            }

            std::wstring resume_error;
            if (!control.resume(ResumeAction::StepOver, resume_error)) {
                std::wcerr << L"step-over failed: " << resume_error << L"\n";
                break;
            }
            inspected = true;
            stepped = true;
            continue;
        }

        std::wstring resume_error;
        if (!control.resume(ResumeAction::Continue, resume_error)) {
            std::wcerr << L"continue failed: " << resume_error << L"\n";
            break;
        }
        continued = true;
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
    if (!saw_breakpoint || !inspected) {
        std::wcerr << L"expected breakpoint inspection was not observed\n";
        return 2;
    }
    if (stopped_line != 4) {
        std::wcerr << L"expected stop at line 4, got line " << stopped_line << L"\n";
        return 3;
    }
    if (!stepped || !saw_step) {
        std::wcerr << L"expected step-complete event was not observed\n";
        return 4;
    }
    if (stepped_line != 5) {
        std::wcerr << L"expected step to line 5, got line " << stepped_line << L"\n";
        return 5;
    }
    if (!continued) {
        std::wcerr << L"expected final continue was not issued\n";
        return 6;
    }
    if (output.find(L"2") == std::wstring::npos) {
        std::wcerr << L"expected WScript.Echo output was not observed\n";
        return 7;
    }

    std::wcout << L"breakpoint/inspection/step integration test passed\n";
    return 0;
}
