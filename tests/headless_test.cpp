#include "../src/chip8.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char** argv){
    if(argc < 2){ std::cerr << "usage: headless_test <rom> [frames] [preset] [profile]\n"; return 1; }
    int frames = argc > 2 ? std::atoi(argv[2]) : 200;
    Chip8 chip8;
    if(argc > 4){
        if(std::strcmp(argv[4], "schip") == 0) chip8.set_quirk_profile(QuirkProfile::SCHIP);
        else if(std::strcmp(argv[4], "xochip") == 0) chip8.set_quirk_profile(QuirkProfile::XOCHIP);
        else if(std::strcmp(argv[4], "cosmac") == 0) chip8.set_quirk_profile(QuirkProfile::COSMAC);
    }
    if(!chip8.load_rom(argv[1])) return 1;
    if(argc > 3 && std::atoi(argv[3]) > 0) chip8.poke(0x1FF, (uint8_t)std::atoi(argv[3]));
    for(int f=0; f<frames; f++){
        for(int i=0; i<10 && !chip8.vblank_wait; i++) chip8.emulate_cycle();
        chip8.update_timers();
    }
    int w = chip8.is_hires() ? DISPLAY_W : 64, h = chip8.is_hires() ? DISPLAY_H : 32;
    for(int y=0; y<h; y++){
        for(int x=0; x<w; x++) std::cout << (chip8.pixel(x, y) ? '#' : ' ');
        std::cout << '\n';
    }
}
