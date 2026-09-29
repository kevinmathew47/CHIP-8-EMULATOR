// Checks that a savestate written to disk restores the machine exactly:
// run -> save -> run further -> load -> run the same amount again -> displays must match.
#include "../src/chip8.h"
#include <cstring>
#include <iostream>

static void run_frames(Chip8& c, int frames){
    for(int f=0; f<frames; f++){
        for(int i=0; i<10 && !c.vblank_wait; i++) c.emulate_cycle();
        c.update_timers();
    }
}

int main(int argc, char** argv){
    if(argc < 2){ std::cerr << "usage: savestate_test <rom>\n"; return 1; }
    const char* path = "savestate_test.c8s";
    Chip8 chip8;
    if(!chip8.load_rom(argv[1])) return 1;

    run_frames(chip8, 60);
    if(!chip8.save_state_to_file(path)){ std::cerr << "FAIL: could not write savestate\n"; return 1; }
    Chip8State saved = chip8.save_state();

    run_frames(chip8, 30);
    uint8_t expected[DISPLAY_W*DISPLAY_H];
    std::memcpy(expected, chip8.display, sizeof(expected));

    run_frames(chip8, 90); // Drift away from the saved point
    if(!chip8.load_state_from_file(path)){ std::cerr << "FAIL: could not read savestate\n"; return 1; }
    Chip8State restored = chip8.save_state();
    if(std::memcmp(&saved, &restored, sizeof(saved)) != 0){ std::cerr << "FAIL: restored state differs\n"; return 1; }

    run_frames(chip8, 30);
    if(std::memcmp(expected, chip8.display, sizeof(expected)) != 0){ std::cerr << "FAIL: replay diverged\n"; return 1; }

    // A corrupt file must be rejected without touching the machine
    { FILE* f = std::fopen(path, "wb"); std::fputs("junk", f); std::fclose(f); }
    if(chip8.load_state_from_file(path)){ std::cerr << "FAIL: accepted a corrupt savestate\n"; return 1; }
    std::remove(path);

    std::cout << "PASS: savestate round-trip" << std::endl;
    return 0;
}
