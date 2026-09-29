#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>

uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

uint8_t chip8_bigfont[160] = {
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C,
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF,
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C,
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C,
    0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C,
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60,
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C,
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C,
    0x18, 0x3C, 0x66, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3,
    0xFC, 0xFE, 0xC3, 0xC3, 0xFE, 0xFE, 0xC3, 0xC3, 0xFE, 0xFC,
    0x3C, 0x7E, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0x7E, 0x3C,
    0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC,
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFC, 0xC0, 0xC0, 0xFF, 0xFF,
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFC, 0xC0, 0xC0, 0xC0, 0xC0
};

const char* const BUILD_AUTHOR = "KEVIN MATHEW";
const char* const BUILD_SIGNATURE = "KM-CHIP8-9532C2F87939";

Quirks quirks_for(QuirkProfile profile){
    switch(profile){
        case QuirkProfile::SCHIP:  return {false,  false,  false,   true,   true,    false};
        case QuirkProfile::XOCHIP: return {false,  true,   true,    false,  false,   false};
        case QuirkProfile::COSMAC: return {true,   true,   true,    false,  true,    true};
        case QuirkProfile::CHIP8:
        default:                   return {true,   true,   true,    false,  true,    false};
    }
}

const char* quirk_profile_name(QuirkProfile profile){
    switch(profile){
        case QuirkProfile::COSMAC: return "COSMAC VIP";
        case QuirkProfile::SCHIP:  return "SCHIP";
        case QuirkProfile::XOCHIP: return "XO-CHIP";
        default:                   return "CHIP-8";
    }
}

static uint32_t crc32(const uint8_t* data, size_t length){
    uint32_t crc = 0xFFFFFFFF;
    for(size_t i=0; i<length; i++){
        crc ^= data[i];
        for(int bit=0; bit<8; bit++) crc = (crc >> 1) ^ (0xEDB88320 & (0u - (crc & 1)));
    }
    return ~crc;
}

Chip8::Chip8(){
    set_quirk_profile(QuirkProfile::CHIP8);
    initialise();
}

void Chip8::set_quirk_profile(QuirkProfile profile){
    quirk_profile = profile;
    quirks = quirks_for(profile);
}

void Chip8::initialise(){
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;

    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));
    memset(v, 0, sizeof(v));
    memset(memory, 0, sizeof(memory));
    memset(key, 0, sizeof(key));

    load_fonts();
    delay_timer = 0;
    sound_timer = 0;
    memset(rpl, 0, sizeof(rpl));
    hires = false;
    halted = false;
    draw_flag = false;
    vblank_wait = false;
    waiting_key = -1;
    key_polls = 0;
    waits_for_any_key = false;
}

void Chip8::load_fonts(){
    for(int i=0; i<80; i++) memory[SMALL_FONT_ADDR + i] = chip8_fontset[i];
    for(int i=0; i<160; i++) memory[BIG_FONT_ADDR + i] = chip8_bigfont[i];
    std::string tag = std::string("BUILT BY ") + BUILD_AUTHOR + " " + BUILD_SIGNATURE;
    for(size_t i=0; i<tag.size() && AUTHOR_TAG_ADDR + i < 0x1F0; i++) memory[AUTHOR_TAG_ADDR + i] = (uint8_t)tag[i];
}

bool Chip8::load_rom(const std::string& filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file.is_open()){
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if(size > (4096-512)){
        std::cerr << "ROM too large to fit in memory" << std::endl;
        return false;
    }

    file.read((char*)(memory+512),size);
    file.close();

    rom_path = filename;
    rom_crc = crc32(memory+512, (size_t)size);
    std::cout << "Loaded ROM: " << filename << std::endl;
    return true;
}

void Chip8::reset(){
    initialise();
    if(!rom_path.empty()) load_rom(rom_path);
}

Chip8State Chip8::save_state() const{
    Chip8State s;
    std::memset(&s, 0, sizeof(s));
    std::memcpy(s.memory, memory, sizeof(memory));
    std::memcpy(s.v, v, sizeof(v));
    s.index = index;
    s.pc = pc;
    std::memcpy(s.stack, stack, sizeof(stack));
    s.sp = sp;
    s.delay_timer = delay_timer;
    s.sound_timer = sound_timer;
    s.hires = hires;
    s.halted = halted;
    std::memcpy(s.rpl, rpl, sizeof(rpl));
    std::memcpy(s.display, display, sizeof(display));
    return s;
}

void Chip8::load_state(const Chip8State& s){
    std::memcpy(memory, s.memory, sizeof(memory));
    std::memcpy(v, s.v, sizeof(v));
    index = s.index;
    pc = s.pc & 0xFFF;
    std::memcpy(stack, s.stack, sizeof(stack));
    sp = s.sp & 0xF;
    delay_timer = s.delay_timer;
    sound_timer = s.sound_timer;
    hires = s.hires != 0;
    halted = s.halted != 0;
    std::memcpy(rpl, s.rpl, sizeof(rpl));
    std::memcpy(display, s.display, sizeof(display));
    vblank_wait = false;
    waiting_key = -1;
    draw_flag = true;
}

static const char SAVESTATE_MAGIC[4] = {'C', '8', 'S', 'T'};
static const uint8_t SAVESTATE_VERSION = 2;

bool Chip8::save_state_to_file(const std::string& path) const{
    std::ofstream file(path, std::ios::binary);
    if(!file) return false;
    Chip8State s = save_state();
    file.write(SAVESTATE_MAGIC, 4);
    file.write((const char*)&SAVESTATE_VERSION, 1);
    file.write((const char*)&s, sizeof(s));
    return (bool)file;
}

bool Chip8::load_state_from_file(const std::string& path){
    std::ifstream file(path, std::ios::binary);
    if(!file) return false;
    char magic[4];
    uint8_t version = 0;
    Chip8State s;
    file.read(magic, 4);
    file.read((char*)&version, 1);
    file.read((char*)&s, sizeof(s));
    if(!file || std::memcmp(magic, SAVESTATE_MAGIC, 4) != 0 || version != SAVESTATE_VERSION) return false;
    load_state(s);
    return true;
}

void Chip8::draw_sprite(int vx, int vy, int n){
    const int scale = hires ? 1 : 2;
    const int width = DISPLAY_W/scale, height = DISPLAY_H/scale;
    const bool big = (n == 0) && (quirk_profile == QuirkProfile::SCHIP || quirk_profile == QuirkProfile::XOCHIP);
    const int sprite_w = big ? 16 : 8, rows = big ? 16 : n;
    const int x0 = vx % width, y0 = vy % height;

    v[0xF] = 0;
    for(int row=0; row<rows; row++){
        uint16_t bits = big ? (memory[(index + row*2) & 0xFFF] << 8 | memory[(index + row*2 + 1) & 0xFFF])
                            : (memory[(index + row) & 0xFFF] << 8);
        heat_read[(index + (big ? row*2 : row)) & 0xFFF] = 255;
        if(big) heat_read[(index + row*2 + 1) & 0xFFF] = 255;
        for(int col=0; col<sprite_w; col++){
            if(!(bits & (0x8000 >> col))) continue;
            int px = x0 + col, py = y0 + row;
            if(quirks.clipping){
                if(px >= width || py >= height) continue;
            }
            else{
                px %= width;
                py %= height;
            }
            uint8_t* cell = &display[(px*scale) + (py*scale)*DISPLAY_W];
            if(*cell) v[0xF] = 1;
            for(int dy=0; dy<scale; dy++)
                for(int dx=0; dx<scale; dx++) cell[dx + dy*DISPLAY_W] ^= 1;
        }
    }
    draw_flag = true;
}

void Chip8::scroll(int dx, int dy){
    uint8_t shifted[DISPLAY_W*DISPLAY_H] = {};
    for(int y=0; y<DISPLAY_H; y++){
        for(int x=0; x<DISPLAY_W; x++){
            int sx = x - dx, sy = y - dy;
            if(sx >= 0 && sx < DISPLAY_W && sy >= 0 && sy < DISPLAY_H) shifted[x + y*DISPLAY_W] = display[sx + sy*DISPLAY_W];
        }
    }
    std::memcpy(display, shifted, sizeof(display));
    draw_flag = true;
}

void Chip8::emulate_cycle(){
    if(halted) return;
    opcode = memory[pc & 0xFFF] << 8 | memory[(pc+1) & 0xFFF];
    heat_exec[pc & 0xFFF] = heat_exec[(pc+1) & 0xFFF] = 255;

    switch(opcode & 0xF000){
        case 0x0000:{
            const int scale = hires ? 1 : 2;
            if((opcode & 0xFFF0) == 0x00C0){
                scroll(0, (opcode & 0xF)*scale);
                pc += 2;
                break;
            }
            if((opcode & 0xFFF0) == 0x00D0){
                scroll(0, -(opcode & 0xF)*scale);
                pc += 2;
                break;
            }
            switch(opcode & 0x0FFF){
                case 0x00E0:
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE:
                    sp = (sp - 1) & 0xF;
                    pc = stack[sp];
                    pc += 2;
                    break;
                case 0x00FB:
                    scroll(4*scale, 0);
                    pc += 2;
                    break;
                case 0x00FC:
                    scroll(-4*scale, 0);
                    pc += 2;
                    break;
                case 0x00FD:
                    halted = true;
                    break;
                case 0x00FE:
                case 0x00FF:
                    hires = (opcode & 0xFF) == 0xFF;
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
            }
            break;
        }
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            stack[sp] = pc;
            sp = (sp + 1) & 0xF;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            if(v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;
        case 0x4000:
            if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;
        case 0x5000:
            if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4]) pc += 4;
            else pc += 2;
            break;
        case 0x6000:
            v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000:
            v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            switch(opcode & 0x000F){
                case 0x0000:
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0001:
                    v[(opcode & 0x0F00) >> 8] |= v[(opcode & 0x00F0) >> 4];
                    if(quirks.vf_reset) v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0002:
                    v[(opcode & 0x0F00) >> 8] &= v[(opcode & 0x00F0) >> 4];
                    if(quirks.vf_reset) v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0003:
                    v[(opcode & 0x0F00) >> 8] ^= v[(opcode & 0x00F0) >> 4];
                    if(quirks.vf_reset) v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0004:{
                    uint16_t sum = v[(opcode & 0x0F00) >> 8] + v[(opcode & 0x00F0) >> 4];
                    v[(opcode & 0x0F00) >> 8] = sum & 0xFF;
                    v[0xF] = (sum > 0xFF) ? 1: 0;
                    pc += 2;
                }
                    break;
                case 0x0005:{
                    uint8_t no_borrow = (v[(opcode & 0x0F00) >> 8] >= v[(opcode & 0x00F0) >> 4]) ? 1 : 0;
                    v[(opcode & 0x0F00) >> 8] -= v[(opcode & 0x00F0) >> 4];
                    v[0xF] = no_borrow;
                    pc += 2;
                }
                    break;
                case 0x0006:{
                    if(quirks.shift_uses_vy) v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    uint8_t lsb = v[(opcode & 0x0F00) >> 8] & 0x1;
                    v[(opcode & 0x0F00) >> 8] >>= 1;
                    v[0xF] = lsb;
                    pc += 2;
                }
                    break;
                case 0x0007:{
                    uint8_t no_borrow = (v[(opcode & 0x00F0) >> 4] >= v[(opcode & 0x0F00) >> 8]) ? 1 : 0;
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4] - v[(opcode & 0x0F00) >> 8];
                    v[0xF] = no_borrow;
                    pc += 2;
                }
                    break;
                case 0x000E:{
                    if(quirks.shift_uses_vy) v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    uint8_t msb = v[(opcode & 0x0F00) >> 8] >> 7;
                    v[(opcode & 0x0F00) >> 8] <<= 1;
                    v[0xF] = msb;
                    pc += 2;
                }
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0x9000:
            if (v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;
        case 0xA000:
            index = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xB000:
            pc = ((opcode & 0xFFF) + v[quirks.jump_uses_vx ? (opcode & 0x0F00) >> 8 : 0]) & 0xFFF;
            break;
        case 0xC000:
            v[(opcode & 0x0F00) >> 8] = (uint8_t)(rng() & 0xFF) & (opcode & 0x00FF);
            pc += 2;
            break;
        case 0xD000:
            draw_sprite(v[(opcode & 0x0F00) >> 8], v[(opcode & 0x00F0) >> 4], opcode & 0x000F);
            if(quirks.display_wait) vblank_wait = true;
            pc += 2;
            break;
        case 0xE000:
            switch(opcode & 0x00FF){
                case 0x009E:
                    key_polls |= 1u << (v[(opcode & 0x0F00) >> 8] & 0xF);
                    if(key[v[(opcode & 0x0F00) >> 8] & 0xF] != 0) pc += 4;
                    else pc += 2;
                    break;
                case 0x00A1:
                    key_polls |= 1u << (v[(opcode & 0x0F00) >> 8] & 0xF);
                    if(key[v[(opcode & 0x0F00) >> 8] & 0xF] == 0) pc += 4;
                    else pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
            }
            break;
        case 0xF000:
            switch(opcode & 0x00FF){
                case 0x0007:
                    v[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A:{
                    waits_for_any_key = true;
                    if(waiting_key < 0){
                        for(int i=0; i<16; i++){
                            if(key[i] != 0){ waiting_key = i; break; }
                        }
                    }
                    else if(key[waiting_key] == 0){
                        v[(opcode & 0x0F00) >> 8] = (uint8_t)waiting_key;
                        waiting_key = -1;
                        pc += 2;
                    }
                }
                    break;
                case 0x0015:
                    delay_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0018:
                    sound_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x001E:
                    index += v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0029:
                    index = SMALL_FONT_ADDR + (v[(opcode & 0x0F00) >> 8] & 0xF) * 5;
                    pc += 2;
                    break;
                case 0x0030:
                    index = BIG_FONT_ADDR + (v[(opcode & 0x0F00) >> 8] & 0xF) * 10;
                    pc += 2;
                    break;
                case 0x0075:
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++) rpl[i] = v[i];
                    pc += 2;
                    break;
                case 0x0085:
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++) v[i] = rpl[i];
                    pc += 2;
                    break;
                case 0x0033:{
                    uint8_t value = v[(opcode & 0x0F00) >> 8];
                    for(int i=0; i<3; i++) heat_write[(index+i) & 0xFFF] = 255;
                    memory[index & 0xFFF] = value/100;
                    memory[(index+1) & 0xFFF] = (value/10)%10;
                    memory[(index+2) & 0xFFF] = value%10;
                    pc += 2;
                }
                    break;
                case 0x0055:
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++){
                        memory[(index+i) & 0xFFF] = v[i];
                        heat_write[(index+i) & 0xFFF] = 255;
                    }
                    if(quirks.memory_increment) index += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;
                case 0x0065:
                    for(int i=0; i<=((opcode & 0x0F00) >> 8); i++){
                        v[i] = memory[(index+i) & 0xFFF];
                        heat_read[(index+i) & 0xFFF] = 255;
                    }
                    if(quirks.memory_increment) index += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
            }
            break;
        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            pc += 2;
            break;
    }
}

void Chip8::fade_heat(uint8_t amount){
    for(int i=0; i<4096; i++){
        heat_exec[i] = heat_exec[i] > amount ? heat_exec[i] - amount : 0;
        heat_read[i] = heat_read[i] > amount ? heat_read[i] - amount : 0;
        heat_write[i] = heat_write[i] > amount ? heat_write[i] - amount : 0;
    }
}

void Chip8::update_timers(){
    vblank_wait = false;
    if(delay_timer > 0) delay_timer--;
    if(sound_timer > 0) sound_timer--;
}
