#include "text.h"
#include <cctype>
#include <cstdint>

// Each glyph is 5 rows of 3 bits (bit 2 = left column).
struct Glyph{ char c; uint8_t rows[5]; };

static const Glyph GLYPHS[] = {
    {'0',{7,5,5,5,7}}, {'1',{2,6,2,2,7}}, {'2',{7,1,7,4,7}}, {'3',{7,1,7,1,7}},
    {'4',{5,5,7,1,1}}, {'5',{7,4,7,1,7}}, {'6',{7,4,7,5,7}}, {'7',{7,1,1,2,2}},
    {'8',{7,5,7,5,7}}, {'9',{7,5,7,1,7}},
    {'A',{2,5,7,5,5}}, {'B',{6,5,6,5,6}}, {'C',{3,4,4,4,3}}, {'D',{6,5,5,5,6}},
    {'E',{7,4,6,4,7}}, {'F',{7,4,6,4,4}}, {'G',{3,4,5,5,3}}, {'H',{5,5,7,5,5}},
    {'I',{7,2,2,2,7}}, {'J',{1,1,1,5,2}}, {'K',{5,5,6,5,5}}, {'L',{4,4,4,4,7}},
    {'M',{5,7,7,5,5}}, {'N',{6,5,5,5,5}}, {'O',{2,5,5,5,2}}, {'P',{6,5,6,4,4}},
    {'Q',{2,5,5,6,3}}, {'R',{6,5,6,5,5}}, {'S',{3,4,2,1,6}}, {'T',{7,2,2,2,2}},
    {'U',{5,5,5,5,7}}, {'V',{5,5,5,5,2}}, {'W',{5,5,7,7,5}}, {'X',{5,5,2,5,5}},
    {'Y',{5,5,2,2,2}}, {'Z',{7,1,2,4,7}},
    {'.',{0,0,0,0,2}}, {':',{0,2,0,2,0}}, {'-',{0,0,7,0,0}}, {'+',{0,2,7,2,0}},
    {'/',{1,1,2,4,4}}, {'[',{3,2,2,2,3}}, {']',{6,2,2,2,6}}, {'%',{5,1,2,4,5}},
    {'!',{2,2,2,0,2}}, {'?',{6,1,2,0,2}}, {'<',{1,2,4,2,1}}, {'>',{4,2,1,2,4}},
    {'=',{0,7,0,7,0}}, {',',{0,0,0,2,4}}, {'(',{1,2,2,2,1}}, {')',{4,2,2,2,4}},
    {'@',{2,5,7,4,3}}, {'_',{0,0,0,0,7}}, {'#',{5,7,5,7,5}}, {'\'',{2,2,0,0,0}}, {'|',{2,2,2,2,2}},
};

static const Glyph* find_glyph(char c){
    c = (char)std::toupper((unsigned char)c);
    for(const Glyph& g : GLYPHS) if(g.c == c) return &g;
    return nullptr; // Unknown characters (and space) render as blank
}

void draw_text(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, SDL_Color color){
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int cursor_x = x;
    for(char c : text){
        if(c == '\n'){ cursor_x = x; y += 6*scale; continue; }
        const Glyph* g = find_glyph(c);
        if(g){
            for(int row=0; row<5; row++){
                for(int col=0; col<3; col++){
                    if(g->rows[row] & (4 >> col)){
                        SDL_Rect r = {cursor_x + col*scale, y + row*scale, scale, scale};
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
        }
        cursor_x += 4*scale;
    }
}

int text_width(const std::string& text, int scale){
    int longest = 0, current = 0;
    for(char c : text){
        if(c == '\n'){ current = 0; continue; }
        current++;
        if(current > longest) longest = current;
    }
    return longest > 0 ? longest*4*scale - scale : 0;
}

int text_height(int scale){
    return 5*scale;
}
