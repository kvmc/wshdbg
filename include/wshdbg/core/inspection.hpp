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

struct VariableInfo {
    std::wstring name;
    std::wstring type;
    std::wstring value;
    bool expandable{false};
    bool read_only{false};
};

struct EvaluationResult {
    bool success{false};
    std::wstring expression;
    std::wstring value;
    std::wstring type;
    std::wstring error;
};

} // namespace wshdbg
