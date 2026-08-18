#pragma once

#include "wshdbg/core/types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wshdbg {

struct StackFrameInfo {
    std::uint32_t index{0};
    std::wstring name;
    std::optional<SourceLocation> location;
};

struct EvaluationResult {
    bool success{false};
    std::wstring expression;
    std::wstring value;
    std::wstring type;
    std::wstring error;
};

} // namespace wshdbg
