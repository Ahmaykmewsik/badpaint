@echo off
setlocal EnableDelayedExpansion

set MODE= -Od -DDEBUG_MODE
REM set MODE= -O2

REM max warnings, warnings are errors
set WARNINGS=-W4 -WX
REM enable non-standard structs
set WARNINGS=%WARNINGS% /wd4201
REM enable unreferenced formal parameter
set WARNINGS=%WARNINGS% /wd4100
REM enable local variable initialized but not referenced
set WARNINGS=%WARNINGS% /wd4189
REM uncomment this line to disable warnings
REM set WARNINGS=

set FLAGS= -nologo -EHsc -Zi -DOS_WINDOWS %WARNINGS%
set CODEPATHS= ../code/platform_win32.cpp ../code/main.cpp
set INCLUDES= -I ../includes -I ../code -I ../code/base
set LIBRARIES= ./libraries/builtLibraries.lib user32.lib shell32.lib gdi32.lib winmm.lib kernel32.lib Ole32.lib winhttp.lib dbghelp.lib comctl32.lib onecore.lib
set DIRECTORY_NAME="../badpaint"

cl %FLAGS% %MODE% %CODEPATHS% %INCLUDES% %LIBRARIES% -Fe"%DIRECTORY_NAME%/badpaint.exe"

if %ERRORLEVEL% equ 0 (
  printf "\nBuild Complete :)"
) else (
  printf "\nBuild Failed. Try again^!" 
)

popd
