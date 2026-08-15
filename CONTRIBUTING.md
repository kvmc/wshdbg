# Contributing

The project is debugger-core first: backend behavior should be represented in portable core types before frontend-specific behavior is added.

For Windows backend changes:

1. Keep Windows SDK and COM types out of `include/wshdbg/core`.
2. Prefer RAII for COM ownership.
3. Preserve HRESULTs in diagnostics when adding failure paths.
4. Add portable tests for state/breakpoint/model behavior and Windows integration tests where an Active Scripting engine is required.
5. Do not add a Visual Studio runtime dependency merely to consume debugger interop APIs; use Windows SDK headers/interfaces directly.
