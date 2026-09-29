# Test ROMs

These ROMs are from Timendus' [CHIP-8 test suite](https://github.com/Timendus/chip8-test-suite)
and are licensed under the GNU GPL v3.0 by their author. They are unmodified and are used only
as test data by `make test`; they are not part of the emulator source.

| File | What it checks |
|---|---|
| `2-ibm-logo.ch8` | Basic drawing (`00E0`, `6XNN`, `ANNN`, `7XNN`, `DXYN`, `1NNN`) |
| `3-corax+.ch8` | Every arithmetic, logic, skip, memory and subroutine opcode |
| `4-flags.ch8` | VF carry/borrow/shift flags, including when VF is an operand |
| `5-quirks.ch8` | Quirk behaviour for CHIP-8, SUPER-CHIP and XO-CHIP |
| `8-scrolling.ch8` | SUPER-CHIP / XO-CHIP scrolling opcodes in low and high resolution |

The expected screens in `../expected/` were checked by eye against the test suite's documentation
(every row shows a checkmark) before being recorded.
