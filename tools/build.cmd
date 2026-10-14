@echo off
setlocal

rem Build an already-configured CMake directory:
rem   cmake --preset windows-msvc        (needs QT_ROOT set, see CMakePresets.json)
rem   tools\build.cmd build\windows-msvc
rem   tools\build.cmd cmake-build-debug --target ExampleGame
rem
rem VsDevCmd is sourced because the Ninja generator has to find cl/ninja/link on
rem PATH itself; cmake does not provide them. Point SRI_VS_DEV_CMD at another
install if yours is not Community edition.

set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not "%SRI_VS_DEV_CMD%"=="" set "VS_DEV_CMD=%SRI_VS_DEV_CMD%"
if not exist "%VS_DEV_CMD%" (
    echo VsDevCmd not found: "%VS_DEV_CMD%" 1>&2
    exit /b 1
)
call "%VS_DEV_CMD%" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b 1

set "BUILD_DIR=build\windows-msvc"
if not "%~1"=="" (
    set "BUILD_DIR=%~1"
    shift
)

set "ARGS="
:collect
if "%~1"=="" goto build
set "ARGS=%ARGS% %~1"
shift
goto :collect

:build
cmake --build "%BUILD_DIR%" %ARGS%
exit /b %errorlevel%
