# Architecture

`wshdbg` is a standalone debugger for Windows Active Scripting. It is intentionally split into a portable debugger model and Windows-specific runtime adapters.

## Components

- `wshdbg_core`: session lifecycle, breakpoint model, events, and later stack/property abstractions.
- `wshdbg_windows`: COM initialization, Active Scripting hosting, Process Debug Manager integration, and eventually attach mode.
- `wshdbg`: CLI frontend. The first UI contract is deliberately terminal-oriented so debugger primitives stabilize before a graphical frontend is added.

## Debugging architecture

The Microsoft Active Scripting debugging model revolves around the Process Debug Manager, `IDebugApplication`, script-site debugging interfaces, document helpers, code contexts, stack frames, expressions, and `IApplicationDebugger`. `wshdbg` uses those interfaces directly rather than automating Visual Studio.

### Hosted mode

1. Initialize COM.
2. Create an Active Scripting engine (`VBScript` or `JScript`).
3. Supply an `IActiveScriptSite` and the WSH-compatible named objects.
4. In milestone 1, supply `IActiveScriptSiteDebug` and an `IDebugApplication` service. Microsoft PDM may be used when present, but is not a required dependency.
5. Register source text as a debug document.
6. Parse and execute the script.
7. Receive break/error callbacks and map them into the portable `DebugSession` model.

### Attach mode

Attach mode is deliberately deferred until hosted mode has correct source/document semantics. It will enumerate script applications through the machine/process debug manager and connect the same application-debugger implementation.

## Design rules

- No Visual Studio dependency.
- Windows SDK interfaces only in the backend.
- Debugger state is frontend-neutral.
- Script-language assumptions belong in adapters, not the core.
- Every backend feature must have a deterministic core model and tests where feasible.
- Source fidelity comes before GUI work.

## PDM dependency policy

Historical Microsoft documentation describes `PDM.DLL` as shipping with the scripting components, while current Visual Studio documentation describes a PDM as a Visual Studio debugger component. `wshdbg` therefore treats Microsoft PDM availability as an optional compatibility path, not as a prerequisite. The long-term hosted debugger must own enough of the debug-application/document plumbing to work on a machine that has WSH/Active Scripting but no Visual Studio installation.
