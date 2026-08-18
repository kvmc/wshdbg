#pragma once

#include "debug_document.hpp"
#include <activdbg.h>
#include <string>

namespace wshdbg::windows {

// COM-facing document registration boundary. This object will own the
// IDebugDocumentHelper lifecycle once the Active Debugging service is wired.
class DebugDocumentHelper {
public:
    explicit DebugDocumentHelper(DebugDocument& document) noexcept;

    bool register_document();
    bool registered() const noexcept { return registered_; }

private:
    DebugDocument& document_;
    bool registered_{false};
};

}
