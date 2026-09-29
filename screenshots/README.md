# Proof screenshots

Every image here was produced by the real emulator in this repository: in-game shots come from
`chip8.exe --capture`, with key presses replayed through `--press KEY@FRAME` so each one shows an
actual action; test images come from the headless test runner used by `make test`.

| # | Image | Shows |
|---|---|---|
| 01 | [Bug fixes, before vs after](01_bugfix_before_after.png) | The original code draws upside down, crashes on the first subroutine return and leaves the flags test blank; the fixed core passes every check |
| 02 | [Automated tests](02_test_suite_passing.png) | `make test`: 13 of 13 passing |
| 03 | [Speed control](03_speed_control.png) | Required feature 1: `-` / `=` change speed at runtime |
| 04 | [Savestates](04_savestate.png) | Required feature 2: save, keep playing, load, board restored exactly |
| 05 | [Colour palettes](05_color_palettes.png) | Required feature 3: 8 palettes, including Classic Green Screen, Amber CRT and Neon High-Contrast |
| 06 | [Rewind](06_rewind.png) | Hold Backspace to run time backwards (10 s history) |
| 07 | [Debugger](07_debugger.png) | Registers, disassembly, memory, sprite preview, stack and keypad |
| 08 | [Breakpoint](08_breakpoint.png) | Execution stopped on a breakpoint inside Tetris' spawn logic |
| 09 | [SUPER-CHIP games](09_superchip_games.png) | 128x64 hi-res games running |
| 10 | [Quirk and scrolling tests](10_quirks_and_scrolling_tests.png) | CHIP-8, SUPER-CHIP and XO-CHIP compatibility test results |
| 11 | [Help overlay](11_help_overlay.png) | All controls on one screen (`H`) |
| 12 | [CRT effects](12_crt_effects.png) | Phosphor fade removes CHIP-8 flicker; optional scanlines |
| 13 | [ROM auto-detection](13_rom_autodetect.png) | Blinky broken under plain CHIP-8 quirks vs fixed by automatic detection |
| 14 | [Control detection](14_control_detection.png) | Verified controls for known games; keys detected live for an unknown ROM |
| 15 | [Memory heat-map](15_memory_heatmap.png) | All 4 KB of memory lit up as it is executed, read and written |
| 16 | [Time-travel debugging](16_time_travel_debugging.png) | Stepping forward three instructions, then back two with the state restored exactly |
| 17 | [Web interface](17_web_interface.png) | The browser version at 1920x1080: library, monitor, live readout, proof of fixes and every action on one screen |
| 18 | [IBM logo test on the web](18_web_ibm_logo_test.png) | The test ROMs ship with the web page, so the proof of our fixes can be re-run in any browser |
| 19 | [Tablet layout](19_web_tablet.png) | The same page stacked for narrower screens |
