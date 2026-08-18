#pragma once
#include "wshdbg/core/session.hpp"
#include <filesystem>
#include <memory>
#include <string>
namespace wshdbg::windows {
enum class ScriptLanguage { VBScript, JScript };
struct LaunchOptions {
    std::filesystem::path script_path;
    ScriptLanguage language{ScriptLanguage::VBScript};
    bool debug{false};
    bool break_on_entry{false};
};
class ActiveScriptHost {
public:
    ActiveScriptHost();
    ~ActiveScriptHost();
    ActiveScriptHost(const ActiveScriptHost&) = delete;
    ActiveScriptHost& operator=(const ActiveScriptHost&) = delete;
    ActiveScriptHost(ActiveScriptHost&&) noexcept;
    ActiveScriptHost& operator=(ActiveScriptHost&&) noexcept;
    bool run(const LaunchOptions& options, DebugSession& session, std::wstring& error);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace wshdbg::windows
