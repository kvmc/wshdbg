#pragma once

#include "wshdbg/platform/windows_backend.hpp"
#include <activdbg.h>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace wshdbg::windows {

struct DebugControl::Impl {
    bool capture(IRemoteDebugApplicationThread* thread, std::wstring& error) noexcept;
    void set_stack(std::vector<StackFrameInfo> frames);
    void complete() noexcept;
    DebugWaitResult wait(std::chrono::milliseconds timeout);
    bool paused() const noexcept;
    std::vector<StackFrameInfo> stack() const;
    bool resume(ResumeAction action, std::wstring& error) noexcept;

private:
    void clear_locked() noexcept;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    DWORD application_cookie_{0};
    DWORD thread_cookie_{0};
    std::vector<StackFrameInfo> stack_;
    bool paused_{false};
    bool completed_{false};
};

} // namespace wshdbg::windows
