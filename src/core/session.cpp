#include "wshdbg/core/session.hpp"
#include <utility>
namespace wshdbg {
void DebugSession::subscribe(EventHandler handler){handlers_.push_back(std::move(handler));}
bool DebugSession::can_transition(SessionState from,SessionState to) noexcept { if(from==to)return true; switch(from){case SessionState::Created:return to==SessionState::Launching||to==SessionState::Stopped||to==SessionState::Failed;case SessionState::Launching:return to==SessionState::Running||to==SessionState::Paused||to==SessionState::Stopping||to==SessionState::Stopped||to==SessionState::Failed;case SessionState::Running:return to==SessionState::Paused||to==SessionState::Stopping||to==SessionState::Stopped||to==SessionState::Failed;case SessionState::Paused:return to==SessionState::Running||to==SessionState::Stopping||to==SessionState::Stopped||to==SessionState::Failed;case SessionState::Stopping:return to==SessionState::Stopped||to==SessionState::Failed;case SessionState::Stopped:case SessionState::Failed:return false;} return false; }
bool DebugSession::transition(SessionState next){if(!can_transition(state_,next))return false;state_=next;if(next==SessionState::Running)last_stop_.reset();publish(SessionEvent{.kind=SessionEvent::Kind::StateChanged,.state=state_});return true;}
void DebugSession::stopped(StopInfo info){if(state_!=SessionState::Paused&&!transition(SessionState::Paused))return;last_stop_=std::move(info);publish(SessionEvent{.kind=SessionEvent::Kind::Stopped,.state=state_,.stop=last_stop_});}
void DebugSession::output(std::wstring_view text){publish(SessionEvent{.kind=SessionEvent::Kind::Output,.state=state_,.text=std::wstring{text}});}
void DebugSession::publish(const SessionEvent& event) const {for(const auto& handler:handlers_)handler(event);}
} // namespace wshdbg
