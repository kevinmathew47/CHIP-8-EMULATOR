#include "chip8.h"
#include "debugger.h"
#include "text.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace fs = std::filesystem;

const int SCALE = 10; // Each low-res pixel is 10x10 screen pixels
const int WIDTH = 64*SCALE;
const int HEIGHT = 32*SCALE;
const int CELL = WIDTH/DISPLAY_W; // Screen pixels per display-buffer cell (5, i.e. one high-res pixel)
const double PI = 3.14159265358979323846;
const size_t REWIND_FRAMES = 60*10; // Ten seconds of history
const int SAVE_SLOTS = 10;

// Keyboard mapping
SDL_Keycode keymap[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E
    SDLK_v  // F
};

// Instructions per frame; multiply by 60 for instructions per second
const int SPEED_STEPS[] = {1, 2, 3, 5, 8, 10, 15, 20, 30, 50, 100, 200, 500, 1000};
const int SPEED_STEP_COUNT = sizeof(SPEED_STEPS)/sizeof(SPEED_STEPS[0]);
const int DEFAULT_SPEED_STEP = 5; // 10 per frame = 600 Hz

// ROMs that need non-default settings to run correctly, keyed by CRC32 of the file
struct RomInfo{ uint32_t crc; const char* title; QuirkProfile quirks; int speed_step; const char* controls; };
const RomInfo ROM_DATABASE[] = {
    {0x9d307e90, "Blinky", QuirkProfile::SCHIP, 7, "V START   3 UP  E DOWN  A LEFT  S RIGHT"}, // Relies on SCHIP shift/load behaviour, runs best at 1200 IPS
    {0x7d75a857, "Pong",   QuirkProfile::CHIP8, DEFAULT_SPEED_STEP, "LEFT PADDLE 1 / Q    RIGHT PADDLE 4 / R"},
    {0x0ce70772, "Tetris", QuirkProfile::CHIP8, DEFAULT_SPEED_STEP, "Q ROTATE  W LEFT  E RIGHT  A DROP"},
    // Timendus test suite (tests/roms): proof that the fixed core passes every check
    {0x42adaf83, "IBM Logo test",   QuirkProfile::CHIP8, DEFAULT_SPEED_STEP, "NO INPUT - DRAWS THE IBM LOGO"},
    {0x561bf2f2, "Opcode test",     QuirkProfile::CHIP8, DEFAULT_SPEED_STEP, "NO INPUT - A TICK FOR EVERY OPCODE"},
    {0x3e251b98, "Flags test",      QuirkProfile::CHIP8, DEFAULT_SPEED_STEP, "NO INPUT - A TICK FOR EVERY FLAG CHECK"},
    {0xae214ed4, "Quirks test",     QuirkProfile::COSMAC, DEFAULT_SPEED_STEP, "PRESS 1 FOR CHIP-8 (F2 + 2 FOR SCHIP)"},
    {0x0ab291f3, "Scrolling test",  QuirkProfile::SCHIP, DEFAULT_SPEED_STEP, "PRESS 1 (LOW-RES) OR 3 (HIGH-RES)"},
    // SUPER-CHIP games from the chip8Archive (CC0), see roms/schip/README.md. Speed and quirks follow
    // the archive's own recommended settings (Octo defaults = our XO-CHIP profile)
    {0x92e250ca, "Eaty The Alien", QuirkProfile::XOCHIP, 11, "E START / ACTION   W A S D MOVE"}, // 200 per frame
    {0x35c089ce, "Super Octogon",  QuirkProfile::SCHIP,  11, "A / D TURN"}, // 200 per frame
    {0x8bd69060, "Black Rainbow",  QuirkProfile::XOCHIP, 7, "D START   W A S D MOVE   X ACTION"}, // 20 per frame
    {0x36df976a, "Rockto",         QuirkProfile::XOCHIP, 6, "ANY KEY START   A S D MOVE   1 REDRAW LEVEL"}, // 15 per frame
};

const RomInfo* find_rom_info(uint32_t crc){
    for(const RomInfo& info : ROM_DATABASE) if(info.crc == crc) return &info;
    return nullptr;
}

// Keyboard key for each CHIP-8 key (see keymap), used to describe detected controls
const char KEY_LABELS[] = "X123QWEASDZC4RFV";

struct Palette{ const char* name; SDL_Color background; SDL_Color foreground; };
const Palette PALETTES[] = {
    {"Classic",        {0, 0, 0, 255},       {255, 255, 255, 255}},
    {"Classic Green Screen", {0, 18, 4, 255},     {51, 255, 102, 255}},
    {"Amber CRT",      {24, 12, 0, 255},     {255, 176, 0, 255}},
    {"Neon High-Contrast", {12, 0, 28, 255},    {0, 255, 240, 255}},
    {"Game Boy",       {155, 188, 15, 255},  {15, 56, 15, 255}},
    {"Ice",            {0, 14, 40, 255},     {170, 220, 255, 255}},
    {"Paper",          {238, 232, 213, 255}, {40, 40, 40, 255}},
    {"Hot Pink",       {20, 0, 10, 255},     {255, 60, 170, 255}},
};
const int PALETTE_COUNT = sizeof(PALETTES)/sizeof(PALETTES[0]);

enum Waveform{ WAVE_SQUARE = 0, WAVE_TRIANGLE, WAVE_SINE, WAVE_SAWTOOTH, WAVE_COUNT };
const char* WAVEFORM_NAMES[] = {"Square", "Triangle", "Sine", "Sawtooth"};

struct AudioState{
    bool beeping = false;
    int waveform = WAVE_SQUARE;
    double frequency = 440.0;
    double phase = 0.0; // 0..1 position within the current wave cycle
    int sample_rate = 44100;
    uint64_t samples_played = 0; // Non-silent samples produced; lets tests prove sound really plays
};

void audio_callback(void* userdata, uint8_t* stream, int len){
    AudioState* audio = (AudioState*) userdata;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len/2;
    const double amplitude = 7000.0; // About 21% of full scale: clearly audible on laptop speakers

    for(int i=0; i<samples; i++){
        if(!audio->beeping){
            audio_buffer[i] = 0; // Silence
            audio->phase = 0.0;
            continue;
        }
        double p = audio->phase, value;
        switch(audio->waveform){
            case WAVE_TRIANGLE: value = 4.0*std::fabs(p - 0.5) - 1.0; break;
            case WAVE_SINE:     value = std::sin(2.0*PI*p); break;
            case WAVE_SAWTOOTH: value = 2.0*p - 1.0; break;
            default:            value = (p < 0.5) ? 1.0 : -1.0; break;
        }
        audio_buffer[i] = (int16_t)(value*amplitude);
        audio->samples_played++;
        audio->phase += audio->frequency/audio->sample_rate;
        if(audio->phase >= 1.0) audio->phase -= 1.0;
    }
}

struct App{
    Chip8 chip8;
    std::string rom_path;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioDeviceID audio_device = 0;
    AudioState audio;

    bool running = true;
    bool paused = false;
    bool step_frame = false;
    bool step_instruction = false;
    bool skip_breakpoint_once = false; // Lets execution leave a breakpoint after resuming
    bool debug_open = false;
    Debugger debugger;
    bool rewinding = false;
    bool show_help = false;
    bool phosphor = true;   // Fade pixels out over a few frames instead of flickering
    bool scanlines = false;
    bool muted = false;
    std::string title;              // Game title shown to the player (database title or file name)
    int speed_step = DEFAULT_SPEED_STEP;
    int palette = 0;
    int save_slot = 0;
    std::string controls;          // Known controls for this ROM, or empty to auto-detect
    uint32_t random_seed = 0;      // --seed: fixed CXNN random sequence for repeatable demos (0 = random)
    uint16_t announced_keys = 0;   // Detected keys already shown to the player
    uint32_t controls_until = 0;   // Controls banner is visible until this tick
    std::deque<Chip8State> step_history; // Per-instruction states for stepping backwards in the debugger
    bool quirks_forced = false; // Set by --quirks / --speed so the ROM database doesn't override them
    bool speed_forced = false;
    int capture_after = -1; // --capture N: screenshot after N frames and quit (for docs/CI)
    // --press KEY@FRAME[:HOLD] replays key presses, so demos and screenshots are reproducible
    struct ScriptedKey{ int frame; SDL_Keycode key; bool down; };
    std::vector<ScriptedKey> script;
    int frame_counter = 0;

    void play_script(){
        for(const ScriptedKey& k : script){
            if(k.frame != frame_counter) continue;
            SDL_Event e;
            SDL_zero(e);
            e.type = k.down ? SDL_KEYDOWN : SDL_KEYUP;
            e.key.state = k.down ? SDL_PRESSED : SDL_RELEASED;
            e.key.keysym.sym = k.key;
            SDL_PushEvent(&e);
        }
        frame_counter++;
    }
    std::string capture_path;
    std::string forced_capture_path;

    float intensity[DISPLAY_W*DISPLAY_H] = {}; // Per-cell brightness used for the phosphor effect
    std::deque<Chip8State> history;

    std::string message;
    uint32_t message_until = 0;

    void notify(const std::string& text){
        message = text;
        message_until = SDL_GetTicks() + 2000;
        std::cout << text << std::endl;
    }

    std::string rom_name() const{
        return fs::path(rom_path).stem().string();
    }

    std::string slot_path(int slot) const{
        return (fs::path("saves") / (rom_name() + ".slot" + std::to_string(slot) + ".c8s")).string();
    }

    void update_title(){
        std::string title = "CHIP-8 | " + rom_name() + " | " + std::to_string(SPEED_STEPS[speed_step]*60) + " IPS | "
            + PALETTES[palette].name + " | " + quirk_profile_name(chip8.get_quirk_profile()) + " | Slot " + std::to_string(save_slot)
            + (paused ? " | PAUSED" : "") + " | H for help | Built by Kevin Mathew";
        SDL_SetWindowTitle(window, title.c_str());
    }

    bool load_rom(const std::string& path){
        Chip8 fresh;
        fresh.set_quirk_profile(chip8.get_quirk_profile());
        if(!fresh.load_rom(path)){
            notify("Could not load ROM");
            return false;
        }
        if(!rom_path.empty()) debugger.breakpoints.clear(); // Addresses from the previous ROM mean nothing now
        chip8 = fresh;
        if(random_seed) chip8.seed_random(random_seed);
        rom_path = path;
        history.clear();
        std::memset(intensity, 0, sizeof(intensity));
        const RomInfo* info = find_rom_info(chip8.get_rom_crc());
        if(info){
            if(!quirks_forced) chip8.set_quirk_profile(info->quirks);
            if(!speed_forced) speed_step = info->speed_step;
            notify(std::string("Loaded ") + info->title + " (" + quirk_profile_name(chip8.get_quirk_profile()) + ")");
            title = info->title;
            controls = info->controls;
        }
        else{
            notify("Loaded " + rom_name());
            title = rom_name();
            controls.clear();
        }
        announced_keys = 0;
        controls_until = SDL_GetTicks() + 6000;
        step_history.clear();
        update_title();
        return true;
    }

    void set_paused(bool value){
        if(paused && !value) skip_breakpoint_once = true;
        paused = value;
        update_title();
    }

    void set_debugger_open(bool open){
        debug_open = open;
        int w = open ? DEBUG_WIDTH : WIDTH, h = open ? DEBUG_HEIGHT : HEIGHT;
        SDL_RenderSetLogicalSize(renderer, w, h);
        if(!(SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN))) SDL_SetWindowSize(window, w, h);
        debugger.reset_view();
    }

    void change_speed(int delta){
        speed_step += delta;
        if(speed_step < 0) speed_step = 0;
        if(speed_step >= SPEED_STEP_COUNT) speed_step = SPEED_STEP_COUNT - 1;
        notify("Speed " + std::to_string(SPEED_STEPS[speed_step]*60) + " IPS");
        update_title();
    }

    void save_to_slot(){
        std::error_code ec;
        fs::create_directories("saves", ec);
        if(chip8.save_state_to_file(slot_path(save_slot))) notify("Saved slot " + std::to_string(save_slot));
        else notify("Save failed");
    }

    void load_from_slot(){
        if(chip8.load_state_from_file(slot_path(save_slot))){
            history.clear(); // Rewinding across a load would jump between timelines
            step_history.clear();
            std::memset(intensity, 0, sizeof(intensity)); // No ghosts from the timeline we just left
            notify("Loaded slot " + std::to_string(save_slot));
        }
        else notify("Slot " + std::to_string(save_slot) + " is empty");
    }

    void screenshot(const std::string& target = ""){
        std::error_code ec;
        if(target.empty()) fs::create_directories("captures", ec);
        int w, h;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
        if(!surface){ notify("Screenshot failed"); return; }
        SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
        std::string path = !target.empty() ? target
            : (fs::path("captures") / (rom_name() + "_" + std::to_string(frame_counter) + ".bmp")).string();
        if(SDL_SaveBMP(surface, path.c_str()) == 0) notify("Screenshot saved");
        else notify("Screenshot failed");
        SDL_FreeSurface(surface);
    }

    void handle_hotkey(SDL_Keycode sym){
        switch(sym){
            case SDLK_ESCAPE: running = false; break;
            case SDLK_p:
                set_paused(!paused);
                notify(paused ? "Paused" : "Resumed");
                break;
            case SDLK_n:
                if(paused){ step_frame = true; skip_breakpoint_once = true; }
                break;
            case SDLK_F1: set_debugger_open(!debug_open); break;
            case SDLK_F4: step_back(); break;
            case SDLK_F3:
                debugger.show_map = !debugger.show_map;
                if(!debug_open) set_debugger_open(true);
                break;
            case SDLK_F6:
                if(!paused){ set_paused(true); notify("Paused"); }
                else step_instruction = true;
                break;
            case SDLK_F7:{
                uint16_t pc = chip8.get_pc();
                debugger.toggle_breakpoint(pc);
                char buf[40];
                std::snprintf(buf, sizeof(buf), "Breakpoint %s at %03X", debugger.breakpoints.count(pc) ? "set" : "cleared", pc);
                notify(buf);
                break;
            }
            case SDLK_EQUALS: case SDLK_PLUS: case SDLK_KP_PLUS: change_speed(+1); break;
            case SDLK_MINUS: case SDLK_KP_MINUS: change_speed(-1); break;
            case SDLK_0: case SDLK_KP_0:
                speed_step = DEFAULT_SPEED_STEP;
                notify("Speed reset");
                update_title();
                break;
            case SDLK_F5: save_to_slot(); break;
            case SDLK_F9: load_from_slot(); break;
            case SDLK_LEFTBRACKET:
                save_slot = (save_slot + SAVE_SLOTS - 1) % SAVE_SLOTS;
                notify("Slot " + std::to_string(save_slot) + (fs::exists(slot_path(save_slot)) ? " (saved)" : " (empty)"));
                update_title();
                break;
            case SDLK_RIGHTBRACKET:
                save_slot = (save_slot + 1) % SAVE_SLOTS;
                notify("Slot " + std::to_string(save_slot) + (fs::exists(slot_path(save_slot)) ? " (saved)" : " (empty)"));
                update_title();
                break;
            case SDLK_TAB:
                palette = (palette + 1) % PALETTE_COUNT;
                notify(std::string("Palette: ") + PALETTES[palette].name);
                update_title();
                break;
            case SDLK_g:
                phosphor = !phosphor;
                notify(phosphor ? "Phosphor fade on" : "Phosphor fade off");
                break;
            case SDLK_l:
                scanlines = !scanlines;
                notify(scanlines ? "Scanlines on" : "Scanlines off");
                break;
            case SDLK_b: // Sound check: half a second of tone, whatever the ROM is doing
                chip8.set_sound_timer(30);
                notify("Sound test: 0.5 s tone");
                break;
            case SDLK_u:
                muted = !muted;
                notify(muted ? "Sound off" : "Sound on");
                break;
            case SDLK_m:
                SDL_LockAudioDevice(audio_device);
                audio.waveform = (audio.waveform + 1) % WAVE_COUNT;
                SDL_UnlockAudioDevice(audio_device);
                notify(std::string("Sound: ") + WAVEFORM_NAMES[audio.waveform]);
                break;
            case SDLK_F2:{
                QuirkProfile next = (QuirkProfile)(((int)chip8.get_quirk_profile() + 1) % (int)QuirkProfile::COUNT);
                chip8.set_quirk_profile(next);
                notify(std::string("Quirks: ") + quirk_profile_name(next));
                update_title();
                break;
            }
            case SDLK_F8:
                chip8.reset();
                history.clear();
                step_history.clear();
                announced_keys = 0;
                notify("Reset");
                break;
            case SDLK_F12: screenshot(); break;
            case SDLK_h: show_help = !show_help; break;
            default: break;
        }
    }

    void handle_input(){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running = false;
            // Key-up events are lost while another window has focus, so release everything
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                std::memset(chip8.key, 0, sizeof(chip8.key));
                rewinding = false;
            }
            if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && debug_open){
                debugger.handle_click(event.button.x, event.button.y); // Already in logical coordinates
            }
            if(event.type == SDL_MOUSEWHEEL && debug_open){
                int mx, my;
                float lx, ly;
                SDL_GetMouseState(&mx, &my);
                SDL_RenderWindowToLogical(renderer, mx, my, &lx, &ly);
                debugger.handle_wheel((int)lx, (int)ly, event.wheel.y);
            }
            if(event.type == SDL_DROPFILE){
                load_rom(event.drop.file);
                SDL_free(event.drop.file);
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.repeat == 0) handle_hotkey(event.key.keysym.sym);
                if(event.key.keysym.sym == SDLK_BACKSPACE) rewinding = true;
                // Check which Chip-8 key was pressed
                for(int i=0; i<16; i++){
                    if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 1;
                }
            }
            if(event.type == SDL_KEYUP){
                if(event.key.keysym.sym == SDLK_BACKSPACE) rewinding = false;
                for(int i=0; i<16; i++){
                    if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 0;
                }
            }
        }
    }

    void run_frame(){
        if(rewinding){
            // Step back two frames per real frame so rewinding feels quicker than playing
            for(int i=0; i<2 && !history.empty(); i++){
                uint8_t keys[16];
                std::memcpy(keys, chip8.key, sizeof(keys)); // Keys reflect the keyboard, not the past
                chip8.load_state(history.back());
                std::memcpy(chip8.key, keys, sizeof(keys));
                history.pop_back();
                step_history.clear();
            }
            return;
        }
        if(paused && step_instruction){
            step_instruction = false;
            record_step();
            chip8.emulate_cycle();
            chip8.vblank_wait = false; // Stepping ignores the display wait so every press makes progress
            return;
        }
        if(paused && !step_frame) return;
        step_frame = false;

        if(chip8.is_halted()){
            if(message.find("exited") == std::string::npos || SDL_GetTicks() >= message_until) notify("Program exited - F8 to restart");
            return;
        }

        history.push_back(chip8.save_state());
        if(history.size() > REWIND_FRAMES) history.pop_front();

        for(int i=0; i<SPEED_STEPS[speed_step] && !chip8.vblank_wait; i++){
            if(!skip_breakpoint_once && debugger.breakpoints.count(chip8.get_pc())){
                set_paused(true);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Breakpoint hit at %03X", chip8.get_pc());
                notify(buf);
                if(!debug_open) set_debugger_open(true);
                return;
            }
            skip_breakpoint_once = false;
            if(debug_open) record_step();
            chip8.emulate_cycle();
        }
        chip8.update_timers();
    }

    static const size_t STEP_HISTORY = 1000; // About 12 MB of full machine states

    void record_step(){
        step_history.push_back(chip8.save_state());
        if(step_history.size() > STEP_HISTORY) step_history.pop_front();
    }

    // Time-travel debugging: undo the last instruction by restoring the state saved before it ran
    void step_back(){
        if(!paused){ set_paused(true); notify("Paused"); return; }
        if(step_history.empty()){ notify("No earlier steps recorded (open F1 first)"); return; }
        uint8_t keys[16];
        std::memcpy(keys, chip8.key, sizeof(keys));
        chip8.load_state(step_history.back());
        std::memcpy(chip8.key, keys, sizeof(keys));
        step_history.pop_back();
        char buf[40];
        std::snprintf(buf, sizeof(buf), "Stepped back to %03X", chip8.get_pc());
        notify(buf);
    }

    // Controls banner: known ROMs show their verified controls; others show the keys the ROM
    // has actually been seen checking, updated whenever it starts reading a new key.
    void update_controls_banner(){
        uint16_t polls = chip8.get_key_polls();
        if(controls.empty() && (polls & ~announced_keys)){
            announced_keys = polls;
            controls_until = SDL_GetTicks() + 5000;
        }
    }

    std::string controls_text() const{
        if(!controls.empty()) return controls;
        std::string keys;
        for(int k=0; k<16; k++) if((announced_keys >> k) & 1){ keys += KEY_LABELS[k]; keys += ' '; }
        if(keys.empty()) return chip8.get_waits_for_any_key() ? "PRESS ANY KEY" : "";
        return "DETECTED KEYS: " + keys;
    }

    // One 60Hz frame: input, emulation, audio, drawing. Both the native loop and the browser call this.
    void tick(){
        play_script();
        handle_input();
        run_frame();
        chip8.fade_heat(6);
        update_controls_banner();

        if(audio_device != 0){
            SDL_LockAudioDevice(audio_device);
            audio.beeping = (chip8.get_sound_timer() > 0) && !paused && !rewinding && !muted;
            SDL_UnlockAudioDevice(audio_device);
        }
        draw();
        if(capture_after >= 0 && capture_after-- == 0){
            screenshot(forced_capture_path);
            running = false;
        }
    }

    void draw(){
        const Palette& pal = PALETTES[palette];
        SDL_SetRenderDrawColor(renderer, pal.background.r, pal.background.g, pal.background.b, 255);
        SDL_RenderClear(renderer);

        // Phosphor: lit pixels are full brightness, unlit ones fade out over a few frames
        bool frozen = paused; // Keep fading while rewinding, or every past position stays lit
        for(int i=0; i<DISPLAY_W*DISPLAY_H; i++){
            if(chip8.display[i]) intensity[i] = 1.0f;
            else if(!phosphor) intensity[i] = 0.0f;
            else if(!frozen) intensity[i] *= 0.55f;
            if(intensity[i] < 0.02f) intensity[i] = 0.0f;
        }
        for(int y=0; y<DISPLAY_H; y++){
            for(int x=0; x<DISPLAY_W; x++){
                float a = intensity[x + y*DISPLAY_W];
                if(a <= 0.0f) continue;
                SDL_SetRenderDrawColor(renderer,
                    (uint8_t)(pal.background.r + (pal.foreground.r - pal.background.r)*a),
                    (uint8_t)(pal.background.g + (pal.foreground.g - pal.background.g)*a),
                    (uint8_t)(pal.background.b + (pal.foreground.b - pal.background.b)*a), 255);
                SDL_Rect rect = {x*CELL, y*CELL, CELL, CELL};
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        if(scanlines){
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90);
            for(int y=0; y<HEIGHT; y+=3) SDL_RenderDrawLine(renderer, 0, y, WIDTH, y);
        }

        draw_overlays();
        debugger.back_steps = (int)step_history.size();
        if(debug_open) debugger.draw(renderer, chip8, paused);
        SDL_RenderPresent(renderer);
    }

    void draw_panel(const std::string& text, int x, int y, int scale){
        int pad = 2*scale;
        SDL_Rect box = {x - pad, y - pad, text_width(text, scale) + 2*pad, 0};
        int lines = 1;
        for(char c : text) if(c == '\n') lines++;
        box.h = lines*6*scale - scale + 2*pad;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
        SDL_RenderFillRect(renderer, &box);
        draw_text(renderer, text, x, y, scale, {255, 255, 255, 255});
    }

    void draw_overlays(){
        if(rewinding){
            std::string label = "<< REWIND " + std::to_string(history.size()*100/REWIND_FRAMES) + "%";
            draw_panel(label, 10, 10, 3);
            // History bar along the bottom
            SDL_Rect bar = {0, HEIGHT - 6, (int)(WIDTH*history.size()/REWIND_FRAMES), 6};
            SDL_SetRenderDrawColor(renderer, 255, 80, 80, 220);
            SDL_RenderFillRect(renderer, &bar);
        }
        else if(paused){
            draw_panel("PAUSED  N: STEP  P: RESUME", 10, 10, 2);
        }

        if(!message.empty() && SDL_GetTicks() < message_until){
            int w = text_width(message, 2);
            draw_panel(message, WIDTH - w - 10, HEIGHT - 20, 2);
        }

        std::string ctl = controls_text();
        if(!ctl.empty() && SDL_GetTicks() < controls_until && !show_help && !rewinding){
            int w = text_width(ctl, 2);
            draw_panel(ctl, (WIDTH - w)/2, HEIGHT - 44, 2);
        }

        if(show_help){
            const std::string help =
                "CHIP-8 KEYPAD   1 2 3 4 / Q W E R / A S D F / Z X C V\n"
                "\n"
                "P  PAUSE        N  STEP FRAME   ESC  QUIT\n"
                "- =  SPEED      0  RESET SPEED  F8   RESET ROM\n"
                "BACKSPACE (HOLD)  REWIND\n"
                "F5 SAVE   F9 LOAD   [ ]  CHANGE SLOT\n"
                "TAB  PALETTE    G  PHOSPHOR     L  SCANLINES\n"
                "M  SOUND WAVE   U  MUTE         B    SOUND TEST\n"
                "F2 QUIRKS\n"
                "F1 DEBUGGER     F6  STEP INSTR  F7   BREAKPOINT\n"
                "F3 MEMORY MAP   F4  STEP BACK   F12  SCREENSHOT\n"
                "DRAG A .CH8 FILE ONTO THE WINDOW TO LOAD IT\n"
                "\n"
                "THIS GAME: " + (ctl.empty() ? std::string("KEYS NOT DETECTED YET") : ctl) + "\n"
                "H  CLOSE HELP              BUILT BY KEVIN MATHEW";
            int w = text_width(help, 2);
            draw_panel(help, (WIDTH - w)/2, 40, 2);
        }
    }
};

#ifdef __EMSCRIPTEN__
static App* web_app = nullptr;

// Called from the web page after it writes a ROM (bundled or uploaded) into the virtual filesystem
extern "C" EMSCRIPTEN_KEEPALIVE int web_load_rom(const char* path){
    return web_app && web_app->load_rom(path) ? 1 : 0;
}

// Lets the page's buttons trigger the same actions as the keyboard shortcuts
extern "C" EMSCRIPTEN_KEEPALIVE void web_hotkey(int keycode){
    if(web_app) web_app->handle_hotkey((SDL_Keycode)keycode);
}

// "Hold to rewind" button: same as holding Backspace
extern "C" EMSCRIPTEN_KEEPALIVE void web_set_rewind(int on){
    if(web_app) web_app->rewinding = on != 0;
}

// On-screen keypad: press or release CHIP-8 key 0-F
extern "C" EMSCRIPTEN_KEEPALIVE void web_set_key(int key, int down){
    if(web_app) web_app->chip8.key[key & 0xF] = down ? 1 : 0;
}

// Everything the page shows about the running game, as a small JSON object
extern "C" EMSCRIPTEN_KEEPALIVE const char* web_get_info(){
    static std::string json;
    if(!web_app) return "{}";
    App& a = *web_app;
    uint16_t pressed = 0;
    for(int k=0; k<16; k++) if(a.chip8.key[k]) pressed |= 1u << k;
    auto quote = [](const std::string& text){
        std::string out = "\"";
        for(char c : text){ if(c == '"' || c == '\\') out += '\\'; out += c; }
        return out + "\"";
    };
    json = "{\"title\":" + quote(a.title)
         + ",\"ips\":" + std::to_string(SPEED_STEPS[a.speed_step]*60)
         + ",\"palette\":" + quote(PALETTES[a.palette].name)
         + ",\"quirks\":" + quote(quirk_profile_name(a.chip8.get_quirk_profile()))
         + ",\"controls\":" + quote(a.controls_text())
         + ",\"polls\":" + std::to_string(a.chip8.get_key_polls())
         + ",\"pressed\":" + std::to_string(pressed)
         + ",\"paused\":" + (a.paused ? "true" : "false")
         + ",\"debugger\":" + (a.debug_open ? "true" : "false")
         + ",\"muted\":" + (a.muted ? "true" : "false")
         + ",\"beeping\":" + (a.chip8.get_sound_timer() > 0 ? "true" : "false")
         + ",\"audio_ok\":" + (a.audio_device != 0 ? "true" : "false")
         + ",\"audio_samples\":" + std::to_string(a.audio.samples_played)
         + ",\"wave\":" + quote(WAVEFORM_NAMES[a.audio.waveform])
         + ",\"slot\":" + std::to_string(a.save_slot)
         + ",\"hires\":" + (a.chip8.is_hires() ? "true" : "false")
         + ",\"phosphor\":" + (a.phosphor ? "true" : "false")
         + ",\"scanlines\":" + (a.scanlines ? "true" : "false")
         + ",\"back\":" + std::to_string(a.step_history.size())
         + ",\"message\":" + quote(SDL_GetTicks() < a.message_until ? a.message : "")
         + "}";
    return json.c_str();
}
#endif

void print_usage(const char* program){
    std::cerr << "Usage: " << program << " <ROM file> [--quirks chip8|schip|xochip] [--speed N] [--palette N]"
              << " (quirks: chip8, cosmac, schip, xochip)" << " [--scanlines] [--help-overlay] [--debugger] [--break HEXADDR] [--press KEY@FRAME[:HOLD]] [--seed N] [--capture FRAMES OUT.bmp]" << std::endl;
}

int main(int argc, char** argv){
    if(argc < 2){
        print_usage(argv[0]);
        return 1;
    }

    // Static, not a stack variable: in the browser main() returns early while the frame loop keeps using app
    static App app;
    std::cout << "CHIP-8 Emulator - built by Kevin Mathew (" << BUILD_SIGNATURE << ")" << std::endl;
    if(std::string(argv[1]) == "--version") return 0;
    for(int i=2; i<argc; i++){
        std::string arg = argv[i];
        if(arg == "--quirks" && i+1 < argc){
            std::string p = argv[++i];
            if(p == "schip") app.chip8.set_quirk_profile(QuirkProfile::SCHIP);
            else if(p == "xochip") app.chip8.set_quirk_profile(QuirkProfile::XOCHIP);
            else if(p == "cosmac") app.chip8.set_quirk_profile(QuirkProfile::COSMAC);
            else app.chip8.set_quirk_profile(QuirkProfile::CHIP8);
            app.quirks_forced = true;
        }
        else if(arg == "--speed" && i+1 < argc){
            int target = std::atoi(argv[++i]);
            for(int s=0; s<SPEED_STEP_COUNT; s++) if(SPEED_STEPS[s] <= target) app.speed_step = s;
            app.speed_forced = true;
        }
        else if(arg == "--palette" && i+1 < argc){
            app.palette = std::atoi(argv[++i]) % PALETTE_COUNT;
            if(app.palette < 0) app.palette = 0;
        }
        else if(arg == "--scanlines") app.scanlines = true;
        else if(arg == "--help-overlay") app.show_help = true;
        else if(arg == "--debugger") app.debug_open = true;
        else if(arg == "--seed" && i+1 < argc) app.random_seed = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
        else if(arg == "--press" && i+1 < argc){
            // e.g. --press Tab@30  or  --press Backspace@200:40 (hold for 40 frames)
            std::string spec = argv[++i];
            size_t at = spec.rfind('@');
            if(at == std::string::npos){ print_usage(argv[0]); return 1; }
            SDL_Keycode key = SDL_GetKeyFromName(spec.substr(0, at).c_str());
            int frame = std::atoi(spec.c_str() + at + 1), hold = 1;
            size_t colon = spec.find(':', at);
            if(colon != std::string::npos) hold = std::max(1, std::atoi(spec.c_str() + colon + 1));
            if(key == SDLK_UNKNOWN){ std::cerr << "Unknown key name in --press " << spec << std::endl; return 1; }
            app.script.push_back({frame, key, true});
            app.script.push_back({frame + hold, key, false});
        }
        else if(arg == "--break" && i+1 < argc) app.debugger.breakpoints.insert((uint16_t)std::strtol(argv[++i], nullptr, 16));
        else if(arg == "--capture" && i+2 < argc){
            app.capture_after = std::atoi(argv[++i]);
            app.forced_capture_path = argv[++i];
        }
        else{
            print_usage(argv[0]);
            return 1;
        }
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Audio setup
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = &app.audio;

    app.audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if(app.audio_device == 0) std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    else{
        std::cout << "Audio: " << have.freq << " Hz, " << (int)have.channels << " channel, " << have.samples << " sample buffer" << std::endl;
        app.audio.sample_rate = have.freq;
        SDL_PauseAudioDevice(app.audio_device, 0);
    }

#ifdef __EMSCRIPTEN__
    const Uint32 window_flags = SDL_WINDOW_SHOWN; // The page's CSS scales the canvas; a resizable window would follow the page size
#else
    const Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    app.window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, window_flags);
    if(!app.window){
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED);
    if(!app.renderer){
        std::cerr << "Renderer error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(app.renderer, WIDTH, HEIGHT); // Scale cleanly when the window is resized
    if(app.debug_open) app.set_debugger_open(true);
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    if(!app.load_rom(argv[1])){
        SDL_DestroyRenderer(app.renderer);
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }

#ifdef __EMSCRIPTEN__
    // The browser owns the loop: it calls us on every display refresh, and we run
    // however many 60Hz emulator frames are due so speed is the same on 60Hz and 144Hz screens.
    web_app = &app;
    emscripten_set_main_loop_arg([](void* arg){
        static double last = emscripten_get_now(), pending = 0.0;
        double now = emscripten_get_now();
        pending += now - last;
        last = now;
        if(pending > 100.0) pending = 1000.0/60; // Tab was in the background; don't fast-forward
        for(int i=0; i<2 && pending >= 1000.0/60; i++){
            ((App*)arg)->tick();
            pending -= 1000.0/60;
        }
    }, &app, 0, 1);
#else
    // Frame pacing uses the high-resolution counter so we get a true 60Hz, not 1000/16 = 62.5Hz
    const uint64_t ticks_per_frame = SDL_GetPerformanceFrequency()/60;
    uint64_t next_frame = SDL_GetPerformanceCounter();
    while(app.running){
        app.tick();

        next_frame += ticks_per_frame;
        uint64_t now = SDL_GetPerformanceCounter();
        if(next_frame > now){
            uint32_t wait_ms = (uint32_t)((next_frame - now)*1000/SDL_GetPerformanceFrequency());
            if(wait_ms > 1) SDL_Delay(wait_ms - 1); // Sleep most of the gap...
            while(SDL_GetPerformanceCounter() < next_frame){} // ...then spin for the last millisecond
        }
        else next_frame = now; // Fell behind; don't try to catch up in a burst
    }
#endif
    if(app.capture_after >= -1 && !app.forced_capture_path.empty()) // Scripted runs report how much sound was produced
        std::cout << "Audio samples played: " << app.audio.samples_played << std::endl;
    if(app.audio_device != 0) SDL_CloseAudioDevice(app.audio_device);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();

    return 0;
}
