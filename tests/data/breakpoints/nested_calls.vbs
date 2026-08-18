Option Explicit

Sub Inner(value)
    WScript.Echo value
End Sub

Sub Outer(value)
    Inner value + 1
End Sub

Outer 41
