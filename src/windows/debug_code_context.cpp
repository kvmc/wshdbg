#include "debug_code_context.hpp"

namespace wshdbg::windows {

DebugCodeContext::DebugCodeContext(std::filesystem::path file, SourceLocation location) noexcept
    : file_(std::move(file)), location_(std::move(location)) {}

} // namespace wshdbg::windows
