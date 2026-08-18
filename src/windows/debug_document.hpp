#pragma once

#include <filesystem>
#include <string>

namespace wshdbg::windows {

// Represents a script document known to the debugger. The next layer will map
// this object to IDebugDocumentHelper/IDebugDocumentContext instances.
class DebugDocument {
public:
    explicit DebugDocument(std::filesystem::path path);

    const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace wshdbg::windows
