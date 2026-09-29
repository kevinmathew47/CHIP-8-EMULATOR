#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <random>
#include <string>

// The display buffer is always SUPER-CHIP sized. In low-res (64x32) mode each
// CHIP-8 pixel covers a 2x2 block, so the renderer never needs to know the mode.
const int DISPLAY_W = 128;
const int DISPLAY_H = 64;
const uint16_t SMALL_FONT_ADDR = 0x000; // 16 digits x 5 bytes
const uint16_t BIG_FONT_ADDR = 0x050;   // 16 digits x 10 bytes (SUPER-CHIP)
const uint16_t AUTHOR_TAG_ADDR = 0x160; // Authorship tag in the unused interpreter area (below 0x200)

// Authorship: shown in the interface and stamped into emulated memory, so every
// savestate and memory dump made with this emulator carries it.
extern const char* const BUILD_AUTHOR;
extern const char* const BUILD_SIGNATURE;

// Behaviour differences between CHIP-8 interpreters. ROMs written for one
// platform can misbehave on another, so these are switchable at runtime.
struct Quirks{
    bool vf_reset;          // 8XY1/2/3 clear VF (original COSMAC VIP)
    bool memory_increment;  // FX55/FX65 leave I pointing past the last register
    bool shift_uses_vy;     // 8XY6/8XYE shift VY into VX instead of shifting VX
    bool jump_uses_vx;      // BXNN jumps to XNN + VX instead of NNN + V0
    bool clipping;          // Sprites are clipped at the screen edge instead of wrapping
    bool display_wait;      // DXYN waits for the next 60Hz frame before continuing
};

// CHIP8 is the playable default; COSMAC adds the original VIP's display wait,
// which is historically exact but caps games at one sprite draw per frame.
enum class QuirkProfile{ CHIP8 = 0, COSMAC, SCHIP, XOCHIP, COUNT };
Quirks quirks_for(QuirkProfile profile);
const char* quirk_profile_name(QuirkProfile profile);

// Everything needed to resume emulation exactly where it left off.
// Plain data, so it can be copied into the rewind buffer or written to disk as-is.
struct Chip8State{
    uint8_t memory[4096];
    uint8_t v[16];
    uint16_t index;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t hires;
    uint8_t halted;
    uint8_t rpl[16]; // SUPER-CHIP "RPL user flags" (FX75/FX85)
    uint8_t display[DISPLAY_W*DISPLAY_H];
};

class Chip8{
    public:
        Chip8();
        bool load_rom(const std::string& filename); // To load a game file
        void reset(); // Restart the loaded ROM from scratch
        uint32_t get_rom_crc() const {return rom_crc;} // CRC32 of the ROM bytes, used to look up known ROMs
        void emulate_cycle(); // To execute one instruction
        void update_timers(); // Decrement delay/sound timers, call at 60Hz
        bool draw_flag; // When we need to redraw the screen;
        uint8_t display[DISPLAY_W*DISPLAY_H];
        uint8_t key[16]; // Keyboard of 16 keys
        uint8_t get_sound_timer() const {return sound_timer;} // For getting the value of sound timer
        void set_sound_timer(uint8_t value){sound_timer = value;} // Used by the sound-test key
        bool vblank_wait; // Set by DXYN when the display_wait quirk is on; cleared by update_timers()
        void poke(uint16_t addr, uint8_t value){memory[addr & 0xFFF] = value;} // Write a byte (used by tests/tools)

        // Read-only views for the debugger
        uint8_t peek(uint16_t addr) const {return memory[addr & 0xFFF];}
        uint16_t peek_opcode(uint16_t addr) const {return memory[addr & 0xFFF] << 8 | memory[(addr+1) & 0xFFF];}
        uint8_t get_v(int i) const {return v[i & 0xF];}
        uint16_t get_index() const {return index;}
        uint16_t get_pc() const {return pc;}
        uint8_t get_sp() const {return sp;}
        uint16_t get_stack(int i) const {return stack[i & 0xF];}
        uint8_t get_delay_timer() const {return delay_timer;}
        bool is_hires() const {return hires;}
        bool is_halted() const {return halted;} // Set by 00FD (SUPER-CHIP exit)

        // Control detection: which keys has the ROM actually checked (EX9E/EXA1) since it was loaded?
        uint16_t get_key_polls() const {return key_polls;}      // Bit N set = key N was checked
        bool get_waits_for_any_key() const {return waits_for_any_key;} // FX0A seen ("press any key")

        // Memory heat-map for the debugger: set to 255 when an address is executed/read/written,
        // faded by the front end each frame with fade_heat()
        uint8_t heat_exec[4096] = {}, heat_read[4096] = {}, heat_write[4096] = {};
        void fade_heat(uint8_t amount);

        // Fixed seed for CXNN, so tests and scripted demos are repeatable
        void seed_random(uint32_t seed){rng.seed(seed);}
        // Pixel in CHIP-8 coordinates of the current resolution (64x32 or 128x64)
        bool pixel(int x, int y) const {int s = hires ? 1 : 2; return display[(x*s) + (y*s)*DISPLAY_W] != 0;}

        // Savestates
        Chip8State save_state() const;
        void load_state(const Chip8State& state);
        bool save_state_to_file(const std::string& path) const;
        bool load_state_from_file(const std::string& path);

        // Quirks
        void set_quirk_profile(QuirkProfile profile);
        QuirkProfile get_quirk_profile() const {return quirk_profile;}
    private:
        uint8_t memory[4096]; // Memory of 4KB
        uint8_t v[16]; // 16 registers, V0 to VF
        uint16_t index; // Index register for memory addresses
        uint16_t pc; // Program counter
        uint16_t stack[16];
        uint8_t sp; // Stack pointer
        uint8_t delay_timer; // Counts down at 60Hz
        uint8_t sound_timer; // Beeps when greater than 0, counts down at 60Hz
        bool hires; // SUPER-CHIP 128x64 mode
        bool halted;
        uint8_t rpl[16];
        uint16_t opcode; // Current instruction
        int waiting_key = -1; // Key FX0A saw go down and is waiting to see released
        std::string rom_path; // Kept so reset() can reload the ROM
        std::mt19937 rng{std::random_device{}()};
        uint16_t key_polls = 0;
        bool waits_for_any_key = false;
        uint32_t rom_crc = 0;
        QuirkProfile quirk_profile;
        Quirks quirks;
        void initialise(); // Initialises everything
        void load_fonts(); // Loads font (0-9, A-F)
        void draw_sprite(int x, int y, int n);
        void scroll(int dx, int dy);
};

#endif
