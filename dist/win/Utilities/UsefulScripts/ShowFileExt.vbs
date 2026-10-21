Dim WSHShell
Set WSHShell = WScript.CreateObject("WScript.Shell")
sTitle1 = "SSH=0"
sTitle2 = "SSH=1"

if WshSHell.RegRead("HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\HideFileExt") = 1 then
WshSHell.RegWrite "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\HideFileExt", "0", "REG_DWORD"
WshSHell.SendKeys "{F5}"
else
WshSHell.RegWrite "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\HideFileExt", "1", "REG_DWORD"
WshSHell.SendKeys "{F5}"
end if

Set WSHShell = Nothing
WScript.Quit(0)