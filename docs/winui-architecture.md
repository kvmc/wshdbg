# wshdbg Windows UI Architecture

The primary user experience is a native Windows debugger. VS Code/DAP support is optional and not the primary interface.

## Architecture

```
wshdbg-ui (WinUI 3)
        |
        v
wshdbg-core
        |
        v
wshdbg-win
        |
        +-- Active Scripting
        +-- COM
        +-- Windows debugging APIs
```

## First UI milestone

- Create a native WinUI 3 application shell.
- Host the debugger session model.
- Display script documents.
- Display breakpoint state.
- Display execution events.

## Debugger panels

- Source editor
- Call stack
- Locals
- Watches
- COM object inspector
- Active Script engine state
- Runtime event timeline
