# Implementation Status

## Completed

- Debug session event model
- Active Script host bootstrap
- Active Script debug site boundary
- Debug application abstraction
- Debug document abstraction
- Breakpoint binding boundary
- Diagnostic design

## In Progress

- IDebugApplication integration
- IDebugDocumentHelper integration
- Source context mapping
- Real breakpoint binding
- Execution control

## Goal

Reach:

```
wshdbg debug test.vbs

break test.vbs:10

running...

paused test.vbs:10
```

All engine state should remain observable through diagnostics.
