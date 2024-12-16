#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <SDL2/SDL.h>

#define EMULATE_65C02 0
#include "cpu6502.h"
#include "dis6502.h"

#include "stella.h"

// 1 key toggles TV Type, starts as Color
// 2 key momentaries Reset
// 3 key momentaries Game Type
// 4 key toggles P0 difficulty, starts as A
// 5 key toggles P1 difficulty, starts as A

// 0000-002C TIA (Write)
// 0030-003D TIA (Read)
// 0080-00FF RIOT RAM
// 0280-0297 RIOT I/O, TIMER
// F000-FFFF ROM

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
// The joystick values are set when not pressed
uint8_t SWCHA_value =
    Stella::SWCHA_JOYSTICK0_UP | Stella::SWCHA_JOYSTICK0_DOWN | Stella::SWCHA_JOYSTICK0_LEFT | Stella::SWCHA_JOYSTICK0_RIGHT |
    Stella::SWCHA_JOYSTICK1_UP | Stella::SWCHA_JOYSTICK1_DOWN | Stella::SWCHA_JOYSTICK1_LEFT | Stella::SWCHA_JOYSTICK1_RIGHT;
uint8_t player0button = 0x80; // as shows up in INPT4, 0x00 if pressed, 0x80 if not pressed.
uint8_t player1button = 0x80; // as shows up in INPT5, 0x00 if pressed, 0x80 if not pressed.
std::tuple<uint8_t, uint8_t, uint8_t> ReadJoysticks()
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
        std::array<uint8_t, 256> lead_in;
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
std::chrono::time_point<std::chrono::system_clock> previous_frame_time;

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
    assert(obtained.format == audiospec.format);

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
    previous_frame_time = std::chrono::system_clock::now();
}

static void HandleEvents(void)
{
    using namespace Stella;
    static bool switch_tv_type = true; // true = color
    static bool switch_p0_difficulty = false;
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

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed;

    elapsed = now - previous_frame_time;
    // printf("elapsed.count() == %f\n", elapsed.count());
    static constexpr long long minimum_frame_micros = 15000; // Hand-tuned...

    while(elapsed < std::chrono::microseconds(minimum_frame_micros)) {
        std::this_thread::sleep_for(std::chrono::microseconds(minimum_frame_micros) - elapsed); // XXX either skip until .05 or do this sleep
        elapsed = std::chrono::system_clock::now() - previous_frame_time;
    }
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

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

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    // printf("Draw frame\n");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(!texture) {
        printf("could not create texture\n");
        exit(1);
    }
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
    previous_frame_time = std::chrono::system_clock::now();

    now = std::chrono::system_clock::now();
    elapsed = now - previous_event_time;
    if(elapsed.count() > .05) {
        HandleEvents();
        previous_event_time = now;
    }

}

};

#if 0
void write_screen()
{
    using namespace Stella;

    static int frame_number = 0;
    static char filename[512];

    sprintf(filename, "image%05d.ppm", frame_number);
    FILE *screenfile = fopen(filename, "wb");
    fprintf(screenfile, "P6 %d %d 255\n", clocks_per_line * 2, lines_per_frame);
    for(int y = 0; y < lines_per_frame; y++) {
        for(int x = 0; x < clocks_per_line; x++) {
            uint8_t colu = screen[x + y * clocks_per_line];
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

struct TIAAudioChannel
{
    int sound_bit = 0;
    uint16_t poly4 = 0xff;
    uint16_t poly5 = 0xff;
    uint16_t poly9 = 0xff;
    int tone31Counter = 31;
    int tone6Counter = 3;
    int tone6 = 1;
    int tone2 = 1;

    uint8_t advance_clock(uint8_t AUDV, uint8_t AUDF, uint8_t AUDC, int& counter);
    uint8_t advance_audio_clock(uint8_t AUDC);

    int currentPoly4()
    {
        return poly4 & 0x1;
    }

    int nextPoly4()
    {
        int oldbit = poly4 & 0x1;
        int newbit = ((poly4 >> 1) ^ oldbit) & 0x1;
        poly4 = (poly4 >> 1) | (newbit << 3);
        return oldbit;
    }

    int nextPoly5()
    {
        int oldbit = poly5 & 0x1;
        int newbit = ((poly5 >> 2) ^ oldbit) & 0x1;
        poly5 = (poly5 >> 1) | (newbit << 4);
        return oldbit;
    }

    int nextPoly9()
    {
        int oldbit = poly9 & 0x1;
        int newbit = ((poly9 >> 4) ^ oldbit) & 0x1;
        poly9 = (poly9 >> 1) | (newbit << 8);
        return oldbit;
    }

    int nextTone2()
    {
        int bit = tone2;
        tone2 = tone2 ^ 0x1;
        return bit;
    }

    int currentTone6()
    {
        return tone6;
    }

    int nextTone6()
    {
        if(tone6Counter > 0) {
            tone6Counter--;
        } else {
            tone6Counter = 3;
            tone6 ^= 0x1;
        }
        return tone6;
    }

    int currentTone31()
    {
        // change this in nextTone31
        return (tone31Counter > 13) ? 1 : 0;
    }

    int nextTone31()
    {
        if(tone31Counter > 0) { 
            tone31Counter--;
        } else {
            tone31Counter = 31;
        }
        return currentTone31();
    }
};

uint8_t TIAAudioChannel::advance_audio_clock(uint8_t AUDC)
{
    switch(AUDC & 0xF) {
        case 0x0: case 0xb: default:
            return 1;
            break;
        case 0x1:
            return nextPoly4();
            break;
        case 0x2:
            return (currentTone31() != nextTone31()) ? nextPoly4() : currentPoly4();
            break;
        case 0x3:
            return (nextPoly5() == 1) ? nextPoly4() : currentPoly4();
            break;
        case 0x4: case 0x5:
            return nextTone2();
            break;
        case 0x6: case 0xa:
            return nextTone31();
            break;
        case 0x7: case 0x9:
            return nextPoly5();
            break;
        case 0x8:
            return nextPoly9();
            break;
        case 0xc: case 0xd:
            return nextTone6();
            break;
        case 0xe:
            return (currentTone31() != nextTone31()) ? nextTone6() : currentTone6();
            break;
        case 0xf:
            return (nextPoly5() == 1) ? nextTone6() : currentTone6();
            break;
    }
}

uint8_t TIAAudioChannel::advance_clock(uint8_t AUDV, uint8_t AUDF, uint8_t AUDC, int& counter)
{
    using namespace Stella;

    // Does writing new AUDF reset the counter?  Or is it only used to reload the counter?
    if(counter > 0) {
        counter--;
    } else {
        sound_bit = advance_audio_clock(AUDC);
        counter = AUDF;
    }

    return 128 + (sound_bit ? -128 : 127 ) * (AUDV & 0xF) / 128;
}

struct object_counter
{
    uint8_t counter = 0;
    uint8_t period;
    uint8_t reset_timer = 0;
    int horizontal_motion = 0;
    bool reset_pending = false;

    object_counter(uint8_t period) :
        period(period)
    {}

    operator uint8_t()
    {
        return counter;
    }

    void reset(uint8_t latency)
    {
        reset_timer = latency;
        reset_pending = true;
    }

    void set_horizontal_motion(uint8_t move_register)
    {
        horizontal_motion = Stella::get_signed_move(move_register);
    }

    void advance(bool within_hblank, bool hmove, int hmove_counter)
    {
        if(!within_hblank || (hmove && (hmove_counter > 7 - horizontal_motion))) {
            bool reset = reset_pending && (reset_timer == 0);

            counter = !reset ? ((counter + 1) % period) : 0;

            reset_pending = (reset_timer > 0);

            if(reset_timer > 0) {
                reset_timer--;
            }
        }
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
    static constexpr uint32_t debug = DEBUG_TIA;

    std::array<uint8_t, 128> RAM;
    std::vector<uint8_t> ROM;
    uint16_t ROM_address_mask;
    sysclock& clk;
    uint32_t horizontal_clock = 0;
    uint32_t scanline = 0;
    bool within_hblank = true;
    bool late_reset_hblank = false;
    bool hmove_latched = false;
    int hmove_counter = 0;

    object_counter P0counter{Stella::visible_pixels};
    object_counter P1counter{Stella::visible_pixels};
    object_counter M0counter{Stella::visible_pixels};
    object_counter M1counter{Stella::visible_pixels};
    object_counter BLcounter{Stella::visible_pixels};

    uint32_t interval_timer_subcounter = 2;
    uint32_t interval_timer_prescaler = 1;
    uint32_t interval_timer_counter = 0;
    uint32_t interval_timer = 0;
    bool timer_interrupt = false;

    uint8_t screen[228 * 262];

    int audio_counter[2] = {0, 0};
    uint64_t next_sample_index = 0;
    uint8_t audio_levels[2] = {128, 128};
    clk_t next_sample_clock = 0;
    uint32_t sampling_rate = 44100;
    clk_t previous_audio_processing_clock = 0;
    clk_t next_audio_clock = 0;
    clk_t clock_rate = 3579540;
    static constexpr clk_t video_clocks_per_audio_clock = 114;
    TIAAudioChannel audio_channels[2];
    uint32_t stereoU8SampleRate;
    size_t preferredAudioBufferSizeBytes;
    std::vector<unsigned char> audio_buffer;

    void set_colu(int x, int y, uint8_t colu)
    {
        using namespace Stella;
        screen[x + y * clocks_per_line] = colu;
    }

    void set_interval_timer(int prescaler, uint8_t value)
    {
        interval_timer_prescaler = prescaler;
        interval_timer_counter = prescaler - 1;
        if(value == 0) {
            interval_timer = 0xFF;
        } else {
            interval_timer = value - 1;
        }
        // printf("set_interval_timer %d and scaler %d\n", value, prescaler);
        timer_interrupt = false;
    }

    void advance_interval_timer()
    {
        if(interval_timer_subcounter > 0) {
            interval_timer_subcounter--;
        } else {
            interval_timer_subcounter = 2;
            if(interval_timer_counter > 0) {
                interval_timer_counter--;
            } else {
                interval_timer_counter = interval_timer_prescaler - 1;
                if(interval_timer == 0) {
                    timer_interrupt = true;
                    interval_timer = 0xFF;
                } else {
                    interval_timer--;
                }
                if(debug & DEBUG_TIMER) { printf("timer now %d\n", interval_timer); }
            }
        }
    }

    void advance_sound_clock()
    {
        using namespace Stella;
        clk_t current_audio_processing_clock = previous_audio_processing_clock + 1;

        // fprintf(stderr, "%llu, %llu, %llu\n", current_audio_processing_clock, next_audio_clock, next_sample_clock);

        if(next_audio_clock < current_audio_processing_clock) {
            audio_levels[0] = audio_channels[0].advance_clock(tia_write[AUDV0], tia_write[AUDF0], tia_write[AUDC0], audio_counter[0]);
            audio_levels[1] = audio_channels[1].advance_clock(tia_write[AUDV1], tia_write[AUDF1], tia_write[AUDC1], audio_counter[1]);
            next_audio_clock = current_audio_processing_clock + video_clocks_per_audio_clock;
        }

        if(next_sample_clock < current_audio_processing_clock) {
            audio_buffer.push_back(audio_levels[0]);
            audio_buffer.push_back(audio_levels[1]);
            if(audio_buffer.size() == preferredAudioBufferSizeBytes) {
                PlatformInterface::EnqueueStereoU8AudioSamples(audio_buffer.data(), audio_buffer.size());
                audio_buffer.clear();
            }
            next_sample_index ++;
            next_sample_clock = next_sample_index * clock_rate / sampling_rate;
        }

        previous_audio_processing_clock = current_audio_processing_clock;
    }

    void advance_object_counters()
    {
        using namespace Stella;
        // printf("P0 advance at %d, current is %d,", horizontal_clock, (int)P0counter);
        P0counter.advance(within_hblank, hmove_latched, hmove_counter);
        // printf("advanced to %d\n", (int)P0counter);
        P1counter.advance(within_hblank, hmove_latched, hmove_counter);
        M0counter.advance(within_hblank, hmove_latched, hmove_counter);
        M1counter.advance(within_hblank, hmove_latched, hmove_counter);
        BLcounter.advance(within_hblank, hmove_latched, hmove_counter);
    }

    uint8_t tia_write[64];
    uint8_t GRP0A, GRP1A, ENABLA;
    uint8_t tia_read[64];
    bool wait_for_hsync = false;
    bool vsync_enabled = false;
    bool mark_cpu_wait = false;

    stella(const std::vector<uint8_t>& ROM, sysclock& clock) :
        ROM(std::move(ROM)),
        clk(clock)
    {
        using namespace Stella;
        if(ROM.size() == 0x800) {
            ROM_address_mask = 0x7ff;
        } else if(ROM.size() == 0x1000) {
            ROM_address_mask = 0xfff;
        } else {
            std::cout << "dunno about ROM size " << ROM.size() << "\n";
            abort();
        }
        memset(screen, 0, sizeof(screen));
        tia_write[AUDV0] = 0;
        tia_write[AUDV1] = 0;
        PlatformInterface::Start(stereoU8SampleRate, preferredAudioBufferSizeBytes);
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
            uint16_t reg = addr & 0xF;
            if(reg == INPT5) {
                // read latched or unlatched input port 5
                uint8_t inpt5 = 0;
                uint8_t swcha, player0button, player1button;
                std::tie(swcha, player0button, player1button) = PlatformInterface::ReadJoysticks();
                inpt5 |= player1button;
                return inpt5;
            } else if(reg == INPT4) {
                // read latched or unlatched input port 4
                uint8_t inpt4 = 0;
                uint8_t swcha, player0button, player1button;
                std::tie(swcha, player0button, player1button) = PlatformInterface::ReadJoysticks();
                inpt4 |= player0button;
                return inpt4;
            } else if(reg == INPT3) {
                // read latched or unlatched input port 3
                return 0x0;
            } else if(reg == INPT2) {
                // read latched or unlatched input port 2
                return 0x0;
            } else if(reg == INPT1) {
                // read latched or unlatched input port 1
                return 0x0;
            } else if(reg == INPT0) {
                // read latched or unlatched input port 0
                return 0x0;
            } else if(reg == CXM0P) {
                return tia_read[CXM0P];
            } else if(reg == CXM1P) {
                return tia_read[CXM1P];
            } else if(reg == CXP0FB) {
                return tia_read[CXP0FB];
            } else if(reg == CXP1FB) {
                return tia_read[CXP1FB];
            } else if(reg == CXM0FB) {
                return tia_read[CXM0FB];
            } else if(reg == CXM1FB) {
                return tia_read[CXM1FB];
            } else if(reg == CXBLPF) {
                return tia_read[CXBLPF];
            } else if(reg == CXPPMM) {
                return tia_read[CXPPMM];
            } else {
                printf("unhandled read from TIA at %04X\n", addr);
                // abort();
                return 0x00;
            }
        } else if(isPIA(addr)) {
            if(debug & DEBUG_PIA) { printf("read from PIA %04X\n", addr); }
            addr &= 0x1F;
            if(addr == SWCHB) {
                return PlatformInterface::ReadConsoleSwitches();
            } else if(addr == INTIM) {
                uint8_t data = interval_timer;
                timer_interrupt = false;
                if(debug & DEBUG_TIMER) { printf("read interval timer, %2X\n", data); }
                return data;
            } else if(addr == INSTAT) {
                uint8_t data = timer_interrupt ? 0x80 : 0;
                timer_interrupt = false;
                if(debug & DEBUG_TIMER) { printf("read interval status, %2X\n", data); }
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
            // printf("wrote %02X to PIA %04X\n", data, addr);
            addr &= 0x1F;
            if(addr == TIM1T) {
                set_interval_timer(1, data);
            } else if(addr == TIM8T) {
                set_interval_timer(8, data);
            } else if(addr == TIM64T) {
                set_interval_timer(64, data);
            } else if(addr == T1024T) {
                set_interval_timer(1024, data);
            }
            // XXX TODO
        } else if(isTIA(addr)) {
            uint8_t reg = addr & 0x3F;
            if(debug & DEBUG_TIA) { printf("(%3d, %3d) wrote %02X to %02X (%s)\n", horizontal_clock, scanline, data, reg, TIA_register_names[reg].c_str()); }
            if(reg == VSYNC) {
                if(data & VSYNC_SET) {
                    // printf("VSYNC was enabled at %d, %d\n", horizontal_clock, scanline);
                    vsync_enabled = true;
                } else {
                    if(vsync_enabled) {
                        // printf("VSYNC was disabled at %d, %d\n", horizontal_clock, scanline);
                        scanline = 0;
                        // write_screen();
                        PlatformInterface::Frame(screen, 1.0f);
                        vsync_enabled = false;
                    }
                }
            } else if(reg == CXCLR) {
                // reset collision latches
                tia_read[CXM0P] = 0;
                tia_read[CXM1P] = 0;
                tia_read[CXP0FB] = 0;
                tia_read[CXP1FB] = 0;
                tia_read[CXM0FB] = 0;
                tia_read[CXM1FB] = 0;
                tia_read[CXBLPF] = 0;
                tia_read[CXPPMM] = 0;
            } else if(reg == HMCLR) {
                // Reset all 5 motion registers to 0
                tia_write[HMBL] = 0;
                tia_write[HMM1] = 0;
                tia_write[HMM0] = 0;
                tia_write[HMP1] = 0;
                tia_write[HMP0] = 0;
                P0counter.set_horizontal_motion(0);
                P1counter.set_horizontal_motion(0);
                M0counter.set_horizontal_motion(0);
                M1counter.set_horizontal_motion(0);
                BLcounter.set_horizontal_motion(0);
            } else if(reg == HMOVE) {
                // Apply the motion registers to players, missles, and ball
                late_reset_hblank = true;
                hmove_latched = true;
                bool within_vblank = tia_write[VBLANK] & VBLANK_ENABLED;
                hmove_counter = within_vblank ? 15 - 12 : 15;
            } else if(reg == RESMP1) {
                // XXX handle hid/lock bit
                M0counter.counter = P0counter.counter;
            } else if(reg == RESMP0) {
                // XXX handle hid/lock bit
                M1counter.counter = P1counter.counter;
            } else if(reg == VDELBL) {
                tia_write[VDELBL] = data;
            } else if(reg == VDELP1) {
                tia_write[VDELP1] = data;
            } else if(reg == VDELP0) {
                tia_write[VDELP0] = data;
            } else if(reg == HMBL) {
                tia_write[HMBL] = data;
                BLcounter.set_horizontal_motion(data);
            } else if(reg == HMM1) {
                tia_write[HMM1] = data;
                M1counter.set_horizontal_motion(data);
            } else if(reg == HMM0) {
                tia_write[HMM0] = data;
                M0counter.set_horizontal_motion(data);
            } else if(reg == HMP1) {
                tia_write[HMP1] = data;
                P1counter.set_horizontal_motion(data);
            } else if(reg == HMP0) {
                tia_write[HMP0] = data;
                P0counter.set_horizontal_motion(data);
            } else if(reg == ENABL) {
                if(VDELBL & VDEL_ENABLED) {
                    ENABLA = data;
                } else {
                    tia_write[ENABL] = data;
                }
            } else if(reg == ENAM1) {
                tia_write[ENAM1] = data;
            } else if(reg == ENAM0) {
                tia_write[ENAM0] = data;
            } else if(reg == GRP1) {
                ENABLA = tia_write[ENABL];
                GRP0A = tia_write[GRP0];
                tia_write[GRP1] = data;
                // printf("%02X %02X %02X %02X\n", tia_write[GRP0], GRP0A, tia_write[GRP1], GRP1A);
            } else if(reg == GRP0) {
                GRP1A = tia_write[GRP1];
                tia_write[GRP0] = data;
                // printf("%02X %02X %02X %02X\n", tia_write[GRP0], GRP0A, tia_write[GRP1], GRP1A);
            } else if(reg == AUDV1) {
                // printf("AUDV1,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDV1] = data;
            } else if(reg == AUDV0) {
                // printf("AUDV0,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDV0] = data;
            } else if(reg == AUDF1) {
                // printf("AUDF1,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDF1] = data;
            } else if(reg == AUDF0) {
                // printf("AUDF0,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDF0] = data;
            } else if(reg == AUDC1) {
                // printf("AUDC1,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDC1] = data;
            } else if(reg == AUDC0) {
                // printf("AUDC0,%llu,%d,%d\n", (clk_t)clk, reg, data);
                tia_write[AUDC0] = data;
            } else if(reg == RESBL) {
                // ALMOST DEFINITELY WRONG
                BLcounter.reset(within_hblank ? 2 : 4);
            } else if(reg == RESM1) {
                // ALMOST DEFINITELY WRONG
                M1counter.reset(within_hblank ? 2 : 4);
            } else if(reg == RESM0) { 
                // ALMOST DEFINITELY WRONG
                M0counter.reset(within_hblank ? 2 : 4);
            } else if(reg == RESP1) {
                P1counter.reset(within_hblank ? 3 : 5);
            } else if(reg == RESP0) {
                P0counter.reset(within_hblank ? 3 : 5);
            } else if(reg == PF2) {
                tia_write[PF2] = data;
            } else if(reg == PF1) {
                tia_write[PF1] = data;
            } else if(reg == PF0) {
                tia_write[PF0] = data;
            } else if(reg == REFP1) {
                tia_write[REFP1] = data;
            } else if(reg == REFP0) {
                tia_write[REFP0] = data;
            } else if(reg == CTRLPF) {
                tia_write[CTRLPF] = data;
            } else if(reg == COLUBK) {
                tia_write[COLUBK] = data;
            } else if(reg == COLUPF) {
                tia_write[COLUPF] = data;
            } else if(reg == COLUP1) {
                tia_write[COLUP1] = data;
            } else if(reg == COLUP0) {
                tia_write[COLUP0] = data;
            } else if(reg == NUSIZ0) {
                tia_write[NUSIZ0] = data;
            } else if(reg == NUSIZ1) {
                tia_write[NUSIZ1] = data;
            } else if(reg == RSYNC) {
                /* ignored, resets hsync for testing */
            } else if(reg == WSYNC) {
                // printf("write %d to WSYNC\n", data); 
                wait_for_hsync = true;
            } else if(reg == VBLANK) {
                tia_write[VBLANK] = data;
                // printf("write %d to VBLANK\n", data); 
            } else if(reg == 0x2D) {
                // ignore
            } else if(reg == 0x2E) {
                // ignore
            } else if(reg == 0x2F) {
                // ignore
            } else if((reg >= 0x30) && (reg <= 0x3F)) {
                // ignore
            }
        } else {
            printf("unhandled write of %02X to %04X\n", data, addr);
            abort();
        }
    }

    int get_playfield_bit(int horizontal_clock)
    {
        using namespace Stella;

        static uint8_t cachedPF0 = 0;
        static uint8_t cachedPF1 = 0;
        static uint8_t cachedPF2 = 0;

        if((horizontal_clock == hblank_pixels - 1) || (horizontal_clock == hblank_pixels + visible_pixels / 2)) {
            cachedPF0 = tia_write[PF0];
        }
        if((horizontal_clock == hblank_pixels - 1 + 16) || (horizontal_clock == hblank_pixels + visible_pixels / 2 + 16)) {
            cachedPF1 = tia_write[PF1];
        }
        if((horizontal_clock == hblank_pixels - 1 + 32) || (horizontal_clock == hblank_pixels + visible_pixels / 2 + 32)) {
            cachedPF2 = tia_write[PF2];
        }

        if(horizontal_clock < hblank_pixels) {
            return 0;
        }

        int x = horizontal_clock - hblank_pixels;

        int playfield_bit_number = x / 4;

        if(playfield_bit_number >= 20) {
            if(tia_write[CTRLPF] & CTRLPF_REFLECT_PLAYFIELD) {
                playfield_bit_number = 39 - playfield_bit_number;
            } else {
                playfield_bit_number = playfield_bit_number - 20;
            }
        }

        if(playfield_bit_number < 4) {
            return (cachedPF0 >> (4 + playfield_bit_number)) & 0x01;
        }
        if(playfield_bit_number < 12) {
            return (cachedPF1 >> (11 - playfield_bit_number)) & 0x01;
        }
        return (cachedPF2 >> (playfield_bit_number - 12)) & 0x01;
    }
    
    int get_player_bit(uint8_t counter, uint8_t grp, uint8_t nusiz, uint8_t refp)
    {
        using namespace Stella;

        // XXX This can't be right, but it's what I measured...
        // counter = (counter - 24 + 160) % 160;

        bool replicateY = false;
        bool replicateZ = false;
        int offsetY, offsetZ;
        bool shift = 0;

        switch(nusiz & 0x7) {
            case 0: /* X......... */
                break;
            case 1: /* X.Y....... */
                replicateY = true;
                offsetY = 16;
                break;
            case 2: /* X...Y..... */
                replicateY = true;
                offsetY = 32;
                break;
            case 3: /* X.Y.Z..... */
                replicateY = true;
                replicateZ = true;
                offsetY = 16;
                offsetZ = 32;
                break;
            case 4: /* X.......Y. */
                replicateY = true;
                offsetY = 64;
                break;
            case 5: /* XX........ */
                shift = 1;
                break;
            case 6: /* X...Y...Z. */
                replicateY = true;
                replicateZ = true;
                offsetY = 32;
                offsetZ = 64;
                break;
            case 7: /* XXXX...... */
                shift = 2;
                break;
        }

        int bit_index;
        if((counter >> shift) < 8) {
            bit_index = counter >> shift;
        } else if(replicateY && (counter >= offsetY) && ((counter - offsetY) < 8)) {
            bit_index = counter - offsetY;
        } else if(replicateZ && (counter >= offsetZ) && ((counter - offsetZ) < 8)) {
            bit_index = counter - offsetZ;
        } else {
            return 0;
        }

        if(refp & REFP_REFLECT) {
            return (grp >> bit_index) & 0x01;
        } else {
            return (grp << bit_index) & 0x80;
        }
    }

    int get_missile_bit(uint8_t counter, uint8_t nusiz)
    {
        using namespace Stella;
        
        bool replicateY = false;
        bool replicateZ = false;
        int offsetY, offsetZ;
        bool shift = (nusiz >> 4) & 0x03;

        switch(nusiz & 0x7) {
            case 0: /* X......... */
                break;
            case 1: /* X.Y....... */
                replicateY = true;
                offsetY = 16;
                break;
            case 2: /* X...Y..... */
                replicateY = true;
                offsetY = 32;
                break;
            case 3: /* X.Y.Z..... */
                replicateY = true;
                replicateZ = true;
                break;
            case 4: /* X.......Y. */
                replicateY = true;
                offsetY = 64;
                break;
            case 5: /* ?? */
                break;
            case 6: /* X...Y...Z. */
                replicateY = true;
                replicateZ = true;
                offsetY = 32;
                offsetZ = 64;
                break;
            case 7: /* ?? */
                break;
        }

        if((counter >> shift) == 0) {
            return 1;
        } else if(replicateY && (counter >= offsetY) && ((counter - offsetY) == 0)) {
            return 1;
        } else if(replicateZ && (counter >= offsetZ) && ((counter - offsetZ) == 0)) {
            return 1;
        } else {
            return 0;
        }
    }

    int get_ball_bit(uint8_t counter, uint8_t ctrlpf)
    {
        using namespace Stella;

        int shift = (tia_write[CTRLPF] >> 4) & 0x03;
        return (counter >> shift) == 0;
    }

    uint8_t evaluate_pixel_color()
    {
        using namespace Stella;

        bool within_vblank = tia_write[VBLANK] & VBLANK_ENABLED;
        if(within_vblank) {
            return 0x00; // BLACK
        }

        // Background
        uint8_t color = tia_write[COLUBK];

        // Playfield

        int pf = 0;
        pf = get_playfield_bit(horizontal_clock); // Does caching

        if(within_hblank) {
            return 0x00; // color;
        }

        // Players

        int p0 = 0;
        {
            uint8_t grp0 = (tia_write[VDELP0] & VDEL_ENABLED) ? GRP0A : tia_write[GRP0];
            if(grp0 != 0) {
                p0 = get_player_bit(P0counter, grp0, tia_write[NUSIZ0], tia_write[REFP0]);
            }
        }

        int p1 = 0;
        {
            uint8_t grp1 = (tia_write[VDELP1] & VDEL_ENABLED) ? GRP1A : tia_write[GRP1];
            if(grp1 != 0) {
                p1 = get_player_bit(P1counter, grp1, tia_write[NUSIZ1], tia_write[REFP1]);
            }
        }

        // Missiles

        int m0 = 0;
        if(tia_write[ENAM0] & ENABL_ENABLED) {
            m0 = get_missile_bit(M0counter, tia_write[NUSIZ0]);
        }

        int m1 = 0;
        if(tia_write[ENAM1] & ENABL_ENABLED) {
            m1 = get_missile_bit(M1counter, tia_write[NUSIZ1]);
        }

        // Ball

        int bl = 0;
        int enabl = (tia_write[VDELBL] & VDEL_ENABLED) ? ENABLA : tia_write[ENABL];
        if(enabl & ENABL_ENABLED) {
            bl = get_ball_bit(BLcounter, tia_write[CTRLPF]);
        }

        // XXX process rest of registers

        // Priority
        // XXX read and use priority register
        // XXX playfield and ball can be over players and missles if CTRLPF & 0x4, so need to handle that later
        if(pf) {
            color = tia_write[COLUPF];
        }
        if(p0) {
            color = tia_write[COLUP0];
        }
        if(p1) {
            color = tia_write[COLUP1];
        }
        if(m0) {
            color = tia_write[COLUP0];
        }
        if(m1) {
            color = tia_write[COLUP1];
        }
        if(bl) {
            color = tia_write[COLUPF];
        }

        // Collision
        tia_read[CXM0P] |=
            ((m0 && p1) ? 0x80 : 0) |
            ((m0 && p0) ? 0x40 : 0);
        tia_read[CXM1P] |=
            ((m1 && p0) ? 0x80 : 0) |
            ((m1 && p1) ? 0x40 : 0);
        tia_read[CXP0FB] |=
            ((p0 && pf) ? 0x80 : 0) |
            ((p0 && bl) ? 0x40 : 0);
        tia_read[CXP1FB] |=
            ((p1 && pf) ? 0x80 : 0) |
            ((p1 && bl) ? 0x40 : 0);
        tia_read[CXM0FB] |=
            ((m0 && pf) ? 0x80 : 0) |
            ((m0 && bl) ? 0x40 : 0);
        tia_read[CXM1FB] |=
            ((m1 && pf) ? 0x80 : 0) |
            ((m1 && bl) ? 0x40 : 0);
        tia_read[CXBLPF] |=
            ((bl && pf) ? 0x80 : 0);
        tia_read[CXPPMM] |=
            ((p0 && p1) ? 0x80 : 0) |
            ((m0 && m1) ? 0x40 : 0);

        return color;
    }

    clk_t last_pixel_clocked;

    // Must be called once and only once for every clock value
    // I.e. clk must be incrementing on each call
    bool advance_one_clock(bool mark_cpu_wait)
    {
        using namespace Stella;

	// Do all the work for this clock except leave horizontal_clock
	// and scanline until the end since other operations use them

        if(late_reset_hblank) {
            within_hblank = horizontal_clock < (hblank_pixels + 8);
        } else {
            within_hblank = horizontal_clock < hblank_pixels;
        }

        advance_object_counters();

        advance_interval_timer();

        advance_sound_clock();

        int color = evaluate_pixel_color();

        if(mark_cpu_wait) {
            color = 0x0F;
        }

        set_colu(horizontal_clock, scanline, color);

        // And then move forward the horizontal clock and scanline

        if(hmove_counter > 0) {
            hmove_counter -= 1;
        }

        horizontal_clock++;
        if(horizontal_clock >= clocks_per_line) {
            late_reset_hblank = false;
            hmove_latched = false;
            horizontal_clock = 0;
            scanline++;
            if(scanline >= lines_per_frame) {
                scanline = 0;
            }
        }

        return within_hblank;
    }

    void advance_to_clock(const sysclock& clk)
    {
        using namespace Stella;
        for(clk_t c = last_pixel_clocked; c < clk; c++) {
            advance_one_clock(false);
        }
        last_pixel_clocked = clk;
    }

    clk_t advance_to_hsync(const sysclock& clk)
    {
        using namespace Stella;
        bool start_of_hblank = horizontal_clock == 0;
        clk_t clocks = 0;

        while(!start_of_hblank) {
            advance_one_clock(false);
            clocks++;
            start_of_hblank = horizontal_clock == 0;
        };

        last_pixel_clocked = clk + clocks;

        wait_for_hsync = false;
        return clocks;
    }
};

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
    if(argc < 2) {
        fprintf(stderr, "usage: %s cartridge-rom-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }
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

    struct clock_handler
    {
        sysclock& clk;
        stella& hw;
        clock_handler(sysclock& clk, stella& hw) : clk(clk), hw(hw) {}
        void add_cpu_cycles(int n) {
            for(int i = 0; i < n; i++) {
                clk.add_pixel_cycles(3);
                hw.advance_to_clock(clk);
            }
        }
    } clk_(clk, hw);

    CPU6502 cpu(clk_, hw);
    cpu.reset();

    while(1) {
        if(false) {
            std::string dis = read_bus_and_disassemble(hw, cpu.pc);
            printf("%10llu %4u %s\n", (clk_t)clk, hw.horizontal_clock, dis.c_str());
        }
        cpu.cycle();
        // printf("clk = %llu\n", (clk_t)clk);
        if(hw.wait_for_hsync) {
            auto cycles = hw.advance_to_hsync(clk);
            clk.add_pixel_cycles(cycles);
        }// else {
          //  hw.advance_to_clock(clk);
        //}
    }
}
