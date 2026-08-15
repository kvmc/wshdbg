#pragma once
#include "wshdbg/core/types.hpp"
#include <optional>
#include <vector>
namespace wshdbg {
class BreakpointStore {
public:
    BreakpointId add(SourceLocation location, std::optional<std::wstring> condition = std::nullopt);
    bool remove(BreakpointId id);
    bool set_state(BreakpointId id, BreakpointState state, std::optional<std::wstring> diagnostic = std::nullopt);
    [[nodiscard]] std::optional<Breakpoint> get(BreakpointId id) const;
    [[nodiscard]] std::vector<Breakpoint> all() const;
    [[nodiscard]] std::vector<Breakpoint> for_file(const std::filesystem::path& file) const;
private:
    BreakpointId next_id_{1};
    std::vector<Breakpoint> breakpoints_;
};
} // namespace wshdbg
