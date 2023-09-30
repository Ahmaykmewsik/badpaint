@echo off
setlocal EnableDelayedExpansion

set BUILD_CACHE=%~dp0\_build_cache.cmd

if exist "!BUILD_CACHE!" (
  rem cache file exists, so call it to set env variables very fast
  call "!BUILD_CACHE!"
) else (
  if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Visual Studio not found or installed version is to old.
    exit /b 1
  )
  rem cache file does not exist, get env variables in slow way
  call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
  echo set PATH=!PATH!> "!BUILD_CACHE!"
  echo set INCLUDE=!INCLUDE!>> "!BUILD_CACHE!"
  echo set LIB=!LIB!>> "!BUILD_CACHE!"
)

set DIRECTORY_NAME="..\badpaint"

IF NOT EXIST "%DIRECTORY_NAME%" (
    mkdir "%DIRECTORY_NAME%"
)

set CODEPATHS= ..\code\platform_win32.cpp ..\code\main.cpp
set INCLUDES= -I ..\includes\ 
set LIBRARIES= .\libraries\builtLibraries.lib user32.lib shell32.lib gdi32.lib winmm.lib kernel32.lib Ole32.lib winhttp.lib dbghelp.lib comctl32.lib

@REM Uncomment when you want to update the application icon
@REM rc resources.rc

set COMMAND= cl -EHsc -Zi -O2 %CODEPATHS% %INCLUDES% %LIBRARIES% /Fe"%DIRECTORY_NAME%\badpaint.exe"

@REM%COMMAND% 

@REM if %ERRORLEVEL% equ 0 (
@REM   printf "\n\033[0;32mBuild Complete :)"
@REM ) else (
@REM   printf "\n\033[0;31mBuild Failed. Try again^!" 

%COMMAND% 2>&1 > temp_output.txt

set "build_status=%ERRORLEVEL%"

for /f "tokens=1* delims=:" %%i in ('type temp_output.txt ^| findstr /n "^"') do (
    echo(%%j
    if %%i equ 60 pause >nul
)

if %build_status% equ 0 (
  printf "\n\033[0;32mBuild Complete :)"
) else (
  printf "\n\033[0;31mBuild Failed. Try again^!" 
)

del temp_output.txt

popd

pause>nul