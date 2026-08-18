#include "debug_application.hpp"

namespace wshdbg::windows {

DebugApplication::DebugApplication() = default;

DebugApplication::~DebugApplication() {
    shutdown();
}

bool DebugApplication::initialize(const std::wstring&) {
    // Active Debugging document/application registration will be connected here.
    // Keeping this lifecycle explicit prevents hidden debugger dependencies.
    initialized_ = true;
    return true;
}

void DebugApplication::shutdown() {
    initialized_ = false;
}

} // namespace wshdbg::windows
