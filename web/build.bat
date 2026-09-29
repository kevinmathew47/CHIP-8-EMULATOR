@echo off
rem Windows helper for the browser build (same steps as "make web").
rem Needs the Emscripten SDK; set EMSDK_DIR if it is not at ..\..\emsdk.
setlocal
cd /d "%~dp0.."
if "%EMSDK_DIR%"=="" set "EMSDK_DIR=%~dp0..\..\emsdk"
call "%EMSDK_DIR%\emsdk_env.bat" >nul 2>&1 || (echo Emscripten SDK not found at %EMSDK_DIR% & exit /b 1)
if not exist web\build\assets mkdir web\build\assets
copy /y screenshots\01_bugfix_before_after.png web\build\assets\bugfix.png >nul
em++ -std=c++17 -O2 -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 ^
  -sEXPORTED_FUNCTIONS=_main,_web_load_rom,_web_hotkey,_web_set_key,_web_set_rewind,_web_get_info ^
  -sEXPORTED_RUNTIME_METHODS=ccall,FS ^
  --preload-file roms --preload-file tests/roms@test-roms ^
  --shell-file web/shell.html -o web/build/index.html ^
  src/main.cpp src/chip8.cpp src/text.cpp src/disasm.cpp src/debugger.cpp
if errorlevel 1 exit /b 1
rem Publish: docs\ is what GitHub Pages serves
if not exist docs mkdir docs
xcopy /e /y /q web\build\* docs\ >nul
type nul > docs\.nojekyll
echo Built web\build\index.html. Serve it with: python -m http.server 8000 --directory web\build
echo Also copied to docs\ for GitHub Pages.
