#include "com_runtime.hpp"
namespace wshdbg::windows {
ComRuntime::ComRuntime(){ hr_=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED); owns_uninitialize_=SUCCEEDED(hr_); }
ComRuntime::~ComRuntime(){ if(owns_uninitialize_) CoUninitialize(); }
}
