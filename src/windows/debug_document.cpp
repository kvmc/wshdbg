#include "debug_document.hpp"

#include <algorithm>

namespace wshdbg::windows {

DebugDocument::DebugDocument(std::filesystem::path path, std::wstring source)
    : path_(std::move(path)), source_(std::move(source)) {
    build_line_index();
}

void DebugDocument::build_line_index() {
    line_offsets_.clear();
    line_offsets_.push_back(0);
    for (std::uint32_t i = 0; i < source_.size(); ++i) {
        if (source_[i] == L'\n' && i + 1 < source_.size()) {
            line_offsets_.push_back(i + 1);
        }
    }
}

std::optional<std::uint32_t> DebugDocument::offset_for_line_column(
    std::uint32_t line,
    std::uint32_t column) const noexcept {
    if (line == 0 || column == 0 || line > line_offsets_.size()) return std::nullopt;
    const auto base = line_offsets_[line - 1];
    const auto offset = base + column - 1;
    if (offset > source_.size()) return std::nullopt;
    return offset;
}

std::optional<std::pair<std::uint32_t, std::uint32_t>> DebugDocument::line_column_for_offset(
    std::uint32_t offset) const noexcept {
    if (offset > source_.size()) return std::nullopt;
    const auto it = std::upper_bound(line_offsets_.begin(), line_offsets_.end(), offset);
    const auto index = static_cast<std::size_t>(std::distance(line_offsets_.begin(), it) - 1);
    return std::pair{
        static_cast<std::uint32_t>(index + 1),
        static_cast<std::uint32_t>(offset - line_offsets_[index] + 1)
    };
}

} // namespace wshdbg::windows
