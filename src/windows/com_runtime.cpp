#include "com_runtime.hpp"
#include <objbase.h>

namespace wshdbg::windows {

ComRuntime::ComRuntime() {
    hr_ = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    owns_uninitialize_ = SUCCEEDED(hr_);
}

ComRuntime::~ComRuntime() {
    if (owns_uninitialize_) {
        CoUninitialize();
    }
}

} // namespace wshdbg::windows
