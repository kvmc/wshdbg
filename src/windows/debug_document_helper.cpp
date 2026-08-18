#include "debug_document_helper.hpp"

namespace wshdbg::windows {

DebugDocumentHelper::DebugDocumentHelper(DebugDocument& document) noexcept
    : document_(document) {}

bool DebugDocumentHelper::register_document() {
    // The helper is intentionally separated from DebugDocument. The next
    // implementation stage injects the Active Debugging service and creates
    // the real IDebugDocumentHelper object.
    registered_ = true;
    return registered_;
}

}
