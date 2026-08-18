# Diagnostic Bundles

wshdbg should make failures reproducible without requiring a debugger attached to wshdbg itself.

A future diagnostic export contains:

```
wshdbg-diagnostic/
├── session.json
├── events.jsonl
├── breakpoints.json
├── scripts/
├── environment.json
└── logs/
```

Supported verbosity levels:

- off
- error
- debug
- trace

The bundle is intended for reporting failed breakpoint binding, COM failures, script engine errors, and UI/backend synchronization issues.
