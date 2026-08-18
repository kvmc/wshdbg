#pragma once

#include "wshdbg/core/types.hpp"
#include <filesystem>

namespace wshdbg::windows {

// Represents the source position information needed to create an
// IDebugDocumentContext-backed breakpoint location.
class DebugCodeContext {
public:
    DebugCodeContext(std::filesystem::path file, SourceLocation location) noexcept;

    [[nodiscard]] const SourceLocation& location() const noexcept { return location_; }
    [[nodiscard]] const std::filesystem::path& file() const noexcept { return file_; }

private:
    std::filesystem::path file_;
    SourceLocation location_;
};

} // namespace wshdbg::windows
