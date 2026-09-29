#include "debugger.h"
#include "disasm.h"
#include "text.h"
#include <algorithm>
#include <cstdio>
#include <string>

static const int S = 2;
static const int CHAR_W = 4*S;
static const int LINE_H = 6*S + 2;

static const SDL_Color WHITE  = {235, 235, 235, 255};
static const SDL_Color DIM    = {120, 120, 130, 255};
static const SDL_Color LABEL  = {255, 196, 70, 255};
static const SDL_Color RED    = {255, 90, 90, 255};
static const SDL_Color GREEN  = {90, 230, 120, 255};

static std::string hex(unsigned value, int digits){
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%0*X", digits, value);
    return buf;
}

static void fill(SDL_Renderer* renderer, SDL_Rect r, SDL_Color c){
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer, &r);
}

static void panel(SDL_Renderer* renderer, SDL_Rect r, const char* title){
    fill(renderer, r, {18, 18, 24, 255});
    SDL_SetRenderDrawColor(renderer, 60, 60, 75, 255);
    SDL_RenderDrawRect(renderer, &r);
    draw_text(renderer, title, r.x + 6, r.y + 5, S, LABEL);
}

void Debugger::draw(SDL_Renderer* renderer, const Chip8& chip8, bool paused){
    std::string reg_title = paused ? "REGISTERS  (PAUSED)" : "REGISTERS  (RUNNING)";
    if(back_steps > 0) reg_title += "  " + std::to_string(back_steps) + " STEPS BACK";
    panel(renderer, {644, 0, DEBUG_WIDTH - 644, 116}, reg_title.c_str());
    draw_registers(renderer, chip8, 652, 22);
    panel(renderer, {644, 120, DEBUG_WIDTH - 644, DEBUG_HEIGHT - 120}, "DISASSEMBLY  (CLICK = BREAKPOINT)");
    draw_disassembly(renderer, chip8, 652, 142, 26);

    if(show_map){
        panel(renderer, {0, 324, 420, DEBUG_HEIGHT - 324}, "MEMORY MAP, 4 KB LIVE  (F3 = HEX)");
        draw_memory_map(renderer, chip8, 8, 346);
    }
    else{
        panel(renderer, {0, 324, 420, DEBUG_HEIGHT - 324}, "MEMORY @ I  (F3 = MAP, WHEEL)");
        draw_memory(renderer, chip8, 8, 346, 11);
    }
    panel(renderer, {424, 324, 120, DEBUG_HEIGHT - 324}, "STACK");
    draw_stack(renderer, chip8, 432, 346);
    panel(renderer, {548, 324, 92, DEBUG_HEIGHT - 324}, "KEYS");
    draw_keypad(renderer, chip8, 556, 346);

    draw_text(renderer, "F1 CLOSE P RUN F6 STEP F4 BACK F7 BREAK", 8, DEBUG_HEIGHT - 16, S, DIM);
}

void Debugger::draw_memory_map(SDL_Renderer* renderer, const Chip8& chip8, int x, int y){
    const int CW = 6, CH = 2;
    for(int addr=0; addr<4096; addr++){
        int e = chip8.heat_exec[addr], r = chip8.heat_read[addr], w = chip8.heat_write[addr];
        int base = chip8.peek((uint16_t)addr) ? 38 : 20;
        SDL_Color c = {
            (uint8_t)std::min(255, base + w + r/6),
            (uint8_t)std::min(255, base + r*3/4 + e/3),
            (uint8_t)std::min(255, base + 12 + e),
            255};
        fill(renderer, {x + (addr % 64)*CW, y + (addr / 64)*CH, CW - 1, CH}, c);
    }
    uint16_t pc = chip8.get_pc() & 0xFFF, index = chip8.get_index() & 0xFFF;
    fill(renderer, {x + (pc % 64)*CW - 1, y + (pc / 64)*CH - 1, CW + 1, CH + 2}, WHITE);
    fill(renderer, {x + (index % 64)*CW - 1, y + (index / 64)*CH - 1, CW + 1, CH + 2}, LABEL);

    int ly = y + 64*CH + 6;
    fill(renderer, {x, ly + 1, 8, 8}, {60, 140, 255, 255});
    draw_text(renderer, "EXEC", x + 12, ly, S, DIM);
    fill(renderer, {x + 58, ly + 1, 8, 8}, {60, 220, 110, 255});
    draw_text(renderer, "READ", x + 70, ly, S, DIM);
    fill(renderer, {x + 116, ly + 1, 8, 8}, {255, 70, 70, 255});
    draw_text(renderer, "WRITE", x + 128, ly, S, DIM);
    fill(renderer, {x + 182, ly + 1, 8, 8}, WHITE);
    draw_text(renderer, "PC", x + 194, ly, S, DIM);
    fill(renderer, {x + 220, ly + 1, 8, 8}, LABEL);
    draw_text(renderer, "I", x + 232, ly, S, DIM);
    draw_text(renderer, "200 = PROGRAM START", x + 250, ly, S, DIM);
}

void Debugger::draw_registers(SDL_Renderer* renderer, const Chip8& chip8, int x, int y){
    std::string line1 = "PC " + hex(chip8.get_pc(), 3) + "   I " + hex(chip8.get_index(), 3) + "   SP " + std::to_string(chip8.get_sp())
                      + (chip8.is_halted() ? "   HALTED" : chip8.is_hires() ? "   128X64" : "   64X32");
    std::string line2 = "DT " + hex(chip8.get_delay_timer(), 2) + "    ST " + hex(chip8.get_sound_timer(), 2)
                      + "    OP " + hex(chip8.peek_opcode(chip8.get_pc()), 4);
    draw_text(renderer, line1, x, y, S, WHITE);
    draw_text(renderer, line2, x, y + LINE_H, S, WHITE);
    for(int i=0; i<16; i++){
        int cx = x + (i % 4)*(CHAR_W*9), cy = y + (2 + i/4)*LINE_H + 4;
        draw_text(renderer, "V" + hex(i, 1), cx, cy, S, i == 0xF ? LABEL : DIM);
        draw_text(renderer, hex(chip8.get_v(i), 2), cx + CHAR_W*3, cy, S, chip8.get_v(i) ? WHITE : DIM);
    }
}

void Debugger::draw_disassembly(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int lines){
    disasm_rows.clear();
    uint16_t pc = chip8.get_pc();
    int start = pc - 2*6;
    if(start < 0) start = pc & 1;
    for(int i=0; i<lines; i++){
        uint16_t addr = (uint16_t)(start + i*2);
        if(addr > 0xFFE) break;
        int row_y = y + i*LINE_H;
        SDL_Rect area = {x - 6, row_y - 2, DEBUG_WIDTH - x, LINE_H};
        disasm_rows.push_back({area, addr});

        bool is_pc = (addr == pc);
        bool has_break = breakpoints.count(addr) > 0;
        if(is_pc) fill(renderer, area, {40, 70, 140, 255});
        if(has_break) fill(renderer, {x - 4, row_y + 1, 6, 6}, RED);

        uint16_t op = chip8.peek_opcode(addr);
        draw_text(renderer, hex(addr, 3), x + 6, row_y, S, is_pc ? WHITE : DIM);
        draw_text(renderer, hex(op, 4), x + 6 + CHAR_W*5, row_y, S, is_pc ? WHITE : DIM);
        draw_text(renderer, disassemble(op), x + 6 + CHAR_W*11, row_y, S, has_break ? RED : WHITE);
    }
}

void Debugger::draw_memory(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int rows){
    memory_area = {0, 324, 420, DEBUG_HEIGHT - 324};
    uint16_t index = chip8.get_index();
    int base = (index & ~0x7) + memory_offset*8;
    for(int r=0; r<rows; r++){
        int addr = (base + r*8) & 0xFFF;
        int row_y = y + r*LINE_H;
        draw_text(renderer, hex(addr, 3), x, row_y, S, DIM);
        for(int b=0; b<8; b++){
            uint16_t a = (addr + b) & 0xFFF;
            uint8_t value = chip8.peek(a);
            int bx = x + CHAR_W*4 + b*CHAR_W*3;
            if(a == index) fill(renderer, {bx - 2, row_y - 2, CHAR_W*2 + 3, LINE_H}, {110, 70, 20, 255});
            draw_text(renderer, hex(value, 2), bx, row_y, S, value ? WHITE : DIM);
        }
    }

    int px = x + CHAR_W*29 + 8;
    draw_text(renderer, "SPRITE", px, y, S, DIM);
    for(int row=0; row<15; row++){
        uint8_t value = chip8.peek((uint16_t)(index + row));
        for(int bit=0; bit<8; bit++){
            SDL_Rect cell = {px + bit*7, y + LINE_H + row*7, 6, 6};
            fill(renderer, cell, (value & (0x80 >> bit)) ? GREEN : SDL_Color{35, 35, 45, 255});
        }
    }
}

void Debugger::draw_stack(SDL_Renderer* renderer, const Chip8& chip8, int x, int y){
    int sp = chip8.get_sp();
    if(sp == 0) draw_text(renderer, "EMPTY", x, y, S, DIM);
    for(int i=0; i<sp && i<11; i++){
        int slot = sp - 1 - i;
        draw_text(renderer, std::to_string(slot) + " " + hex(chip8.get_stack(slot), 3), x, y + i*LINE_H, S, i == 0 ? WHITE : DIM);
    }
}

void Debugger::draw_keypad(SDL_Renderer* renderer, const Chip8& chip8, int x, int y){
    static const int LAYOUT[16] = {0x1, 0x2, 0x3, 0xC, 0x4, 0x5, 0x6, 0xD, 0x7, 0x8, 0x9, 0xE, 0xA, 0x0, 0xB, 0xF};
    for(int i=0; i<16; i++){
        int k = LAYOUT[i];
        SDL_Rect cell = {x + (i % 4)*19, y + (i / 4)*22, 17, 20};
        bool down = chip8.key[k] != 0;
        bool used = (chip8.get_key_polls() >> k) & 1;
        fill(renderer, cell, down ? SDL_Color{255, 196, 70, 255} : SDL_Color{40, 40, 52, 255});
        if(used && !down){
            SDL_SetRenderDrawColor(renderer, GREEN.r, GREEN.g, GREEN.b, 255);
            SDL_RenderDrawRect(renderer, &cell);
        }
        draw_text(renderer, hex(k, 1), cell.x + 5, cell.y + 5, S, down ? SDL_Color{0, 0, 0, 255} : used ? WHITE : DIM);
    }
    draw_text(renderer, "GREEN =", x, y + 96, S, GREEN);
    draw_text(renderer, "USED BY", x, y + 110, S, DIM);
    draw_text(renderer, "THIS ROM", x, y + 124, S, DIM);
}

bool Debugger::handle_click(int x, int y){
    for(const Row& row : disasm_rows){
        if(x >= row.area.x && x < row.area.x + row.area.w && y >= row.area.y && y < row.area.y + row.area.h){
            toggle_breakpoint(row.addr);
            return true;
        }
    }
    if(x >= memory_area.x && x < memory_area.x + memory_area.w && y >= memory_area.y && y < memory_area.y + memory_area.h){
        memory_offset = 0;
        return true;
    }
    return false;
}

void Debugger::handle_wheel(int x, int y, int delta){
    if(x >= memory_area.x && x < memory_area.x + memory_area.w && y >= memory_area.y && y < memory_area.y + memory_area.h){
        memory_offset -= delta;
    }
}

void Debugger::toggle_breakpoint(uint16_t addr){
    if(breakpoints.count(addr)) breakpoints.erase(addr);
    else breakpoints.insert(addr);
}
