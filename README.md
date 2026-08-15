# wshdbg

A standalone debugger for Windows Script Host / Active Scripting, designed to provide source-level fidelity without requiring Visual Studio and to grow beyond conventional script debugging into deep runtime and COM instrumentation.

> **Status:** bootstrap / pre-alpha. The repository currently contains the debugger core model and the first hosted Active Scripting path. Source breakpoints and stepping are the next milestone.

## Goals

- Debug VBScript and legacy JScript using the native Active Scripting debugger architecture.
- Launch scripts under an instrumented host and later attach to real WSH processes.
- Source breakpoints, stepping, call stacks, scopes, watches, expression evaluation, and variable editing.
- First-class COM object/call inspection.
- Frontend-neutral core with CLI, DAP, and standalone GUI frontends.
- No Visual Studio installation or runtime dependency.

## Build

Requirements on Windows:

- Windows 10/11 SDK containing `activscp.h` and `activdbg.h`
- CMake 3.24+
- A C++20 compiler (MSVC is the primary supported toolchain)

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The portable core/tests also build on non-Windows systems; the Active Scripting backend is automatically omitted.

## GitHub Actions

Every push to `main` and every pull request builds the portable core on Ubuntu and the full Windows backend on `windows-latest`. The Windows job uploads a `wshdbg-windows-x64` artifact containing the executable and associated build outputs. The workflow also supports manual `workflow_dispatch` runs from the Actions tab.

## Current CLI

```powershell
wshdbg run samples\hello.vbs
wshdbg run samples\hello.js
```

The bootstrap host currently provides a minimal `WScript.Echo` object so the sample scripts can emit console output. Full WSH object-model compatibility and engine-level break-on-entry land in later milestones.

## Repository layout

```text
include/wshdbg/       public API
src/core/             portable debugger model
src/windows/          Active Scripting + COM backend
src/cli/              command-line frontend
tests/                portable unit tests
samples/              script fixtures
docs/                 architecture and roadmap
```

See `docs/roadmap.md` for the implementation sequence.
