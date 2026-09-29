#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <random>
#include <string>

const int DISPLAY_W = 128;
const int DISPLAY_H = 64;
const uint16_t SMALL_FONT_ADDR = 0x000;
const uint16_t BIG_FONT_ADDR = 0x050;
const uint16_t AUTHOR_TAG_ADDR = 0x160;

extern const char* const BUILD_AUTHOR;
extern const char* const BUILD_SIGNATURE;

struct Quirks{
    bool vf_reset;
    bool memory_increment;
    bool shift_uses_vy;
    bool jump_uses_vx;
    bool clipping;
    bool display_wait;
};

enum class QuirkProfile{ CHIP8 = 0, COSMAC, SCHIP, XOCHIP, COUNT };
Quirks quirks_for(QuirkProfile profile);
const char* quirk_profile_name(QuirkProfile profile);

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
    uint8_t rpl[16];
    uint8_t display[DISPLAY_W*DISPLAY_H];
};

class Chip8{
    public:
        Chip8();
        bool load_rom(const std::string& filename);
        void reset();
        uint32_t get_rom_crc() const {return rom_crc;}
        void emulate_cycle();
        void update_timers();
        bool draw_flag;
        uint8_t display[DISPLAY_W*DISPLAY_H];
        uint8_t key[16];
        uint8_t get_sound_timer() const {return sound_timer;}
        void set_sound_timer(uint8_t value){sound_timer = value;}
        bool vblank_wait;
        void poke(uint16_t addr, uint8_t value){memory[addr & 0xFFF] = value;}

        uint8_t peek(uint16_t addr) const {return memory[addr & 0xFFF];}
        uint16_t peek_opcode(uint16_t addr) const {return memory[addr & 0xFFF] << 8 | memory[(addr+1) & 0xFFF];}
        uint8_t get_v(int i) const {return v[i & 0xF];}
        uint16_t get_index() const {return index;}
        uint16_t get_pc() const {return pc;}
        uint8_t get_sp() const {return sp;}
        uint16_t get_stack(int i) const {return stack[i & 0xF];}
        uint8_t get_delay_timer() const {return delay_timer;}
        bool is_hires() const {return hires;}
        bool is_halted() const {return halted;}

        uint16_t get_key_polls() const {return key_polls;}
        bool get_waits_for_any_key() const {return waits_for_any_key;}

        uint8_t heat_exec[4096] = {}, heat_read[4096] = {}, heat_write[4096] = {};
        void fade_heat(uint8_t amount);

        void seed_random(uint32_t seed){rng.seed(seed);}
        bool pixel(int x, int y) const {int s = hires ? 1 : 2; return display[(x*s) + (y*s)*DISPLAY_W] != 0;}

        Chip8State save_state() const;
        void load_state(const Chip8State& state);
        bool save_state_to_file(const std::string& path) const;
        bool load_state_from_file(const std::string& path);

        void set_quirk_profile(QuirkProfile profile);
        QuirkProfile get_quirk_profile() const {return quirk_profile;}
    private:
        uint8_t memory[4096];
        uint8_t v[16];
        uint16_t index;
        uint16_t pc;
        uint16_t stack[16];
        uint8_t sp;
        uint8_t delay_timer;
        uint8_t sound_timer;
        bool hires;
        bool halted;
        uint8_t rpl[16];
        uint16_t opcode;
        int waiting_key = -1;
        std::string rom_path;
        std::mt19937 rng{std::random_device{}()};
        uint16_t key_polls = 0;
        bool waits_for_any_key = false;
        uint32_t rom_crc = 0;
        QuirkProfile quirk_profile;
        Quirks quirks;
        void initialise();
        void load_fonts();
        void draw_sprite(int x, int y, int n);
        void scroll(int dx, int dy);
};

#endif
