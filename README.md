<div align="center">

# 🕹️ CHIP-8 Emulator

**A CHIP-8 / SUPER-CHIP emulator with a time-travel debugger, in C++17 + SDL2, that also runs in your browser.**

Built by **Kevin Mathew** for TatHack '26 · PS1

### 🔴 Live demo: **[kevinmathew47.github.io/CHIP-8-EMULATOR](https://kevinmathew47.github.io/CHIP-8-EMULATOR)**
<sub>Runs in any modern browser, desktop or phone. No install.</sub>

[![Play in browser](https://img.shields.io/badge/▶_Play_in_browser-ffb547?style=for-the-badge)](https://kevinmathew47.github.io/CHIP-8-EMULATOR)
&nbsp;
![Tests](https://img.shields.io/badge/tests-13%2F13_passing-4ade80?style=for-the-badge)
&nbsp;
![C++17](https://img.shields.io/badge/C%2B%2B17-SDL2-60a5fa?style=for-the-badge)
&nbsp;
![WebAssembly](https://img.shields.io/badge/WebAssembly-Emscripten-a78bfa?style=for-the-badge)

<img src="screenshots/17_web_interface.png" alt="The emulator's web interface: game library on the left, the monitor in the centre, all controls on the right" width="100%">

</div>

---

## ⚡ Quick start

| | |
|---|---|
| 🌐 **Browser** | Open **[kevinmathew47.github.io/CHIP-8-EMULATOR](https://kevinmathew47.github.io/CHIP-8-EMULATOR)**, pick a game, click the screen |
| 🪟 **Windows** | Install [MSYS2](https://www.msys2.org/) → `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 make` → `make` → double-click **`play.bat`** |
| 🐧 **Linux / macOS** | `sudo apt install g++ make libsdl2-dev` (or `brew install sdl2`) → `make` → `./chip8 roms/Tetris.ch8` |
| ✅ **Tests** | `make test`, no window needed |

---

## 🐞 The bugs we fixed

The same test ROMs on the **original** code (left) and our **fixed** code (right):

<img src="screenshots/01_bugfix_before_after.png" alt="Before: upside-down IBM logo, opcode test cut short, flags test blank. After: every check passes" width="80%">

| | Bug | Symptom | Fix |
|---|---|---|---|
| 🔁 | `00EE` read the stack before moving the pointer | Every subroutine return crashed | Decrement, then read |
| ⏱️ | `SDL_Delay` after **every** instruction; timers ticked per instruction | CPU crawled, timers 10× too fast | One frame of instructions, then timers once at 60 Hz |
| 🙃 | Rows drawn at `31 - y` | Screen upside down | Draw at `y` |
| 🚩 | `>` instead of `>=`, VF written before the result | Wrong carry/borrow flags | `>=`, write VF last |
| 🔢 | BCD tens digit, `FX55/65` off by one, `FX0A` never waits | Wrong scores, lost registers, skipped menus | Spec-correct versions |

➡️ All 17 fixes with line numbers and reasoning: **[TECHNICAL.md](TECHNICAL.md#debugging-phase-bugs-fixed)**

---

## 🧭 How we found and fixed them

```mermaid
flowchart LR
    A["👀 Symptom<br/>wrong screen, crash, bad timing"] --> B[🧪 Run public<br/>test ROM]
    B --> C["🖥️ Headless runner<br/>prints the screen as text"]
    C --> D{"Tick for<br/>every check?"}
    D -- no --> E["📖 Compare the code<br/>with the CHIP-8 spec"]
    E --> F[🔧 Fix]
    F --> B
    D -- yes --> G["💾 Save as expected output<br/>make test guards it forever"]
```

---

## 🏗️ Architecture

```mermaid
flowchart TB
    ROM[(📼 ROM file)] --> CORE

    subgraph CORE ["⚙️ chip8.cpp · the machine"]
        direction LR
        F[Fetch] --> D[Decode] --> X[Execute]
        X --> MEM["4 KB memory · V0-VF · I · stack"]
        X --> DISP["128×64 display buffer"]
        X --> TIM[Delay / sound timers]
    end

    CORE --> STATE[["Chip8State<br/>one struct = whole machine"]]
    STATE --> SAVE[💾 Savestates]
    STATE --> REW["⏪ Rewind · 600 frames"]
    STATE --> BACK["⏮️ Step back · 1000 instructions"]

    CORE --> FE["🖼️ main.cpp · SDL2 front end<br/>60 Hz loop · input · audio · drawing"]
    FE --> DBG["🐛 Debugger · disassembler · heat-map"]
    FE --> DESK[🪟 Desktop app]
    FE --> WEB["🌐 Emscripten → WebAssembly page"]
```

**Every frame (1/60 s):**

```mermaid
flowchart LR
    I[⌨️ Read keys] --> R["▶️ Run N instructions<br/>N = speed setting"]
    R --> T[⏱️ Tick timers once]
    T --> S["🔊 Beep while<br/>sound timer &gt; 0"]
    S --> P[🎨 Draw with palette<br/>+ phosphor fade]
    P --> W[⏳ Wait for the<br/>next 1/60 s]
    W --> I
```

---

## 🖥️ Interface

```
┌──────────────────┬──────────────────────────────────────┬──────────────────┐
│ 01 NOW PLAYING   │ ┌──────────────────────────────────┐ │ 04 EMULATION     │
│  Tetris          │ │                                  │ │  Pause   Reset   │
│  Q ROTATE W LEFT │ │                                  │ │  Slower  Faster  │
│ ┌──┬──┬──┬──┐    │ │           GAME  SCREEN           │ │  Hold to rewind  │
│ │1 │2 │3 │C │    │ │                                  │ │ 05 SAVE STATES   │
│ ├──┼──┼──┼──┤    │ │                                  │ │  Save  Load  ‹0› │
│ │4 │5 │6 │D │    │ │                                  │ │ 06 DEBUGGER      │
│ └──┴──┴──┴──┘    │ └──────────────────────────────────┘ │  Open  Map       │
│ 02 LIBRARY       │  ● RUNNING   ♫ SOUND                 │  Step  Back      │
│  Games           │ SPEED│QUIRKS│PALETTE│DISPLAY│WAVE    │ 07 DISPLAY/SOUND │
│  Super-CHIP      ├──────────────────────────────────────┤  Palette  Mute   │
│  Test ROMs       │ 03 PROOF OF OUR FIXES                │  Test sound      │
│  [Open .ch8]     │  before / after + "Run IBM logo"     │ 08 KEYBOARD      │
└──────────────────┴──────────────────────────────────────┴──────────────────┘
```

Keys the current game uses glow green on the keypad. The desktop app has the same features,
driven by the keyboard (`H` shows every key).

---

## ✨ Features

| Required | | Extra | |
|---|---|---|---|
| ⚡ **Speed control** | `-` `=` · 60 to 60,000 instructions/s | ⏮️ **Time-travel debugger** | `F4` steps *backwards* |
| 💾 **Savestates** | `F5` / `F9` · 10 slots | 🔥 **Memory heat-map** | `F3` · watch code run |
| 🎨 **8 palettes** | `Tab` · Green Screen, Amber CRT, Neon… | 🎮 **Control detection** | shows which keys a game uses |
| | | ⏪ **Rewind** | hold `Backspace` |
| | | 🕹️ **SUPER-CHIP** | 128×64 hi-res games |
| | | 🌐 **Browser version** | same C++ in WebAssembly |

| Time-travel debugging | Memory heat-map |
|---|---|
| <img src="screenshots/16_time_travel_debugging.png" alt="Stepping forward, then back two instructions"> | <img src="screenshots/15_memory_heatmap.png" alt="4 KB of memory lit up as it runs"> |
| **Savestates** | **Palettes** |
| <img src="screenshots/04_savestate.png" alt="Save, play on, load"> | <img src="screenshots/05_color_palettes.png" alt="All eight palettes"> |
| **SUPER-CHIP games** | **Automated tests** |
| <img src="screenshots/09_superchip_games.png" alt="Four SUPER-CHIP games"> | <img src="screenshots/02_test_suite_passing.png" alt="make test: 13 of 13 passing"> |

📸 More: **[all screenshots](screenshots/README.md)**

---

## 🎮 Controls

```
 CHIP-8 keypad     Keyboard           Games
 ┌─┬─┬─┬─┐        ┌─┬─┬─┬─┐          Tetris   Q rotate · W left · E right · A drop
 │1│2│3│C│        │1│2│3│4│          Pong     1/Q left paddle · 4/R right paddle
 │4│5│6│D│   =    │Q│W│E│R│          Eaty     E start · W A S D move
 │7│8│9│E│        │A│S│D│F│
 │A│0│B│F│        │Z│X│C│V│          H in the app shows every key
 └─┴─┴─┴─┘        └─┴─┴─┴─┘
```

| `P` pause | `N` step frame | `F1` debugger | `F6` / `F4` step / back | `F7` breakpoint |
|---|---|---|---|---|
| **`F5` / `F9`** save / load | **`Tab`** palette | **`Backspace`** rewind | **`B`** sound test | **`U`** mute |

---

## 📚 More

- 🔬 **[TECHNICAL.md](TECHNICAL.md)**: every bug with line numbers, design decisions, file-by-file tour
- 📸 **[screenshots/](screenshots/README.md)**: proof images for every feature
- 🤖 **[AI_USAGE.md](AI_USAGE.md)**: how AI tools were used (TatHack Rule 06)
- 📜 Licence: MIT ([LICENSE](LICENSE)) · test ROMs by Timendus (GPL-3.0) · SUPER-CHIP games from the chip8Archive (CC0)

<div align="center"><sub>Built by <b>Kevin Mathew</b> · build <code>KM-CHIP8-9532C2F87939</code></sub></div>
