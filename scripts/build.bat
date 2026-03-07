@echo off
REM ── MindTrace Build Script (Windows) ──────────────────────────────────

setlocal enabledelayedexpansion

set BUILD_DIR=build
set BUILD_TYPE=Release

REM Parse arguments
:parse_args
if "%~1"=="" goto :done_args
if /I "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /I "%~1"=="--clean" (
    echo [*] Cleaning build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
    shift
    goto :parse_args
)
if /I "%~1"=="--test" (
    set RUN_TESTS=1
    shift
    goto :parse_args
)
if /I "%~1"=="--help" (
    goto :show_help
)
shift
goto :parse_args
:done_args

echo.
echo   ╔══════════════════════════════════════╗
echo   ║       MindTrace Build Script         ║
echo   ╚══════════════════════════════════════╝
echo.
echo   Build Type: %BUILD_TYPE%
echo.

REM Create build directory
if not exist %BUILD_DIR% (
    echo [1/3] Creating build directory...
    mkdir %BUILD_DIR%
)

REM Configure
echo [2/3] Configuring with CMake...
cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

REM Build
echo [3/3] Building...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo   [SUCCESS] Build complete!
echo   Binary: %BUILD_DIR%\%BUILD_TYPE%\mindtrace.exe
echo.

REM Run tests if requested
if defined RUN_TESTS (
    echo [*] Running tests...
    cd %BUILD_DIR%
    ctest --build-config %BUILD_TYPE% --output-on-failure
    cd ..
)

exit /b 0

:show_help
echo Usage: build.bat [options]
echo.
echo Options:
echo   --debug    Build in Debug mode (default: Release)
echo   --clean    Clean build directory before building
echo   --test     Run tests after building
echo   --help     Show this help message
exit /b 0
