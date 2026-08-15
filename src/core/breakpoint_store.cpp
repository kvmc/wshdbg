#include "wshdbg/core/breakpoint_store.hpp"
#include <algorithm>
#include <utility>
namespace wshdbg {
BreakpointId BreakpointStore::add(SourceLocation location, std::optional<std::wstring> condition) { const auto id = next_id_++; breakpoints_.push_back(Breakpoint{.id=id,.location=std::move(location),.condition=std::move(condition)}); return id; }
bool BreakpointStore::remove(BreakpointId id) { const auto it=std::find_if(breakpoints_.begin(),breakpoints_.end(),[id](const Breakpoint& bp){return bp.id==id;}); if(it==breakpoints_.end()) return false; breakpoints_.erase(it); return true; }
bool BreakpointStore::set_state(BreakpointId id, BreakpointState state, std::optional<std::wstring> diagnostic) { const auto it=std::find_if(breakpoints_.begin(),breakpoints_.end(),[id](const Breakpoint& bp){return bp.id==id;}); if(it==breakpoints_.end()) return false; it->state=state; it->diagnostic=std::move(diagnostic); return true; }
std::optional<Breakpoint> BreakpointStore::get(BreakpointId id) const { const auto it=std::find_if(breakpoints_.begin(),breakpoints_.end(),[id](const Breakpoint& bp){return bp.id==id;}); return it==breakpoints_.end()?std::nullopt:std::optional<Breakpoint>{*it}; }
std::vector<Breakpoint> BreakpointStore::all() const { return breakpoints_; }
std::vector<Breakpoint> BreakpointStore::for_file(const std::filesystem::path& file) const { std::vector<Breakpoint> result; for(const auto& bp:breakpoints_) if(bp.location.file.lexically_normal()==file.lexically_normal()) result.push_back(bp); return result; }
} // namespace wshdbg
