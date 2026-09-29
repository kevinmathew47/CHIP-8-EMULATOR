// FX0A must wait for a key to be pressed AND released (original VIP behaviour).
#include "../src/chip8.h"
#include <iostream>

int main(){
    Chip8 chip8;
    chip8.poke(0x200, 0xF3); chip8.poke(0x201, 0x0A); // FX0A: wait for key -> V3
    chip8.poke(0x202, 0x12); chip8.poke(0x203, 0x02); // 1202: loop forever

    for(int i=0; i<5; i++) chip8.emulate_cycle();
    if(chip8.get_pc() != 0x200){ std::cerr << "FAIL: FX0A continued with no key pressed\n"; return 1; }

    chip8.key[5] = 1;
    for(int i=0; i<5; i++) chip8.emulate_cycle();
    if(chip8.get_pc() != 0x200){ std::cerr << "FAIL: FX0A continued while key still held\n"; return 1; }

    chip8.key[5] = 0;
    chip8.emulate_cycle();
    if(chip8.get_pc() != 0x202 || chip8.get_v(3) != 5){ std::cerr << "FAIL: FX0A did not accept key 5 on release\n"; return 1; }

    std::cout << "PASS: FX0A waits for key release" << std::endl;
    return 0;
}
