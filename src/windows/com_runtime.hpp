#pragma once
#include <windows.h>
namespace wshdbg::windows {
class ComRuntime {
public:
    ComRuntime(); ~ComRuntime();
    ComRuntime(const ComRuntime&) = delete; ComRuntime& operator=(const ComRuntime&) = delete;
    [[nodiscard]] HRESULT result() const noexcept { return hr_; }
    [[nodiscard]] bool usable() const noexcept { return SUCCEEDED(hr_) || hr_ == RPC_E_CHANGED_MODE; }
private: HRESULT hr_{}; bool owns_uninitialize_{false};
};
}
