# Implementation Roadmap

## Debug Engine

- Debug application ownership
- Script document registration
- Code context mapping
- Breakpoint binding
- Continue/step control

## Diagnostics

Every subsystem should emit structured events:

- Active Script lifecycle
- COM failures
- breakpoint state changes
- script exceptions
- UI/backend synchronization

## Integration Testing

Integration tests should consume fixtures from `tests/data` and produce diagnostics on failure.
