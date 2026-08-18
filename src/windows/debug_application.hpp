#pragma once

#include <activdbg.h>
#include <atomic>
#include <string>

namespace wshdbg::windows {

// Owns the Active Debugging application boundary. This object will eventually
// provide the bridge between Active Script engines and wshdbg-core execution
// state. It intentionally does not depend on Visual Studio/PDM services.
class DebugApplication {
public:
    DebugApplication();
    ~DebugApplication();

    DebugApplication(const DebugApplication&) = delete;
    DebugApplication& operator=(const DebugApplication&) = delete;

    bool initialize(const std::wstring& name);
    void shutdown();

    bool initialized() const noexcept { return initialized_; }

private:
    bool initialized_{false};
};

} // namespace wshdbg::windows
