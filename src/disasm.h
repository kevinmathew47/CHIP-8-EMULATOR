#ifndef DISASM_H
#define DISASM_H

#include <cstdint>
#include <string>

// Turns one 16-bit CHIP-8 opcode into Cowgod-style assembly, e.g. 0xD125 -> "DRW V1, V2, 5".
std::string disassemble(uint16_t opcode);

#endif
