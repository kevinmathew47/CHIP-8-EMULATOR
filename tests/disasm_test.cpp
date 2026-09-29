#include "../src/disasm.h"
#include <iostream>

int main(){
    struct Case{ uint16_t opcode; const char* expected; };
    const Case cases[] = {
        {0x00E0, "CLS"},            {0x00EE, "RET"},             {0x0123, "SYS 0x123"},
        {0x1ABC, "JP 0xABC"},       {0x2ABC, "CALL 0xABC"},      {0x3A42, "SE VA, 0x42"},
        {0x4A42, "SNE VA, 0x42"},   {0x5AB0, "SE VA, VB"},       {0x6A42, "LD VA, 0x42"},
        {0x7A42, "ADD VA, 0x42"},   {0x8AB0, "LD VA, VB"},       {0x8AB1, "OR VA, VB"},
        {0x8AB2, "AND VA, VB"},     {0x8AB3, "XOR VA, VB"},      {0x8AB4, "ADD VA, VB"},
        {0x8AB5, "SUB VA, VB"},     {0x8AB6, "SHR VA, VB"},      {0x8AB7, "SUBN VA, VB"},
        {0x8ABE, "SHL VA, VB"},     {0x9AB0, "SNE VA, VB"},      {0xAABC, "LD I, 0xABC"},
        {0xBABC, "JP V0, 0xABC"},   {0xCA42, "RND VA, 0x42"},    {0xD125, "DRW V1, V2, 5"},
        {0xEA9E, "SKP VA"},         {0xEAA1, "SKNP VA"},         {0xFA07, "LD VA, DT"},
        {0xFA0A, "LD VA, K"},       {0xFA15, "LD DT, VA"},       {0xFA18, "LD ST, VA"},
        {0xFA1E, "ADD I, VA"},      {0xFA29, "LD F, VA"},        {0xFA33, "LD B, VA"},
        {0xFA55, "LD [I], VA"},     {0xFA65, "LD VA, [I]"},
        {0x00C5, "SCD 5"},          {0x00FB, "SCR"},             {0x00FC, "SCL"},
        {0x00FD, "EXIT"},           {0x00FE, "LOW"},             {0x00FF, "HIGH"},
        {0xFA30, "LD HF, VA"},      {0xFA75, "LD R, VA"},        {0xFA85, "LD VA, R"},
        {0xD120, "DRW V1, V2, 0"},
        {0x5AB1, "DW 0x5AB1"},      {0x8AB8, "DW 0x8AB8"},       {0xFAFF, "DW 0xFAFF"},
    };
    int failures = 0;
    for(const Case& c : cases){
        std::string got = disassemble(c.opcode);
        if(got != c.expected){
            std::cerr << "FAIL: " << std::hex << c.opcode << " -> \"" << got << "\", expected \"" << c.expected << "\"\n";
            failures++;
        }
    }
    if(failures) return 1;
    std::cout << "PASS: disassembler (" << sizeof(cases)/sizeof(cases[0]) << " opcodes)" << std::endl;
    return 0;
}
