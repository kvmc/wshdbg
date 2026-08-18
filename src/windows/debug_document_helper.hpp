#pragma once

#include "active_debug_services.hpp"
#include "debug_document.hpp"
#include <activdbg.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>

namespace wshdbg::windows {

#ifdef _WIN64
using SourceContext = DWORDLONG;
#else
using SourceContext = DWORD;
#endif

class DebugDocumentHelper {
public:
    explicit DebugDocumentHelper(DebugDocument& document) noexcept;
    ~DebugDocumentHelper();

    DebugDocumentHelper(const DebugDocumentHelper&) = delete;
    DebugDocumentHelper& operator=(const DebugDocumentHelper&) = delete;

    bool register_document(
        ActiveDebugServices& services,
        IActiveScript* engine,
        std::wstring& error);
    void unregister_document() noexcept;

    bool registered() const noexcept { return helper_ != nullptr; }
    SourceContext source_context() const noexcept { return source_context_; }
    DebugDocumentHelperInterface* native() const noexcept { return helper_.Get(); }

    HRESULT create_context(
        std::uint32_t character_offset,
        std::uint32_t character_count,
        IDebugDocumentContext** context) const noexcept;
    HRESULT get_application_node(IDebugApplicationNode** node) const noexcept;

private:
    DebugDocument& document_;
    Microsoft::WRL::ComPtr<DebugDocumentHelperInterface> helper_;
    SourceContext source_context_{0};
};

} // namespace wshdbg::windows
