@echo off

@echo off
setlocal EnableDelayedExpansion

set BUILD_CACHE=%~dp0\_build_cache.cmd

if exist "!BUILD_CACHE!" (
  rem cache file exists, so call it to set env variables very fast
  call "!BUILD_CACHE!"
) else (
  if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Visual Studio 2019 not found or installed version is to old.
    exit /b 1
  )
  rem cache file does not exist, get env variables in slow way
  call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
  echo set PATH=!PATH!> "!BUILD_CACHE!"
  echo set INCLUDE=!INCLUDE!>> "!BUILD_CACHE!"
  echo set LIB=!LIB!>> "!BUILD_CACHE!"
)

set LIB_DIR="libraries"
if not exist "%LIB_DIR%" (
    mkdir "%LIB_DIR%"
)
cd %LIB_DIR%

set FLAGS=-Zi -c -EHsc -O2

printf "\n-------Building raylib-------\n\n"

set FILE_PATH=..\..\includes\raylib\src
@REM 
cl %FLAGS% ^
 %FILE_PATH%\rcore.c^
 %FILE_PATH%\raudio.c^
 %FILE_PATH%\rglfw.c^
 %FILE_PATH%\rmodels.c^
 %FILE_PATH%\rshapes.c^
 %FILE_PATH%\rtext.c^
 %FILE_PATH%\rtextures.c^
 %FILE_PATH%\utils.c

printf "\n-------Building Static Library-------\n\n"

lib /OUT:builtLibraries.lib %newDirectory%*.obj 

popd

if %ERRORLEVEL% equ 0 (
  printf "\n\033[0;32mLibraries Built!"
) else (
  printf "\n\033[0;31mBuild Failed. Try again^!" 
)
Pause>nul 