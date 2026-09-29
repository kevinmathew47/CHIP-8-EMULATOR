#include "disasm.h"
#include <cstdio>

static std::string hex(unsigned value, int digits){
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%0*X", digits, value);
    return buf;
}

std::string disassemble(uint16_t opcode){
    const std::string x = "V" + hex((opcode & 0x0F00) >> 8, 1);
    const std::string y = "V" + hex((opcode & 0x00F0) >> 4, 1);
    const std::string nnn = "0x" + hex(opcode & 0x0FFF, 3);
    const std::string nn = "0x" + hex(opcode & 0x00FF, 2);
    const unsigned n = opcode & 0x000F;

    switch(opcode & 0xF000){
        case 0x0000:
            if(opcode == 0x00E0) return "CLS";
            if(opcode == 0x00EE) return "RET";
            if((opcode & 0xFFF0) == 0x00C0) return "SCD " + std::to_string(n); // SUPER-CHIP from here
            if((opcode & 0xFFF0) == 0x00D0) return "SCU " + std::to_string(n); // XO-CHIP
            if(opcode == 0x00FB) return "SCR";
            if(opcode == 0x00FC) return "SCL";
            if(opcode == 0x00FD) return "EXIT";
            if(opcode == 0x00FE) return "LOW";
            if(opcode == 0x00FF) return "HIGH";
            return "SYS " + nnn;
        case 0x1000: return "JP " + nnn;
        case 0x2000: return "CALL " + nnn;
        case 0x3000: return "SE " + x + ", " + nn;
        case 0x4000: return "SNE " + x + ", " + nn;
        case 0x5000: if(n == 0) return "SE " + x + ", " + y; break;
        case 0x6000: return "LD " + x + ", " + nn;
        case 0x7000: return "ADD " + x + ", " + nn;
        case 0x8000:
            switch(n){
                case 0x0: return "LD " + x + ", " + y;
                case 0x1: return "OR " + x + ", " + y;
                case 0x2: return "AND " + x + ", " + y;
                case 0x3: return "XOR " + x + ", " + y;
                case 0x4: return "ADD " + x + ", " + y;
                case 0x5: return "SUB " + x + ", " + y;
                case 0x6: return "SHR " + x + ", " + y;
                case 0x7: return "SUBN " + x + ", " + y;
                case 0xE: return "SHL " + x + ", " + y;
            }
            break;
        case 0x9000: if(n == 0) return "SNE " + x + ", " + y; break;
        case 0xA000: return "LD I, " + nnn;
        case 0xB000: return "JP V0, " + nnn;
        case 0xC000: return "RND " + x + ", " + nn;
        case 0xD000: return "DRW " + x + ", " + y + ", " + std::to_string(n);
        case 0xE000:
            if((opcode & 0xFF) == 0x9E) return "SKP " + x;
            if((opcode & 0xFF) == 0xA1) return "SKNP " + x;
            break;
        case 0xF000:
            switch(opcode & 0xFF){
                case 0x07: return "LD " + x + ", DT";
                case 0x0A: return "LD " + x + ", K";
                case 0x15: return "LD DT, " + x;
                case 0x18: return "LD ST, " + x;
                case 0x1E: return "ADD I, " + x;
                case 0x29: return "LD F, " + x;
                case 0x30: return "LD HF, " + x;
                case 0x75: return "LD R, " + x;
                case 0x85: return "LD " + x + ", R";
                case 0x33: return "LD B, " + x;
                case 0x55: return "LD [I], " + x;
                case 0x65: return "LD " + x + ", [I]";
            }
            break;
    }
    return "DW 0x" + hex(opcode, 4); // Not an instruction: probably sprite or data bytes
}
