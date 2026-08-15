# Primary references

The implementation is based on Microsoft's Active Scripting and debugging interfaces, principally:

- `IActiveScript` / `IActiveScriptParse`
- `IActiveScriptSite`
- `IActiveScriptSiteDebug`
- `IProcessDebugManager`
- `IDebugApplication`
- `IDebugDocumentHelper`
- `IApplicationDebugger`

Microsoft's archived MSDN Magazine article "Active Scripting APIs: Add Powerful Custom Debugging to Your Script-Hosting App" remains a useful architectural walkthrough of these interfaces.

Current Microsoft Learn interop documentation also exposes the 32/64-bit Active Scripting debugger interface families in the Visual Studio Debugger Interop namespace. The project does not take a runtime dependency on Visual Studio; these documents are used as interface references alongside Windows SDK headers.
