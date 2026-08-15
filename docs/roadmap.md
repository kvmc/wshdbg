# Roadmap

## M0 — Bootstrap (current)

- [x] CMake project and Windows CI.
- [x] Portable debugger session state machine.
- [x] Breakpoint store with pending/bound/error states.
- [x] COM runtime wrapper.
- [x] Process Debug Manager application registration.
- [x] Minimal hosted `IActiveScript` execution path.
- [x] Script error callback plumbing.
- [x] Provide minimal `WScript.Echo` named object.
- [ ] Complete WSH named objects (arguments, stdin/stdout/stderr, object creation, host properties).
- [ ] Probe Microsoft PDM availability without making it a runtime requirement.

## M1 — Real source debugger

- [ ] Implement `IActiveScriptSiteDebug` / 64-bit equivalent.
- [ ] Implement/own the debug-application services needed when Microsoft PDM is absent; no Visual Studio dependency is allowed.
- [ ] Implement `IApplicationDebugger`.
- [ ] Register `IDebugDocumentHelper` source documents.
- [ ] Map line/column breakpoints to `IDebugCodeContext`.
- [ ] Bind/unbind breakpoints.
- [ ] Break on entry.
- [ ] Continue, pause, step into, step over, step out.
- [ ] Enumerate script stack frames.
- [ ] Locals/globals and expression evaluation.
- [ ] Change variable/property values where supported.

## M2 — WSH fidelity

- [ ] `WScript` automation object.
- [ ] `.wsf` package parsing and multi-engine jobs.
- [ ] `WScript.Arguments`, `StdIn`, `StdOut`, `StdErr`.
- [ ] `CreateObject`, `GetObject`, event connections.
- [ ] 32-bit and 64-bit host builds.
- [ ] Compatibility suite against `cscript.exe` behavior.

## M3 — Frontends

- [ ] Interactive CLI/REPL.
- [ ] Debug Adapter Protocol server.
- [ ] Standalone GUI.
- [ ] Source editor, stack, scopes, watches, immediate window.

## M4 — Instrumentation

- [ ] Rich COM object inspection.
- [ ] COM activation/call timeline.
- [ ] Semantic breakpoints over COM activity.
- [ ] Execution trace and state-change history.
- [ ] Exportable trace format.
