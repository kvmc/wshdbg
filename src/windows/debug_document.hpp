#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wshdbg::windows {

class DebugDocument {
public:
    DebugDocument(std::filesystem::path path, std::wstring source);

    const std::filesystem::path& path() const noexcept { return path_; }
    std::wstring_view source() const noexcept { return source_; }
    std::size_t size() const noexcept { return source_.size(); }
    std::size_t line_count() const noexcept { return line_offsets_.size(); }

    std::optional<std::uint32_t> offset_for_line_column(
        std::uint32_t line,
        std::uint32_t column = 1) const noexcept;

    std::optional<std::pair<std::uint32_t, std::uint32_t>> line_column_for_offset(
        std::uint32_t offset) const noexcept;

private:
    void build_line_index();

    std::filesystem::path path_;
    std::wstring source_;
    std::vector<std::uint32_t> line_offsets_;
};

} // namespace wshdbg::windows
