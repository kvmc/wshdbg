#pragma once
#include "wshdbg/core/session.hpp"
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace wshdbg::windows {

class ApplicationDebugger;

enum class ScriptLanguage { VBScript, JScript };
enum class ResumeAction { Continue, StepInto, StepOver, StepOut, Abort };
enum class DebugWaitResult { Paused, Completed, Timeout };

struct LaunchOptions {
    std::filesystem::path script_path;
    ScriptLanguage language{ScriptLanguage::VBScript};
    bool debug{false};
    bool break_on_entry{false};
};

class DebugControl {
public:
    DebugControl();
    ~DebugControl();
    DebugControl(const DebugControl&) = delete;
    DebugControl& operator=(const DebugControl&) = delete;
    DebugControl(DebugControl&&) noexcept;
    DebugControl& operator=(DebugControl&&) noexcept;

    DebugWaitResult wait(std::chrono::milliseconds timeout);
    bool paused() const noexcept;
    bool resume(ResumeAction action, std::wstring& error);

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    friend class ApplicationDebugger;
    friend class ActiveScriptHost;
};

class ActiveScriptHost {
public:
    ActiveScriptHost();
    ~ActiveScriptHost();
    ActiveScriptHost(const ActiveScriptHost&) = delete;
    ActiveScriptHost& operator=(const ActiveScriptHost&) = delete;
    ActiveScriptHost(ActiveScriptHost&&) noexcept;
    ActiveScriptHost& operator=(ActiveScriptHost&&) noexcept;
    bool run(
        const LaunchOptions& options,
        DebugSession& session,
        std::wstring& error,
        DebugControl* control = nullptr);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace wshdbg::windows
