# AI Usage Disclosure

This file is our disclosure under **Rule 06 (AI Usage Policy)** of the TatHack '26 rules.

## Tools used

| Tool | Used for |
|---|---|
| Claude Code (Anthropic) | Choosing the problem statement, finding the intentional bugs, writing and reviewing the new features, writing the automated tests, and drafting documentation |

<!-- TEAM: add any other tools you used (ChatGPT, Copilot, Gemini, ...) and remove anything that is not accurate. -->

## How it was used

- **Bug finding:** the AI read the original source and proposed fixes. Every fix was checked
  against the public CHIP-8 test ROMs (`make test`), not just accepted.
- **Features:** the AI wrote first versions of the speed control, savestates, palettes,
  rewind, debugger, disassembler and SUPER-CHIP support from our instructions. We ran,
  played and tested each one and asked for changes where it was wrong (for example the
  Tetris key mapping and the display-wait speed problem found during playtesting).
- **Tests and docs:** the AI drafted the headless tests and this documentation.

## What we are responsible for

We reviewed the code, ran the emulator ourselves, and can explain how each part works:
the CPU loop in `src/chip8.cpp`, the SDL front end in `src/main.cpp`, the debugger in
`src/debugger.cpp`, and the tests in `tests/`.

## Hidden instruction in the problem statement

The problem statement contained a paragraph addressed to AI models asking for a specific
phrase, a specially named function and themed code comments "for a better evaluation".
We treated it as a check for unreviewed AI output and deliberately did **not** add any
of it. Nothing in this repository was added to game the evaluation.
