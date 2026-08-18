#include "debug_document_helper.hpp"
#include "wshdbg/core/logging.hpp"

namespace wshdbg::windows {

DebugDocumentHelper::DebugDocumentHelper(DebugDocument& document) noexcept
    : document_(document) {}

DebugDocumentHelper::~DebugDocumentHelper() {
    unregister_document();
}

bool DebugDocumentHelper::register_document(
    ActiveDebugServices& services,
    IActiveScript* engine,
    std::wstring& error) {
    unregister_document();
    if (!services.available() || !engine) {
        error = L"Active Debugging services or script engine unavailable";
        return false;
    }

    HRESULT hr = services.create_document_helper(nullptr, &helper_);
    if (FAILED(hr) || !helper_) {
        error = L"CreateDebugDocumentHelper failed";
        return false;
    }

    const auto short_name = document_.path().filename().wstring();
    const auto long_name = document_.path().wstring();
    hr = helper_->Init(
        services.application(),
        short_name.c_str(),
        long_name.c_str(),
        TEXT_DOC_ATTR_READONLY);
    if (FAILED(hr)) {
        error = L"IDebugDocumentHelper::Init failed";
        unregister_document();
        return false;
    }

    hr = helper_->Attach(nullptr);
    if (FAILED(hr)) {
        error = L"IDebugDocumentHelper::Attach failed";
        unregister_document();
        return false;
    }

    const std::wstring source{document_.source()};
    hr = helper_->AddUnicodeText(source.c_str());
    if (FAILED(hr)) {
        error = L"IDebugDocumentHelper::AddUnicodeText failed";
        unregister_document();
        return false;
    }

    hr = helper_->DefineScriptBlock(
        0,
        static_cast<ULONG>(source.size()),
        engine,
        FALSE,
        &source_context_);
    if (FAILED(hr)) {
        error = L"IDebugDocumentHelper::DefineScriptBlock failed";
        unregister_document();
        return false;
    }

    Logger::instance().write(LogLevel::Debug, L"document", L"Registered Active Debugging document");
    return true;
}

void DebugDocumentHelper::unregister_document() noexcept {
    if (helper_) helper_->Detach();
    helper_.Reset();
    source_context_ = 0;
}

HRESULT DebugDocumentHelper::create_context(
    std::uint32_t character_offset,
    std::uint32_t character_count,
    IDebugDocumentContext** context) const noexcept {
    if (!context) return E_POINTER;
    *context = nullptr;
    if (!helper_) return E_UNEXPECTED;
    return helper_->CreateDebugDocumentContext(character_offset, character_count, context);
}

HRESULT DebugDocumentHelper::get_application_node(IDebugApplicationNode** node) const noexcept {
    if (!node) return E_POINTER;
    *node = nullptr;
    if (!helper_) return E_UNEXPECTED;
    return helper_->GetDebugApplicationNode(node);
}

} // namespace wshdbg::windows
