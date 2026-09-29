# SUPER-CHIP games

These ROMs come from John Earnest's [chip8Archive](https://github.com/JohnEarnest/chip8Archive),
which is released under CC0 (public domain). They are included to show off the emulator's
SUPER-CHIP support: 128x64 high-resolution mode, 16x16 sprites and scrolling.

| File | Title | Author |
|---|---|---|
| `eaty.ch8` | Eaty The Alien | John Earnest |
| `octogon.ch8` | Super Octogon | John Earnest |
| `blackrainbow.ch8` | Black Rainbow | John Earnest |
| `rockto.ch8` | Rockto | SupSuper |

The emulator recognises these files and applies the speed and quirk settings the authors published
in the archive's `programs.json`, and shows each game's controls when it starts:

| Game | Controls | Speed | Quirks |
|---|---|---|---|
| Eaty The Alien | `E` start / action, `W A S D` move | 200 per frame | Octo defaults (our XO-CHIP profile) |
| Super Octogon | `A` / `D` turn | 200 per frame | SUPER-CHIP |
| Black Rainbow | `D` start, `W A S D` move, `X` action | 20 per frame | Octo defaults (XO-CHIP) |
| Rockto | any key to start, `A S D` move, `1` redraws the level | 15 per frame | Octo defaults (XO-CHIP) |

For other SUPER-CHIP ROMs, pass `--quirks schip --speed 30` or press `F2`.
