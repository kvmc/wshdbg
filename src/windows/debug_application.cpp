#include "debug_application.hpp"

namespace wshdbg::windows {

DebugApplication::DebugApplication() = default;

DebugApplication::~DebugApplication() {
    shutdown();
}

bool DebugApplication::initialize(const std::wstring& name, std::wstring& error) {
    return services_.initialize(name, error);
}

void DebugApplication::shutdown() {
    services_.shutdown();
}

} // namespace wshdbg::windows
