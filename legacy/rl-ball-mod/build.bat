@echo off
setlocal

cd /d "%~dp0"

echo ========================================
echo   RL Ball Mod - Build Script
echo ========================================
echo.

rem Find GCC compiler
set "GCCDIR=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"

if not exist "%GCCDIR%\g++.exe" (
    echo ERROR: g++ not found at %GCCDIR%
    echo Please install WinLibs: winget install BrechtSanders.WinLibs.POSIX.UCRT
    pause
    exit /b 1
)

echo Using GCC: %GCCDIR%\g++.exe
"%GCCDIR%\g++.exe" --version | findstr "g++"
echo.

echo Compiling winmm.dll...

"%GCCDIR%\g++.exe" -std=c++20 -shared -static -O2 -s -Wall -Wno-cast-function-type ^
    -I"lib/MinHook" ^
    -I"lib" ^
    src/dllmain.cpp ^
    src/proxy.cpp ^
    src/stb_impl.cpp ^
    src/d3d11/tex_hook.cpp ^
    src/d3d11/night_mode.cpp ^
    src/ue3/ue3_globals.cpp ^
    src/ue3/ue3_reflection.cpp ^
    src/hooks/hook_manager.cpp ^
    src/hooks/ball_hooks.cpp ^
    src/texture/texture_loader.cpp ^
    src/texture/texture_manager.cpp ^
    src/util/pattern_scan.cpp ^
    src/util/logger.cpp ^
    src/util/config.cpp ^
    lib/MinHook/hook.c ^
    lib/MinHook/buffer.c ^
    lib/MinHook/trampoline.c ^
    lib/MinHook/hde/hde64.c ^
    lib/MinHook/hde/hde32.c ^
    src/proxy.def ^
    -lpsapi ^
    -lgdi32 ^
    -ld3d11 ^
    -o winmm.dll

if errorlevel 1 (
    echo.
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL!
echo Output: winmm.dll (%~dp0winmm.dll)
echo.
echo Run install.bat to copy to Rocket League directory.

endlocal
