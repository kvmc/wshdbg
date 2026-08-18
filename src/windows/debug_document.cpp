#include "debug_document.hpp"

namespace wshdbg::windows {

DebugDocument::DebugDocument(std::filesystem::path path)
    : path_(std::move(path)) {}

} // namespace wshdbg::windows
