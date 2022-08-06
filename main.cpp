#include <vector>
#include <array>
#include <chrono>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include "cpu6502.h"
#include "dis6502.h"

// 1 key toggles TV Type, starts as Color
// 2 key momentaries Reset
// 3 key momentaries Game Type
// 4 key toggles P0 difficulty, starts as A
// 5 key toggles P1 difficulty, starts as A

#include <SDL2/SDL.h>

// 0000-002C TIA (Write)
// 0030-003D TIA (Read)
// 0080-00FF RIOT RAM
// 0280-0297 RIOT I/O, TIMER
// F000-FFFF ROM

namespace Stella
{
    enum {
        ROMbase = 0xF000,
        address_mask = 0x0280,
        RAM_select_value = 0x0080,
        TIA_select_value = 0x0000,
        PIA_select_value = 0x0280,
        RAM_address_mask = 0x007f,

       // TIA write
        CXCLR = 0x002C,
        HMCLR = 0x002B,
        HMOVE = 0x002A,
        RESMP1 = 0x0029,
        RESMP0 = 0x0028,
        VDELBL = 0x0027,
        VDELP1 = 0x0026,
        VDELP0 = 0x0025,
        HMBL = 0x0024,
        HMM1 = 0x0023,
        HMM0 = 0x0022,
        HMP1 = 0x0021,
        HMP0 = 0x0020,
        ENABL = 0x001F,
        ENAM1 = 0x001E,
        ENAM0 = 0x001D,
        GRP1 = 0x001C,
        GRP0 = 0x001B,
        AUDV1 = 0x001A,
        AUDV0 = 0x0019,
        AUDF1 = 0x0018,
        AUDF0 = 0x0017,
        AUDC1 = 0x0016,
        AUDC0 = 0x0015,
        RESBL = 0x0014,
        RESM1 = 0x0013,
        RESM0 = 0x0012,
        RESP1 = 0x0011,
        RESP0 = 0x0010,
        PF2 = 0x000F,
        PF1 = 0x000E,
        PF0 = 0x000D,
        REFP1 = 0x000C,
        REFP0 = 0x000B,
        CTRLPF = 0x000A,
        COLUBK = 0x0009,
        COLUPF = 0x0008,
        COLUP1 = 0x0007,
        COLUP0 = 0x0006,
        NUSIZ1 = 0x0005,
        NUSIZ0 = 0x0004,
        RSYNC = 0x0003,
        WSYNC = 0x0002,
        VBLANK = 0x0001,
        VSYNC = 0x0000,

        VSYNC_SET = 0x02,

        CTRLPF_REFLECT_PLAYFIELD = 0x01,
        CTRLPF_SCORE_MODE = 0x02,
        CTRLPF_PLAYFIELD_ABOVE = 0x04,

       // TIA read
        CXM0P = 0x00,
        CXM1P = 0x01,
        CXP0FB = 0x02,
        CXP1FB = 0x03,
        CXM0FB = 0x04,
        CXM1FB = 0x05,
        CXBLPF = 0x06,
        CXPPMM = 0x07,
        INPT0 = 0x08,
        INPT1 = 0x09,
        INPT2 = 0x0A,
        INPT3 = 0x0B,
        INPT4 = 0x0C,
        INPT5 = 0x0D,

       // PIA
        SWCHA = 0x00,
        SWACNT = 0x01,
        SWCHB = 0x02,
        SWBCNT = 0x03,
        INTIM = 0x04,
        INSTAT = 0x05,
        TIM1T = 0x14,
        TIM8T = 0x15,
        TIM64T = 0x16,
        T1024T = 0x17,

        SWCHA_JOYSTICK0_UP = 0x10,
        SWCHA_JOYSTICK0_DOWN = 0x20,
        SWCHA_JOYSTICK0_LEFT = 0x40,
        SWCHA_JOYSTICK0_RIGHT = 0x80,
        INPT4_JOYSTICK0_BUTTON = 0x80,
        SWCHA_JOYSTICK1_UP = 0x01,
        SWCHA_JOYSTICK1_DOWN = 0x02,
        SWCHA_JOYSTICK1_LEFT = 0x04,
        SWCHA_JOYSTICK1_RIGHT = 0x05,
        INPT5_JOYSTICK1_BUTTON = 0x80,

        SWCHB_RESET_SWITCH = 0x01,
        SWCHB_SELECT_SWITCH = 0x02,
        SWCHB_TVTYPE_SWITCH = 0x08,
        SWCHB_P0_DIFFICULTY_SWITCH = 0x40,
        SWCHB_P1_DIFFICULTY_SWITCH = 0x80,
    };

    static constexpr uint32_t vsync_lines = 3;
    static constexpr uint32_t vblank_lines = 37;
    static constexpr uint32_t visible_line_start = (vsync_lines + vblank_lines);
    static constexpr uint32_t visible_lines = 192;
    static constexpr uint32_t overscan_lines = 30;
    static constexpr uint32_t lines_per_frame = (vsync_lines + vblank_lines + visible_lines + overscan_lines);
    static constexpr uint32_t hblank_pixels = 68;
    static constexpr uint32_t visible_pixels = 160;
    static constexpr uint32_t pixels_per_line = (hblank_pixels + visible_pixels);
    static constexpr uint32_t clocks_per_frame = lines_per_frame * pixels_per_line;

    int get_signed_move(uint8_t HM)
    {
        int motion = HM >> 4U;
        if(motion > 7) {
            motion -= 16;
        }
        return -motion;
    }

};


namespace PlatformInterface
{

uint8_t colu_to_rgb[256][3] = {
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00},
    {0x40, 0x40, 0x40},
    {0x40, 0x40, 0x40},
    {0x6C, 0x6C, 0x6C},
    {0x6C, 0x6C, 0x6C},
    {0x90, 0x90, 0x90},
    {0x90, 0x90, 0x90},
    {0xB0, 0xB0, 0xB0},
    {0xB0, 0xB0, 0xB0},
    {0xC8, 0xC8, 0xC8},
    {0xC8, 0xC8, 0xC8},
    {0xDC, 0xDC, 0xDC},
    {0xDC, 0xDC, 0xDC},
    {0xEC, 0xEC, 0xEC},
    {0xEC, 0xEC, 0xEC},
    {0x44, 0x44, 0x00},
    {0x44, 0x44, 0x00},
    {0x64, 0x64, 0x10},
    {0x64, 0x64, 0x10},
    {0x84, 0x84, 0x24},
    {0x84, 0x84, 0x24},
    {0xA0, 0xA0, 0x34},
    {0xA0, 0xA0, 0x34},
    {0xB8, 0xB8, 0x40},
    {0xB8, 0xB8, 0x40},
    {0xD0, 0xD0, 0x50},
    {0xD0, 0xD0, 0x50},
    {0xE8, 0xE8, 0x5C},
    {0xE8, 0xE8, 0x5C},
    {0xFC, 0xFC, 0x68},
    {0xFC, 0xFC, 0x68},
    {0x70, 0x28, 0x00},
    {0x70, 0x28, 0x00},
    {0x84, 0x44, 0x14},
    {0x84, 0x44, 0x14},
    {0x98, 0x5C, 0x28},
    {0x98, 0x5C, 0x28},
    {0xAC, 0x78, 0x3C},
    {0xAC, 0x78, 0x3C},
    {0xBC, 0x8C, 0x4C},
    {0xBC, 0x8C, 0x4C},
    {0xCC, 0xA0, 0x5C},
    {0xCC, 0xA0, 0x5C},
    {0xDC, 0xB4, 0x68},
    {0xDC, 0xB4, 0x68},
    {0xE8, 0xCC, 0x7C},
    {0xE8, 0xCC, 0x7C},
    {0x84, 0x18, 0x00},
    {0x84, 0x18, 0x00},
    {0x98, 0x34, 0x18},
    {0x98, 0x34, 0x18},
    {0xAC, 0x50, 0x30},
    {0xAC, 0x50, 0x30},
    {0xC0, 0x68, 0x48},
    {0xC0, 0x68, 0x48},
    {0xD0, 0x80, 0x5C},
    {0xD0, 0x80, 0x5C},
    {0xE0, 0x94, 0x70},
    {0xE0, 0x94, 0x70},
    {0xEC, 0xA8, 0x80},
    {0xEC, 0xA8, 0x80},
    {0xFC, 0xBC, 0x94},
    {0xFC, 0xBC, 0x94},
    {0x88, 0x00, 0x00},
    {0x88, 0x00, 0x00},
    {0x9C, 0x20, 0x20},
    {0x9C, 0x20, 0x20},
    {0xB0, 0x3C, 0x3C},
    {0xB0, 0x3C, 0x3C},
    {0xC0, 0x58, 0x58},
    {0xC0, 0x58, 0x58},
    {0xD0, 0x70, 0x70},
    {0xD0, 0x70, 0x70},
    {0xE0, 0x88, 0x88},
    {0xE0, 0x88, 0x88},
    {0xEC, 0xA0, 0xA0},
    {0xEC, 0xA0, 0xA0},
    {0xFC, 0xB4, 0xB4},
    {0xFC, 0xB4, 0xB4},
    {0x78, 0x00, 0x5C},
    {0x78, 0x00, 0x5C},
    {0x8C, 0x20, 0x74},
    {0x8C, 0x20, 0x74},
    {0xA0, 0x3C, 0x88},
    {0xA0, 0x3C, 0x88},
    {0xB0, 0x58, 0x9C},
    {0xB0, 0x58, 0x9C},
    {0xC0, 0x70, 0xB0},
    {0xC0, 0x70, 0xB0},
    {0xD0, 0x84, 0xC0},
    {0xD0, 0x84, 0xC0},
    {0xDC, 0x9C, 0xD0},
    {0xDC, 0x9C, 0xD0},
    {0xEC, 0xB0, 0xE0},
    {0xEC, 0xB0, 0xE0},
    {0x48, 0x00, 0x78},
    {0x48, 0x00, 0x78},
    {0x60, 0x20, 0x90},
    {0x60, 0x20, 0x90},
    {0x78, 0x3C, 0xA4},
    {0x78, 0x3C, 0xA4},
    {0x8C, 0x58, 0xB8},
    {0x8C, 0x58, 0xB8},
    {0xA0, 0x70, 0xCC},
    {0xA0, 0x70, 0xCC},
    {0xB4, 0x84, 0xDC},
    {0xB4, 0x84, 0xDC},
    {0xC4, 0x9C, 0xEC},
    {0xC4, 0x9C, 0xEC},
    {0xD4, 0xB0, 0xFC},
    {0xD4, 0xB0, 0xFC},
    {0x14, 0x00, 0x84},
    {0x14, 0x00, 0x84},
    {0x30, 0x20, 0x98},
    {0x30, 0x20, 0x98},
    {0x4C, 0x3C, 0xAC},
    {0x4C, 0x3C, 0xAC},
    {0x68, 0x58, 0xC0},
    {0x68, 0x58, 0xC0},
    {0x7C, 0x70, 0xD0},
    {0x7C, 0x70, 0xD0},
    {0x94, 0x88, 0xE0},
    {0x94, 0x88, 0xE0},
    {0xA8, 0xA0, 0xEC},
    {0xA8, 0xA0, 0xEC},
    {0xBC, 0xB4, 0xFC},
    {0xBC, 0xB4, 0xFC},
    {0x00, 0x00, 0x88},
    {0x00, 0x00, 0x88},
    {0x1C, 0x20, 0x9C},
    {0x1C, 0x20, 0x9C},
    {0x38, 0x40, 0xB0},
    {0x38, 0x40, 0xB0},
    {0x50, 0x5C, 0xC0},
    {0x50, 0x5C, 0xC0},
    {0x68, 0x74, 0xD0},
    {0x68, 0x74, 0xD0},
    {0x7C, 0x8C, 0xE0},
    {0x7C, 0x8C, 0xE0},
    {0x90, 0xA4, 0xEC},
    {0x90, 0xA4, 0xEC},
    {0xA4, 0xB8, 0xFC},
    {0xA4, 0xB8, 0xFC},
    {0x00, 0x18, 0x7C},
    {0x00, 0x18, 0x7C},
    {0x1C, 0x38, 0x90},
    {0x1C, 0x38, 0x90},
    {0x38, 0x54, 0xA8},
    {0x38, 0x54, 0xA8},
    {0x50, 0x70, 0xBC},
    {0x50, 0x70, 0xBC},
    {0x68, 0x88, 0xCC},
    {0x68, 0x88, 0xCC},
    {0x7C, 0x9C, 0xDC},
    {0x7C, 0x9C, 0xDC},
    {0x90, 0xB4, 0xEC},
    {0x90, 0xB4, 0xEC},
    {0xA4, 0xC8, 0xFC},
    {0xA4, 0xC8, 0xFC},
    {0x00, 0x2C, 0x5C},
    {0x00, 0x2C, 0x5C},
    {0x1C, 0x4C, 0x78},
    {0x1C, 0x4C, 0x78},
    {0x38, 0x68, 0x90},
    {0x38, 0x68, 0x90},
    {0x50, 0x84, 0xAC},
    {0x50, 0x84, 0xAC},
    {0x68, 0x9C, 0xC0},
    {0x68, 0x9C, 0xC0},
    {0x7C, 0xB4, 0xD4},
    {0x7C, 0xB4, 0xD4},
    {0x90, 0xCC, 0xE8},
    {0x90, 0xCC, 0xE8},
    {0xA4, 0xE0, 0xFC},
    {0xA4, 0xE0, 0xFC},
    {0x00, 0x40, 0x2C},
    {0x00, 0x40, 0x2C},
    {0x1C, 0x5C, 0x48},
    {0x1C, 0x5C, 0x48},
    {0x38, 0x7C, 0x64},
    {0x38, 0x7C, 0x64},
    {0x50, 0x9C, 0x80},
    {0x50, 0x9C, 0x80},
    {0x68, 0xB4, 0x94},
    {0x68, 0xB4, 0x94},
    {0x7C, 0xD0, 0xAC},
    {0x7C, 0xD0, 0xAC},
    {0x90, 0xE4, 0xC0},
    {0x90, 0xE4, 0xC0},
    {0xA4, 0xFC, 0xD4},
    {0xA4, 0xFC, 0xD4},
    {0x00, 0x3C, 0x00},
    {0x00, 0x3C, 0x00},
    {0x20, 0x5C, 0x20},
    {0x20, 0x5C, 0x20},
    {0x40, 0x7C, 0x40},
    {0x40, 0x7C, 0x40},
    {0x5C, 0x9C, 0x5C},
    {0x5C, 0x9C, 0x5C},
    {0x74, 0xB4, 0x74},
    {0x74, 0xB4, 0x74},
    {0x8C, 0xD0, 0x8C},
    {0x8C, 0xD0, 0x8C},
    {0xA4, 0xE4, 0xA4},
    {0xA4, 0xE4, 0xA4},
    {0xB8, 0xFC, 0xB8},
    {0xB8, 0xFC, 0xB8},
    {0x14, 0x38, 0x00},
    {0x14, 0x38, 0x00},
    {0x34, 0x5C, 0x1C},
    {0x34, 0x5C, 0x1C},
    {0x50, 0x7C, 0x38},
    {0x50, 0x7C, 0x38},
    {0x6C, 0x98, 0x50},
    {0x6C, 0x98, 0x50},
    {0x84, 0xB4, 0x68},
    {0x84, 0xB4, 0x68},
    {0x9C, 0xCC, 0x7C},
    {0x9C, 0xCC, 0x7C},
    {0xB4, 0xE4, 0x90},
    {0xB4, 0xE4, 0x90},
    {0xC8, 0xFC, 0xA4},
    {0xC8, 0xFC, 0xA4},
    {0x2C, 0x30, 0x00},
    {0x2C, 0x30, 0x00},
    {0x4C, 0x50, 0x1C},
    {0x4C, 0x50, 0x1C},
    {0x68, 0x70, 0x34},
    {0x68, 0x70, 0x34},
    {0x84, 0x8C, 0x4C},
    {0x84, 0x8C, 0x4C},
    {0x9C, 0xA8, 0x64},
    {0x9C, 0xA8, 0x64},
    {0xB4, 0xC0, 0x78},
    {0xB4, 0xC0, 0x78},
    {0xCC, 0xD4, 0x88},
    {0xCC, 0xD4, 0x88},
    {0xE0, 0xEC, 0x9C},
    {0xE0, 0xEC, 0x9C},
    {0x44, 0x28, 0x00},
    {0x44, 0x28, 0x00},
    {0x64, 0x48, 0x18},
    {0x64, 0x48, 0x18},
    {0x84, 0x68, 0x30},
    {0x84, 0x68, 0x30},
    {0xA0, 0x84, 0x44},
    {0xA0, 0x84, 0x44},
    {0xB8, 0x9C, 0x58},
    {0xB8, 0x9C, 0x58},
    {0xD0, 0xB4, 0x6C},
    {0xD0, 0xB4, 0x6C},
    {0xE8, 0xCC, 0x7C},
    {0xE8, 0xCC, 0x7C},
    {0xFC, 0xE0, 0x8C},
    {0xFC, 0xE0, 0x8C},
};

void create_colormap()
{
    // generated table from an image
    // may one day convert color and luminance to rgb with math
}

static constexpr int SCREEN_SCALE = 2;

uint8_t SWCHB_value =
    Stella::SWCHB_RESET_SWITCH | 
    Stella::SWCHB_SELECT_SWITCH | 
    Stella::SWCHB_TVTYPE_SWITCH;

uint8_t ReadConsoleSwitches()
{
    return SWCHB_value;
}

// SWCHA and then player0button and player1button
uint8_t SWCHA_value;
uint8_t player0button; // as shows up in INPT4, 0x00 if pressed, 0x80 if not pressed.
uint8_t player1button; // as shows up in INPT5, 0x00 if pressed, 0x80 if not pressed.
std::tuple<uint8_t, bool, bool> ReadJoysticks()
{
    return std::make_tuple(SWCHA_value, player0button, player1button);
}

SDL_AudioDeviceID audio_device;
bool audio_needs_start = true;
SDL_AudioFormat actual_audio_format;

void EnqueueStereoU8AudioSamples(uint8_t *buf, size_t sz)
{
    if(audio_needs_start) {
        audio_needs_start = false;
        SDL_PauseAudioDevice(audio_device, 0);
        /* give a little data to avoid gaps and to avoid a pop */
        std::array<uint8_t, 2048> lead_in;
        size_t sampleCount = lead_in.size() / 2;
        for(int i = 0; i < sampleCount; i++) {
            lead_in[i * 2 + 0] = 128 + (buf[0] - 128) * i / sampleCount;
            lead_in[i * 2 + 1] = 128 + (buf[0] - 128) * i / sampleCount;
        }
        SDL_QueueAudio(audio_device, lead_in.data(), lead_in.size());
    }

    if(actual_audio_format == AUDIO_U8) {
        SDL_QueueAudio(audio_device, buf, sz);
    }
}


SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *surface;

std::chrono::time_point<std::chrono::system_clock> previous_event_time;

void Start(uint32_t& stereoU8SampleRate, size_t& preferredAudioBufferSizeBytes)
{
    create_colormap();

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("Atari 2600", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 228 * 2 * SCREEN_SCALE, 262 * SCREEN_SCALE, SDL_WINDOW_RESIZABLE);
    if(!window) {
        printf("could not open window\n");
        exit(1);
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer) {
        printf("could not create renderer\n");
        exit(1);
    }
    surface = SDL_CreateRGBSurface(0, 228 * 2, 262, 24, 0, 0, 0, 0);
    if(!surface) {
        printf("could not create surface\n");
        exit(1);
    }

    SDL_AudioSpec audiospec{0};
    audiospec.freq = 44100;
    audiospec.format = AUDIO_U8;
    audiospec.channels = 2;
    audiospec.samples = 1024; // audiospec.freq / 100;
    audiospec.callback = nullptr;
    SDL_AudioSpec obtained;

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &audiospec, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE); // | SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    assert(audio_device > 0);
    assert(obtained.channels == audiospec.channels);

    switch(obtained.format) {
        case AUDIO_U8: {
            /* okay, native format */
            break;
        }
        default:
            printf("unknown audio format chosen: %X\n", obtained.format);
            exit(1);
    }

    stereoU8SampleRate = obtained.freq;
    preferredAudioBufferSizeBytes = obtained.samples * 2;
    actual_audio_format = obtained.format;

    SDL_PumpEvents();

    previous_event_time = std::chrono::system_clock::now();
}

enum {
    SWITCH_TV_TYPE,
    SWITCH_P0_DIFFICULTY,
    SWITCH_P1_DIFFICULTY,
    SWITCH_GAME_TYPE,
    SWITCH_RESET,
};

static void HandleEvents(void)
{
    using namespace Stella;
    static bool switch_tv_type = true; // true = color
    static bool switch_p0_difficulty = false; // true = color
    static bool switch_p1_difficulty = false;
    static bool shift_pressed = false;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_WINDOWEVENT:
                printf("window event %d\n", event.window.event);
                switch(event.window.event) {
                }
                break;
            case SDL_QUIT:
                // event_queue.push_back({QUIT, 0});
                exit(0);
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_RSHIFT:
                    case SDL_SCANCODE_LSHIFT:
                        shift_pressed = true;
                        break;
                    case SDL_SCANCODE_W:
                        SWCHA_value &= (~Stella::SWCHA_JOYSTICK0_UP);
                        break;
                    case SDL_SCANCODE_S:
                        SWCHA_value &= (~Stella::SWCHA_JOYSTICK0_DOWN);
                        break;
                    case SDL_SCANCODE_A:
                        SWCHA_value &= (~Stella::SWCHA_JOYSTICK0_LEFT);
                        break;
                    case SDL_SCANCODE_D:
                        SWCHA_value &= (~Stella::SWCHA_JOYSTICK0_RIGHT);
                        break;
                    case SDL_SCANCODE_SPACE:
                        player0button = (uint8_t)~Stella::INPT4_JOYSTICK0_BUTTON;
                        break;
                    case SDL_SCANCODE_1:
                        switch_tv_type = !switch_tv_type;
                        if(switch_tv_type) {
                            SWCHB_value |= SWCHB_TVTYPE_SWITCH;
                        } else {
                            SWCHB_value &= ~SWCHB_TVTYPE_SWITCH;
                        }
                        break;
                    case SDL_SCANCODE_2:
                        SWCHB_value &= ~SWCHB_RESET_SWITCH;
                        break;
                    case SDL_SCANCODE_3:
                        SWCHB_value &= ~SWCHB_SELECT_SWITCH;
                        break;
                    case SDL_SCANCODE_4:
                        switch_p0_difficulty = !switch_p0_difficulty;
                        if(switch_p0_difficulty) {
                            SWCHB_value |= SWCHB_P0_DIFFICULTY_SWITCH;
                        } else {
                            SWCHB_value &= ~SWCHB_P0_DIFFICULTY_SWITCH;
                        }
                        break;
                    case SDL_SCANCODE_5:
                        switch_p1_difficulty = !switch_p1_difficulty;
                        if(switch_p1_difficulty) {
                            SWCHB_value |= SWCHB_P1_DIFFICULTY_SWITCH;
                        } else {
                            SWCHB_value &= ~SWCHB_P1_DIFFICULTY_SWITCH;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_RSHIFT:
                    case SDL_SCANCODE_LSHIFT:
                        shift_pressed = false;
                        break;
                    case SDL_SCANCODE_2:
                        SWCHB_value |= SWCHB_RESET_SWITCH;
                        break;
                    case SDL_SCANCODE_3:
                        SWCHB_value |= SWCHB_SELECT_SWITCH;
                        break;
                    case SDL_SCANCODE_W:
                        SWCHA_value |= Stella::SWCHA_JOYSTICK0_UP;
                        break;
                    case SDL_SCANCODE_S:
                        SWCHA_value |= Stella::SWCHA_JOYSTICK0_DOWN;
                        break;
                    case SDL_SCANCODE_A:
                        SWCHA_value |= Stella::SWCHA_JOYSTICK0_LEFT;
                        break;
                    case SDL_SCANCODE_D:
                        SWCHA_value |= Stella::SWCHA_JOYSTICK0_RIGHT;
                        break;
                    case SDL_SCANCODE_SPACE:
                        player0button = Stella::INPT4_JOYSTICK0_BUTTON;
                        break;
                    case SDL_SCANCODE_RETURN:
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

void Frame(const uint8_t* screen, [[maybe_unused]] float megahertz)
{
    using namespace std::chrono_literals;

    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

    uint8_t* framebuffer = reinterpret_cast<uint8_t*>(surface->pixels);

    for(int y = 0; y < 262; y++) {
        for(int x = 0; x < 228; x++) {
            uint8_t *pixel = framebuffer + 3 * (x * 2 + y * 228 * 2);
            uint8_t *rgb = colu_to_rgb[screen[x + y * 228]];
            pixel[0] = rgb[2];
            pixel[1] = rgb[1];
            pixel[2] = rgb[0];
            pixel[3] = rgb[2];
            pixel[4] = rgb[1];
            pixel[5] = rgb[0];
        }
    }

    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed;
    
    elapsed = now - previous_event_time;
    if(elapsed.count() > .05) {
        HandleEvents();
        previous_event_time = now;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(!texture) {
        printf("could not create texture\n");
        exit(1);
    }
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
}

};

uint8_t screen[228 * 262];

void set_colu(int x, int y, uint8_t colu)
{
    using namespace Stella;
    screen[x + y * pixels_per_line] = colu;
}

#if 0
void write_screen()
{
    using namespace Stella;

    static int frame_number = 0;
    static char filename[512];

    sprintf(filename, "image%05d.ppm", frame_number);
    FILE *screenfile = fopen(filename, "wb");
    fprintf(screenfile, "P6 %d %d 255\n", pixels_per_line * 2, lines_per_frame);
    for(int y = 0; y < lines_per_frame; y++) {
        for(int x = 0; x < pixels_per_line; x++) {
            uint8_t colu = screen[x + y * pixels_per_line];
            uint8_t *rgb = colu_to_rgb[colu];
            fwrite(rgb, 3, 1, screenfile);
            fwrite(rgb, 3, 1, screenfile);
        }
    }
    fclose(screenfile);
    printf("wrote image %s\n", filename);

    frame_number++;

    memset(screen, 0x4F, visible_pixels * visible_lines);
}
#endif

typedef uint64_t clk_t;

uint32_t pixel_clock_within_frame = 0;

uint8_t current_horizontal_clock(int if_in_hblank)
{
    using namespace Stella;
    // Could use a value incremented and reset from advance_one_pixel

    uint32_t pixel_within_line = pixel_clock_within_frame % pixels_per_line;

    return (pixel_within_line < 68) ? if_in_hblank : (pixel_within_line - 68);
}

struct sysclock // When I called this "clock" XCode errored out because I shadowed MacOSX's "clock"
{
    clk_t clock = 0;
    operator clk_t() const { return clock; }
    void add_pixel_cycles(int N) 
    {
        clock += N;
    }
    void add_cpu_cycles(int N) 
    {
        clock += N * 3;
    }
};

struct stella 
{
    enum {
        DEBUG_TIA = 0x0001,
        DEBUG_TIMER = 0x0002,
        DEBUG_PIA = 0x0004,
        DEBUG_RAM = 0x0008,
    };
    static constexpr uint32_t debug = 0;

    std::array<uint8_t, 128> RAM;
    std::vector<uint8_t> ROM;
    uint16_t ROM_address_mask;
    sysclock& clk;

    uint8_t mobP0 = 0;
    uint8_t mobP1 = 0;
    uint8_t mobM0 = 0;
    uint8_t mobM1 = 0;
    uint8_t mobBL = 0;

    uint32_t interval_timer = 0;
    uint32_t interval_timer_value = 0;
    uint32_t interval_timer_prescaler = 1;
    clk_t previous_interval_timer = 0;
    bool timer_underflow = false;

    void set_interval_timer(int prescaler, uint8_t value)
    {
        interval_timer_prescaler = prescaler;
        interval_timer_value = value * prescaler;
        if(value == 0) {
            interval_timer = 0xFF * prescaler;
        } else {
            interval_timer = (value - 1) * prescaler;
        }
    }

    void advance_interval_timer(sysclock& clk)
    {
        // could redo without loop
        for(clk_t c = previous_interval_timer; c < clk; c++) {
            if(interval_timer == 0) {
                timer_underflow = true;
                interval_timer = 0xFF * interval_timer_prescaler;
            } else {
                interval_timer--;
            }
        }
        if(debug & DEBUG_TIMER) { printf("timer now %d\n", interval_timer); }
        previous_interval_timer = clk;
    }

    uint8_t tia_write[64];
    uint8_t tia_read[64];
    bool wait_for_hsync = false;

    stella(const std::vector<uint8_t>& ROM, sysclock& clock) :
        ROM(std::move(ROM)),
        clk(clock)
    {
        if(ROM.size() == 0x800) {
            ROM_address_mask = 0x7ff;
        } else if(ROM.size() == 0x1000) {
            ROM_address_mask = 0xfff;
        } else {
            std::cout << "dunno about ROM size " << ROM.size() << "\n";
            abort();
        }
    }

    bool isPIA(uint16_t addr)
    {
        using namespace Stella;
        return (addr & address_mask) == PIA_select_value;
    }

    bool isTIA(uint16_t addr)
    {
        using namespace Stella;
        return (addr & address_mask) == TIA_select_value;
    }

    bool isRAM(uint16_t addr)
    {
        using namespace Stella;
        return (addr & address_mask) == RAM_select_value;
    }
    uint8_t read(uint16_t addr)
    {
        using namespace Stella;
        if(addr >= ROMbase) {
            uint8_t data = ROM.at(addr & ROM_address_mask);
            // printf("read %02X from ROM %04X\n", data, addr);
            return data;
        } else if(isRAM(addr)) {
            uint8_t data = RAM.at(addr & RAM_address_mask);
            // printf("read %02X from RAM %04X\n", data, addr);
            return data;
        } else if(isTIA(addr)) {
            if(debug & DEBUG_TIA) { printf("read from TIA %04X\n", addr); }
            addr &= 0xF;
            if(addr == INPT5) {
                // read latched or unlatched input port 5
                uint8_t inpt5 = 0;
                uint8_t swcha, player0button, player1button;
                std::tie(swcha, player0button, player1button) = PlatformInterface::ReadJoysticks();
                inpt5 |= player1button;
                return inpt5;
            } else if(addr == INPT4) {
                // read latched or unlatched input port 4
                uint8_t inpt4 = 0;
                uint8_t swcha, player0button, player1button;
                std::tie(swcha, player0button, player1button) = PlatformInterface::ReadJoysticks();
                inpt4 |= player0button;
                return inpt4;
            } else if(addr == INPT3) {
                // read latched or unlatched input port 3
                return 0x0;
            } else if(addr == INPT2) {
                // read latched or unlatched input port 2
                return 0x0;
            } else if(addr == INPT1) {
                // read latched or unlatched input port 1
                return 0x0;
            } else if(addr == INPT0) {
                // read latched or unlatched input port 0
                return 0x0;
            } else if(addr == CXM0P) {
                printf("read collision flags for missile 0 and players\n");
                return 0x0;
            } else if(addr == CXM1P) {
                printf("read collision flags for missile 1 and players\n");
                return 0x0;
            } else if(addr == CXP0FB) {
                printf("read collision flags for player 0 and playfield and ball\n");
                return 0x0;
            } else if(addr == CXP1FB) {
                printf("read collision flags for player 1 and playfield and ball\n");
                return 0x0;
            } else if(addr == CXM0FB) {
                printf("read collision flags for missile 0 and playfield and ball\n");
                return 0x0;
            } else if(addr == CXM1FB) {
                printf("read collision flags for missile 1 and playfield and ball\n");
                return 0x0;
            } else if(addr == CXBLPF) {
                printf("read collision flags for ball and playfield\n");
                return 0x0;
            } else if(addr == CXPPMM) {
                printf("read collision flags for player collision and missile collision\n");
                return 0x0;
            }
        } else if(isPIA(addr)) {
            if(debug & DEBUG_PIA) { printf("read from PIA %04X\n", addr); }
            addr &= 0x1F;
            if(addr == SWCHB) {
                return PlatformInterface::ReadConsoleSwitches();
            } else if(addr == INTIM) {
                uint8_t data = interval_timer / interval_timer_prescaler;
                if(debug & DEBUG_TIMER) { printf("read interval timer, %2X\n", data); }
                return data;
            } else if(addr == SWCHA) {
                uint8_t swcha, player0button, player1button;
                std::tie(swcha, player0button, player1button) = PlatformInterface::ReadJoysticks();
                return swcha;
            } else {
                printf("unhandled read from PIA %04X\n", addr);
                abort();
            }
        }
        printf("unhandled read from %04X\n", addr);
        abort();
    }

    void write(uint16_t addr, uint8_t data)
    {
        using namespace Stella;
        if(isRAM(addr)) {
            RAM[addr & RAM_address_mask] = data;
            if(debug & DEBUG_RAM) { printf("wrote %02X to RAM %04X\n", data, addr); }
        } else if(isPIA(addr)) {
            printf("wrote %02X to PIA %04X\n", data, addr);
            addr &= 0x1F;
            if(addr == TIM1T) {
                set_interval_timer(1, data);
            } else if(addr == TIM8T) {
                interval_timer = interval_timer_value = data * 8;
                interval_timer_prescaler = 8;
            } else if(addr == TIM64T) {
                interval_timer = interval_timer_value = data * 64;
                interval_timer_prescaler = 64;
            } else if(addr == T1024T) {
                interval_timer = interval_timer_value = data * 1024;
                interval_timer_prescaler = 1024;
            }
            // XXX TODO
        } else if(isTIA(addr)) {
            addr &= 0x3F;
            if(addr == VSYNC) {
                if(debug & DEBUG_TIA) { printf("wrote %02X to VSYNC\n", data); }
                if(data & VSYNC_SET) {
                    pixel_clock_within_frame = 0;
                    // write_screen();
                    PlatformInterface::Frame(screen, 1.0f);
                }
            } else if(addr == CXCLR) {
                printf("wrote %02X to CXCLR\n", data);
                // reset collision latches
            } else if(addr == HMCLR) {
                // Reset all 5 HM registers to 0
                tia_write[HMBL] = 0;
                tia_write[HMM1] = 0;
                tia_write[HMM0] = 0;
                tia_write[HMP1] = 0;
                tia_write[HMP0] = 0;
            } else if(addr == HMOVE) {
                // Apply the HM variables, move players, missles, and ball
                // printf("HMOVE: mobP0 was %d, signed move %d, becomes %d\n", mobP0, get_signed_move(tia_write[HMP0]), (mobP0 + get_signed_move(tia_write[HMP0]) + 160) % 160);
                mobP0 = (mobP0 + get_signed_move(tia_write[HMP0]) + 160) % 160;
                mobP1 = (mobP1 + get_signed_move(tia_write[HMP1]) + 160) % 160;
                mobM0 = (mobM0 + get_signed_move(tia_write[HMM0]) + 160) % 160;
                mobM1 = (mobM1 + get_signed_move(tia_write[HMM1]) + 160) % 160;
                mobBL = (mobBL + get_signed_move(tia_write[HMBL]) + 160) % 160;
            } else if(addr == RESMP1) {
                // XXX handle hid/lock bit
                mobM0 = mobP0;
            } else if(addr == RESMP0) {
                // XXX handle hid/lock bit
                mobM1 = mobP1;
            } else if(addr == VDELBL) {
                tia_write[VDELBL] = data;
            } else if(addr == VDELP1) {
                tia_write[VDELP1] = data;
            } else if(addr == VDELP0) {
                tia_write[VDELP0] = data;
            } else if(addr == HMBL) {
                tia_write[HMBL] = data;
            } else if(addr == HMM1) {
                tia_write[HMM1] = data;
            } else if(addr == HMM0) {
                tia_write[HMM0] = data;
            } else if(addr == HMP1) {
                tia_write[HMP1] = data;
            } else if(addr == HMP0) {
                tia_write[HMP0] = data;
            } else if(addr == ENABL) {
                // XXX when writing, might need to check VDELBL and hold this until GRP1 is set
                tia_write[ENABL] = data;
            } else if(addr == ENAM1) {
                tia_write[ENAM1] = data;
            } else if(addr == ENAM0) {
                tia_write[ENAM0] = data;
            } else if(addr == GRP1) {
                // XXX when writing, might need to check VDELP1 and hold this until GRP0 is set
                tia_write[GRP1] = data;
            } else if(addr == GRP0) {
                // XXX when writing, might need to check VDELP0 and hold this until GRP1 is set
                tia_write[GRP0] = data;
            } else if(addr == AUDV1) {
                // audio control 0 skip
            } else if(addr == AUDV0) {
                // audio control 0 skip
            } else if(addr == AUDF1) {
                // audio control 0 skip
            } else if(addr == AUDF0) {
                // audio control 0 skip
            } else if(addr == AUDC1) {
                // audio control 0 skip
            } else if(addr == AUDC0) {
                // audio control 0 skip
            } else if(addr == RESBL) {
                mobBL = current_horizontal_clock(2);
            } else if(addr == RESM1) {
                mobM1 = current_horizontal_clock(2);
            } else if(addr == RESM0) { 
                mobM0 = current_horizontal_clock(2);
            } else if(addr == RESP1) {
                mobP1 = current_horizontal_clock(3);
                // printf("RESP1: hclk %d, becomes %d\n", current_horizontal_clock(3), mobP1);
            } else if(addr == RESP0) {
                mobP0 = current_horizontal_clock(3);
                // printf("RESP0: hclk %d, becomes %d\n", current_horizontal_clock(3), mobP0);
            } else if(addr == PF2) {
                tia_write[PF2] = data;
            } else if(addr == PF1) {
                tia_write[PF1] = data;
            } else if(addr == PF0) {
                tia_write[PF0] = data;
            } else if(addr == REFP1) {
                tia_write[REFP1] = data;
            } else if(addr == REFP0) {
                tia_write[REFP0] = data;
            } else if(addr == CTRLPF) {
                tia_write[CTRLPF] = data;
            } else if(addr == COLUBK) {
                tia_write[COLUBK] = data;
            } else if(addr == COLUPF) {
                tia_write[COLUPF] = data;
            } else if(addr == COLUP1) {
                tia_write[COLUP1] = data;
            } else if(addr == COLUP0) {
                tia_write[COLUP0] = data;
            } else if(addr == NUSIZ0) {
                tia_write[NUSIZ0] = data;
            } else if(addr == NUSIZ1) {
                tia_write[NUSIZ1] = data;
            } else if(addr == RSYNC) {
                // printf("write %d to RSYNC\n", data); 
            } else if(addr == WSYNC) {
                // printf("write %d to WSYNC\n", data); 
                wait_for_hsync = true;
            } else if(addr == VBLANK) {
                // printf("write %d to VBLANK\n", data); 
            } else if(addr == 0x2D) {
                // ignore?
            } else if(addr == 0x2E) {
                // ignore?
            } else if(addr == 0x2F) {
                // ignore?
            } else if((addr >= 0x30) && (addr <= 0x3F)) {
                // ignore?
            }
        } else {
            printf("unhandled write of %02X to %04X\n", data, addr);
            abort();
        }
    }

    int get_playfield_bit(int x)
    {
        using namespace Stella;
        int playfield_bit_number = x / 4;
        if(playfield_bit_number >= 20) {
            if(tia_write[CTRLPF] & CTRLPF_REFLECT_PLAYFIELD) {
                playfield_bit_number = 39 - playfield_bit_number;
            } else {
                playfield_bit_number = playfield_bit_number - 20;
            }
        }
        if(playfield_bit_number < 4) {
            return (tia_write[PF0] >> (4 + playfield_bit_number)) & 0x01;
        }
        if(playfield_bit_number < 12) {
            return (tia_write[PF1] >> (11 - playfield_bit_number)) & 0x01;
        }
        return (tia_write[PF2] >> (playfield_bit_number - 12)) & 0x01;
    }
    
    int get_player_bit(int x, uint8_t mob, uint8_t grp, uint8_t nusiz, uint8_t refp)
    {
        int bit;
        // printf("get_player_bit(%d, %d, %02X, %02X, %02X)\n", x, mob, grp, nusiz, refp);

        switch(nusiz & 0x7) {
            case 0: /* X......... */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else  {
                    return 0;
                }
                break;
            case 1: /* X.Y....... */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else if(x >= mob + 16 && x < mob + 16 + 8) {
                    bit = x - 16 - mob;
                } else  {
                    return 0;
                }
                break;
            case 2: /* X...Y..... */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else if(x >= mob + 32 && x < mob + 32 + 8) {
                    bit = x - 32 - mob;
                } else  {
                    return 0;
                }
                break;
            case 3: /* X.Y.Z..... */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else if(x >= mob + 16 && x < mob + 16 + 8) {
                    bit = x - 16 - mob;
                } else if(x >= mob + 32 && x < mob + 32 + 8) {
                    bit = x - 32 - mob;
                } else  {
                    return 0;
                }
                break;
            case 4: /* X.......Y. */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else if(x >= mob + 64 && x < mob + 64 + 8) {
                    bit = x - 64 - mob;
                } else  {
                    return 0;
                }
                break;
            case 5: /* XX........ */
                if(x >= mob && x < mob + 16) {
                    bit = (x - mob) / 2;
                } else  {
                    return 0;
                }
                break;
            case 6: /* X...Y...Z. */
                if(x >= mob && x < mob + 8) {
                    bit = x - mob;
                } else if(x >= mob + 32 && x < mob + 32 + 8) {
                    bit = x - 16 - mob;
                } else if(x >= mob + 64 && x < mob + 64 + 8) {
                    bit = x - 32 - mob;
                } else  {
                    return 0;
                }
                break;
            case 7: /* XXXX...... */
                if(x >= mob && x < mob + 16) {
                    bit = (x - mob) / 4;
                } else  {
                    return 0;
                }
                break;
        }

        if(!refp) {
            bit = 7 - bit;
        }

        // printf("bit = %d\n", bit);
        return (grp >> bit) & 1;
    }

    uint8_t process_TIA_to_pixel(int x)
    {
        using namespace Stella;
        uint8_t color = tia_write[COLUBK];
        if(x < hblank_pixels) {
            return color;
        }
        x -= hblank_pixels;
        int pf = get_playfield_bit(x);
        // XXX playfield and ball can be over players and missles if CTRLPF & 0x4, so need to handle that later
        if(pf) {
            color = tia_write[COLUPF];
        }
        if(tia_write[GRP0] != 0) {
            int p0 = get_player_bit(x, mobP0, tia_write[GRP0], tia_write[NUSIZ0], tia_write[REFP0]);
            if(p0) {
                color = tia_write[COLUP0];
            }
        }
        if(tia_write[GRP1] != 0) {
            int p1 = get_player_bit(x, mobP1, tia_write[GRP1], tia_write[NUSIZ1], tia_write[REFP1]);
            if(p1) {
                color = tia_write[COLUP1];
            }
        }
        // XXX process rest of registers
        return color;
    }

};

clk_t last_pixel_clocked;

// Must be called once and only once for every clock value
// I.e. clk must be incrementing on each call
bool advance_one_pixel(stella &hw)
{
    using namespace Stella;
    uint32_t line_within_frame = (pixel_clock_within_frame / pixels_per_line);
    uint32_t pixel_within_line = pixel_clock_within_frame % pixels_per_line;

    bool hblank_started = (pixel_within_line == 0);

    int color = hw.process_TIA_to_pixel(pixel_within_line);

    set_colu(pixel_within_line, line_within_frame, color);

    pixel_clock_within_frame++;
    if(pixel_clock_within_frame >= clocks_per_frame) {
        pixel_clock_within_frame = 0;
    }

    return hblank_started;
}

void scanout_to_current_clock(const sysclock& clk, stella &hw)
{
    using namespace Stella;
    for(clk_t c = last_pixel_clocked; c < clk; c++) {
        advance_one_pixel(hw);
    }
    last_pixel_clocked = clk;
}

clk_t scanout_to_hsync(const sysclock& clk, stella &hw)
{
    using namespace Stella;
    bool hsync_started;
    do {
        hsync_started = advance_one_pixel(hw);
    } while(!hsync_started);
    auto clocks = last_pixel_clocked - clk;
    last_pixel_clocked = clk;
    return clocks;
}

std::string read_bus_and_disassemble(stella &hw, int pc)
{
    int bytes;
    std::string dis;
    uint8_t buf[4];
    buf[0] = hw.read(pc + 0);
    buf[1] = hw.read(pc + 1);
    buf[2] = hw.read(pc + 2);
    buf[3] = hw.read(pc + 3);
    std::tie(bytes, dis) = disassemble_6502(pc, buf);
    return dis;
}

int main(int argc, char **argv)
{
    uint32_t stereoU8SampleRate;
    size_t preferredAudioBufferSizeBytes;
    PlatformInterface::Start(stereoU8SampleRate, preferredAudioBufferSizeBytes);

    FILE *ROMfile = fopen(argv[1], "rb");
    if(ROMfile == nullptr) {
        std::cerr << "couldn't open " << argv[1] << " for reading.\n";
        exit(EXIT_FAILURE);
    }
    fseek(ROMfile, 0, SEEK_END);
    long length = ftell(ROMfile);
    fseek(ROMfile, 0, SEEK_SET);
    std::vector<uint8_t> ROM;
    ROM.resize(length);
    fread(ROM.data(), length, 1, ROMfile);

    sysclock clk;
    stella hw(ROM, clk);
    CPU6502 cpu(clk, hw);
    cpu.reset();
    while(1) {
        std::string dis = read_bus_and_disassemble(hw, cpu.pc);
        // printf("%s\n", dis.c_str());
        cpu.cycle();
        if(hw.wait_for_hsync) {
            auto cycles = scanout_to_hsync(clk, hw);
            clk.add_pixel_cycles(cycles);
            hw.wait_for_hsync = false;
        } else {
            scanout_to_current_clock(clk, hw);
        }
        hw.advance_interval_timer(clk);
    }
}
