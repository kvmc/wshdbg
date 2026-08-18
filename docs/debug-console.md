# Debug Console and Diagnostics

## Goal

wshdbg should always have a way to capture enough information to diagnose failures without requiring a debugger attached to wshdbg itself.

## Console modes

Planned modes:

- `off` - normal user operation
- `errors` - failures only
- `debug` - debugger lifecycle events
- `trace` - COM, Active Script, document, breakpoint, and execution events

Example:

```powershell
wshdbg --log-level debug script.vbs
```

## Output

Support:

- console output
- rotating log files
- JSON event stream for issue reports

Example event:

```json
{
  "event": "BreakpointHit",
  "script": "test.vbs",
  "line": 10,
  "thread": 42
}
```

The debug stream should be usable by developers to report broken behavior with reproduction data.
