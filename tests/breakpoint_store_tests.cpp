#include "test.hpp"
#include "wshdbg/core/breakpoint_store.hpp"
using namespace wshdbg;
TEST("breakpoints receive stable increasing ids"){BreakpointStore store;const auto first=store.add({.file="one.vbs",.line=10});const auto second=store.add({.file="one.vbs",.line=20});REQUIRE(first==1);REQUIRE(second==2);}
TEST("breakpoints can be bound and queried by file"){BreakpointStore store;const auto id=store.add({.file="scripts/../scripts/test.vbs",.line=7},L"x > 4");REQUIRE(store.set_state(id,BreakpointState::Bound));const auto matches=store.for_file("scripts/test.vbs");REQUIRE(matches.size()==1);REQUIRE(matches[0].state==BreakpointState::Bound);REQUIRE(matches[0].condition.has_value());}
TEST("removing an unknown breakpoint is non-destructive"){BreakpointStore store;store.add({.file="one.vbs",.line=1});REQUIRE(!store.remove(999));REQUIRE(store.all().size()==1);}
