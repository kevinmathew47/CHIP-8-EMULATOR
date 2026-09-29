#ifndef TEXT_H
#define TEXT_H

#include <SDL.h>
#include <string>

// Tiny 3x5 bitmap font for on-screen messages, so the emulator needs no font library.
// Each glyph cell is 4x6 units (3x5 plus spacing); `scale` is screen pixels per unit.
void draw_text(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, SDL_Color color);
int text_width(const std::string& text, int scale);
int text_height(int scale);

#endif
