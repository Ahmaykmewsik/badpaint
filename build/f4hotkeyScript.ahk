#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.
#SingleInstance Force 

F3::
if WinActive("ahk_exe cmd.exe")
    Send, {Esc} 
Run, buildLibraries.bat 
return

F4::
if WinActive("ahk_exe cmd.exe")
    Send, {Esc} 
Run, buildGame.bat 
return

$F6::
if WinActive("ahk_exe cmd.exe")
{
    Send, {Esc} 
    WinActivate ahk_class rdbg_MainWindowClass
    return
}

if WinActive("ahk_exe Code.exe")
{
    WinActivate ahk_class rdbg_MainWindowClass
    return
}
if WinActive("ahk_class rdbg_MainWindowClass") 
{
    Send, +{F5}
    WinActivate ahk_exe Code.exe
}

return