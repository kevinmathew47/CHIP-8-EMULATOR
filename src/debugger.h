#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "chip8.h"
#include <SDL.h>
#include <set>
#include <vector>

const int DEBUG_WIDTH = 1040;
const int DEBUG_HEIGHT = 520;

class Debugger{
    public:
        std::set<uint16_t> breakpoints;
        bool show_map = false;
        int back_steps = 0;

        void draw(SDL_Renderer* renderer, const Chip8& chip8, bool paused);
        bool handle_click(int x, int y);
        void handle_wheel(int x, int y, int delta);
        void toggle_breakpoint(uint16_t addr);
        void reset_view(){ memory_offset = 0; }
    private:
        struct Row{ SDL_Rect area; uint16_t addr; };
        std::vector<Row> disasm_rows;
        int memory_offset = 0;
        SDL_Rect memory_area = {0, 0, 0, 0};

        void draw_registers(SDL_Renderer* renderer, const Chip8& chip8, int x, int y);
        void draw_disassembly(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int lines);
        void draw_memory(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int rows);
        void draw_memory_map(SDL_Renderer* renderer, const Chip8& chip8, int x, int y);
        void draw_stack(SDL_Renderer* renderer, const Chip8& chip8, int x, int y);
        void draw_keypad(SDL_Renderer* renderer, const Chip8& chip8, int x, int y);
};

#endif
