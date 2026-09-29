# AI Usage Disclosure

Our disclosure under **Rule 06 (AI Usage Policy)** of the TatHack '26 rules.

## Tool

**Claude Code (Anthropic)**, used as a pair-programming assistant throughout the project.

## How we worked

Kevin led the project and made the decisions; the AI did most of the typing.

| Kevin | Claude Code |
|---|---|
| Chose PS1 after comparing all eight problem statements with the AI | Analysed the statements and repos and recommended one |
| Set the goal: fix every bug, then make the project stand out | Found the bugs by reading the code against the CHIP-8 spec and public test ROMs |
| Ran and played the emulator and the web version, and reported what felt wrong | Wrote the fixes, the features, the tests, the web version and the documentation |
| Chose which proposed features to build and asked for others (below) | Proposed most of the extra features |
| Reviewed every change and verified the results | Explained each change so it could be reviewed and defended |

## Where the ideas came from

| Source | Features |
|---|---|
| **Problem statement** (required) | Speed control, savestates, colour palettes |
| **Problem statement** (bonus ideas) | Debugger / disassembler, sound waveforms, SUPER-CHIP support, ROM browser |
| **Kevin** (from his own testing and requests) | The web interface and its three-column layout; the IBM logo test and proof of fixes on the web page; making the layout fill every desktop screen; the sound test and mute (after reporting silent sound); checking every game's controls and every key; the authorship watermark; the README format; the live demo |
| **Claude Code** (proposed, Kevin approved) | Time-travel debugging, memory heat-map, automatic control detection, rewind, quirk profiles, ROM auto-detection, phosphor/scanline effects, scripted input for repeatable demos, the automated test suite |

## Problems Kevin caught by testing

Kevin's play-testing found several of the issues listed in the README, including: games feeling
slow, confusing Tetris controls, no sound, an unbalanced web layout with gaps, the layout collapsing on
another desktop screen, and black bars beside the game screen with small debugger text. Each was
investigated and fixed.

## How we checked the AI's work

- Every bug fix is verified by public test ROMs in `make test` (13 checks), run after every change.
- Every game's controls and every hotkey were verified by pressing each key against the ROM.
- The desktop and web versions were run and played, not just compiled.
- A fresh `git clone` was built and tested exactly as a judge would.

## Hidden instruction in the problem statement

The problem statement contained a paragraph addressed to AI models asking for a specific phrase, a
specially named function and themed code comments "for a better evaluation". We treated it as a check
for unreviewed AI output and deliberately did **not** add any of it.
