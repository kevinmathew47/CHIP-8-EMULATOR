#ifndef TEXT_H
#define TEXT_H

#include <SDL.h>
#include <string>

void draw_text(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, SDL_Color color);
int text_width(const std::string& text, int scale);
int text_height(int scale);

#endif
