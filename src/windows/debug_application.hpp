#pragma once

#include "active_debug_services.hpp"
#include <string>

namespace wshdbg::windows {

class DebugApplication {
public:
    DebugApplication();
    ~DebugApplication();

    DebugApplication(const DebugApplication&) = delete;
    DebugApplication& operator=(const DebugApplication&) = delete;

    bool initialize(const std::wstring& name, std::wstring& error);
    void shutdown();

    bool initialized() const noexcept { return services_.available(); }
    DebugApplicationInterface* native() const noexcept { return services_.application(); }
    ActiveDebugServices& services() noexcept { return services_; }
    const ActiveDebugServices& services() const noexcept { return services_; }

private:
    ActiveDebugServices services_;
};

} // namespace wshdbg::windows
