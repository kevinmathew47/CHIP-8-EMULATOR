@echo off
setlocal
cd /d "%~dp0"
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if not exist chip8.exe (
    echo chip8.exe not found. Build it first from the MSYS2 UCRT64 terminal with: make
    pause
    exit /b 1
)
if "%~1"=="" (
    "%~dp0chip8.exe" roms\Tetris.ch8
) else (
    "%~dp0chip8.exe" %*
)
