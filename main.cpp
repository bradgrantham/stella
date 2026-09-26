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

uint16_t paddleValue;

uint16_t RoGetPaddleValue(int paddle)
{
    if(paddle == 0)
    {
        return paddleValue;
    }
    else
    {
        return 0;
    }
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
// Frame() adjusts its pacing to keep the queued audio between these
size_t audio_queue_low_bytes;
size_t audio_queue_middle_bytes;
size_t audio_queue_high_bytes;
bool audio_catching_up = false;

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
std::chrono::time_point<std::chrono::system_clock> next_frame_time;

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
    // No PRESENTVSYNC; frames are paced by the audio queue in Frame()
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // SDL enables text input by default on desktop; with it on, macOS
    // shows the accent popup for held letter keys instead of repeating.
    SDL_StopTextInput();
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
    audiospec.samples = 512; // audiospec.freq / 100;
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
    // Push samples in small pieces so the queue size seen by Frame() is fine-grained
    preferredAudioBufferSizeBytes = obtained.size / 4;
    actual_audio_format = obtained.format;
    audio_queue_low_bytes = obtained.size * 1;
    audio_queue_middle_bytes = obtained.size * 2;
    audio_queue_high_bytes = obtained.size * 4;

    SDL_PumpEvents();

    previous_event_time = std::chrono::system_clock::now();
    previous_frame_time = std::chrono::system_clock::now();
    next_frame_time = std::chrono::system_clock::now();
}

static void HandleEvents(void)
{
    using namespace Stella;
    static bool switch_tv_type = true; // true = color
    static bool switch_p0_difficulty = false;
    static bool switch_p1_difficulty = false;
    static bool shift_pressed = false;
    static SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_MOUSEMOTION:
                int width, height;
                SDL_GetWindowSize(window, &width, &height);
                paddleValue = 65536 * event.motion.x / width;
                break;
            // case SDL_MOUSEWHEELEVENT:
                // SDL_MouseWheelEvent wheel;              /**< Mouse wheel event data */
                // break;
            case SDL_WINDOWEVENT:
                // printf("window event %d\n", event.window.event);
                switch(event.window.event) {
                }
                break;
            case SDL_QUIT:
                // event_queue.push_back({QUIT, 0});
                exit(0);
                break;

            case SDL_KEYDOWN:
                // Held keys (joystick directions, buttons, momentary
                // switches) are read from SDL_GetKeyboardState() below so
                // that autorepeat has no effect on them.  Only toggles are
                // handled here, and only on the first press.
                if(event.key.repeat) {
                    break;
                }
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_RSHIFT:
                    case SDL_SCANCODE_LSHIFT:
                        shift_pressed = true;
                        break;
                    case SDL_SCANCODE_1:
                        switch_tv_type = !switch_tv_type;
                        if(switch_tv_type) {
                            SWCHB_value |= SWCHB_TVTYPE_SWITCH;
                        } else {
                            SWCHB_value &= ~SWCHB_TVTYPE_SWITCH;
                        }
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
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    // Joystick directions, buttons, and momentary switches come from the
    // current physical key state, which SDL maintains from the key down
    // and key up events and which is unaffected by autorepeat.
    const Uint8 *keys = SDL_GetKeyboardState(nullptr);

    SWCHA_value |= SWCHA_JOYSTICK0_UP | SWCHA_JOYSTICK0_DOWN | SWCHA_JOYSTICK0_LEFT | SWCHA_JOYSTICK0_RIGHT;
    if(keys[SDL_SCANCODE_W]) {
        SWCHA_value &= ~SWCHA_JOYSTICK0_UP;
    }
    if(keys[SDL_SCANCODE_S]) {
        SWCHA_value &= ~SWCHA_JOYSTICK0_DOWN;
    }
    if(keys[SDL_SCANCODE_A]) {
        SWCHA_value &= ~SWCHA_JOYSTICK0_LEFT;
    }
    if(keys[SDL_SCANCODE_D]) {
        SWCHA_value &= ~SWCHA_JOYSTICK0_RIGHT;
    }

    player0button = keys[SDL_SCANCODE_SPACE] ? (uint8_t)~INPT4_JOYSTICK0_BUTTON : INPT4_JOYSTICK0_BUTTON;

    SWCHB_value |= SWCHB_RESET_SWITCH | SWCHB_SELECT_SWITCH;
    if(keys[SDL_SCANCODE_2]) {
        SWCHB_value &= ~SWCHB_RESET_SWITCH;
    }
    if(keys[SDL_SCANCODE_3]) {
        SWCHB_value &= ~SWCHB_SELECT_SWITCH;
    }
}

void Frame(const uint8_t* screen, [[maybe_unused]] float megahertz)
{
    using namespace std::chrono_literals;

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed;

    // Pace frames on a wall clock schedule so they come out evenly, and
    // use the amount of audio queued to correct drift between that
    // schedule and the audio device, which consumes samples in real time.
    static constexpr long long frame_micros = 16688; // 262 lines * 228 clocks / 3.579540 MHz

    if(!audio_needs_start && (actual_audio_format == AUDIO_U8)) {
        uint32_t queued = SDL_GetQueuedAudioSize(audio_device);
        if(queued < audio_queue_low_bytes) {
            // Behind real time and audio is about to run dry
            audio_catching_up = true;
        } else if(queued >= audio_queue_middle_bytes) {
            audio_catching_up = false;
        }
        if(audio_catching_up) {
            next_frame_time = now;
        } else if(queued > audio_queue_high_bytes) {
            // Ahead of real time, stretch this frame a little
            next_frame_time += 1ms;
        }
    }

    if(next_frame_time < now - 100ms) {
        // Fell far behind, e.g. stopped in a debugger; don't try to catch up
        next_frame_time = now;
    }

    std::this_thread::sleep_until(next_frame_time);
    next_frame_time += std::chrono::microseconds(frame_micros);

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
    if(elapsed.count() > .03) {
        HandleEvents();
        previous_event_time = now;
    }

}

};

void write_screen(const uint8_t *screen, const char *filename)
{
    using namespace Stella;

    FILE *screenfile = fopen(filename, "wb");
    if(screenfile == nullptr) {
        fprintf(stderr, "couldn't open %s for writing.\n", filename);
        exit(EXIT_FAILURE);
    }
    fprintf(screenfile, "P6 %d %d 255\n", clocks_per_line * 2, lines_per_frame);
    for(int y = 0; y < lines_per_frame; y++) {
        for(int x = 0; x < clocks_per_line; x++) {
            uint8_t colu = screen[x + y * clocks_per_line];
            uint8_t *rgb = PlatformInterface::colu_to_rgb[colu];
            fwrite(rgb, 3, 1, screenfile);
            fwrite(rgb, 3, 1, screenfile);
        }
    }
    fclose(screenfile);
}

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

// The TIA object position counters, "start drawing" decodes, drawing
// state, and horizontal motion below are modeled after Stella's TIA
// (src/emucore/tia/{Player,Missile,Ball,Playfield}.{hxx,cxx} and TIA.cxx).
//
// Each object has a 160-count position counter that is clocked once per
// visible pixel, and additionally by HMOVE "extra" clocks during HBLANK.
// The first copy of an object starts drawing when the counter passes 156,
// and the extra NUSIZ copies start at 12, 28, and 60.  Players then take
// a further 6 clocks to put out their first pixel and missiles and the
// ball take 5, which is why RESPx/RESMx/RESBL load 157 (visible), 159
// (HBLANK) or 158 (late in an HMOVE-extended HBLANK) and not 0.

struct object_decodes
{
    uint8_t table[8][Stella::visible_pixels];

    object_decodes()
    {
        memset(table, 0, sizeof(table));
        for(int nusiz = 0; nusiz < 8; nusiz++) {
            table[nusiz][156] = 1;              // first copy, all NUSIZ values
        }
        table[1][12] = 2;                       // two copies close
        table[2][28] = 2;                       // two copies medium
        table[3][12] = 2; table[3][28] = 3;     // three copies close
        table[4][60] = 2;                       // two copies wide
        table[6][28] = 2; table[6][60] = 3;     // three copies medium
    }
};

static object_decodes decodes;

struct movable_object
{
    uint8_t counter = 0;
    int hmm_clocks = 8;         // HMxx register as a count of extra clocks, 0 through 15
    bool is_moving = false;
    uint8_t last_movement_tick = 0;

    void set_horizontal_motion(uint8_t move_register)
    {
        hmm_clocks = (move_register >> 4) ^ 0x08;
    }

    void start_movement()
    {
        is_moving = true;
    }

    // Called every fourth color clock after HMOVE with the HMOVE counter
    // value; returns true if this object should receive an extra clock
    bool movement_tick(int clock)
    {
        last_movement_tick = counter;
        if(is_moving && (clock == hmm_clocks)) {
            is_moving = false;
        }
        return is_moving;
    }

    void advance_counter()
    {
        counter = (counter + 1) % Stella::visible_pixels;
    }
};

struct player_object : movable_object
{
    static constexpr int render_counter_offset = -5;

    bool is_rendering = false;
    int render_counter = 0;
    int sample_counter = 0;     // which of the 8 GRPx bits is being drawn
    int copy = 0;
    int divider = 1;            // 1, 2, or 4 for single, double, quad width
    int divider_pending = 1;
    int render_counter_trip_point = 0;
    const uint8_t *decode_table = decodes.table[0];
    uint8_t grp_new = 0;
    uint8_t grp_old = 0;
    bool delayed = false;       // VDELPx
    bool reflected = false;     // REFPx
    bool on = false;            // drawing a pixel this clock

    uint8_t pattern()
    {
        return delayed ? grp_old : grp_new;
    }

    void set_divider(int new_divider)
    {
        divider = new_divider;
        // Double and quad width players start one clock later
        render_counter_trip_point = (divider == 1) ? 0 : 1;
    }

    void set_nusiz(uint8_t nusiz)
    {
        using namespace Stella;

        int copies = nusiz & 0x07;
        divider_pending = (copies == 5) ? 2 : (copies == 7) ? 4 : 1;

        const uint8_t *old_decode_table = decode_table;
        decode_table = decodes.table[copies];

        // Changing NUSIZ can trigger a decode in the same clock
        int previous_counter = (counter + visible_pixels - 1) % visible_pixels;
        if(!is_rendering && decode_table[previous_counter]) {
            is_rendering = true;
            sample_counter = 0;
            render_counter = render_counter_offset;
            copy = decode_table[previous_counter];
        }

        if((decode_table != old_decode_table) && is_rendering &&
            ((render_counter - render_counter_offset) < 2) &&
            !decode_table[(counter - render_counter + render_counter_offset + visible_pixels - 1) % visible_pixels]) {
            is_rendering = false;
        }

        // XXX Stella models the effect of changing the width during
        // drawing in detail; here a new width takes effect at the next copy
        if(!is_rendering) {
            set_divider(divider_pending);
        }
    }

    void reset_position(uint8_t new_counter)
    {
        counter = new_counter;
        if(is_rendering && ((render_counter - render_counter_offset) < 4)) {
            render_counter = render_counter_offset + (new_counter - 157);
        }
    }

    void tick()
    {
        if(!is_rendering || (render_counter < render_counter_trip_point)) {
            on = false;
        } else if(reflected) {
            on = (pattern() >> sample_counter) & 0x01;
        } else {
            on = (pattern() >> (7 - sample_counter)) & 0x01;
        }

        if(decode_table[counter]) {
            is_rendering = true;
            sample_counter = 0;
            render_counter = render_counter_offset;
            copy = decode_table[counter];
            if(divider != divider_pending) {
                set_divider(divider_pending);
            }
        } else if(is_rendering) {
            render_counter++;
            if(divider == 1) {
                if(render_counter > 0) {
                    sample_counter++;
                }
            } else {
                if((render_counter > 1) && (((render_counter - 1) % divider) == 0)) {
                    sample_counter++;
                }
            }
            if(sample_counter > 7) {
                is_rendering = false;
            }
        }

        advance_counter();
    }

    bool is_drawing_first_copy_at_4()
    {
        return is_rendering && (sample_counter == 4) && (copy == 1);
    }

    // Counter value that puts a missile at the center of this player,
    // for RESMPx
    uint8_t get_resmp_counter()
    {
        int offset = (divider == 1) ? 5 : (divider == 2) ? 8 : 12;
        return (counter + Stella::visible_pixels - offset) % Stella::visible_pixels;
    }
};

struct missile_object : movable_object
{
    static constexpr int render_counter_offset = -4;

    bool is_rendering = false;
    int render_counter = 0;
    int width = 1;
    int effective_width = 1;
    bool enabled = false;       // ENAMx
    bool locked = false;        // RESMPx, missile hidden and locked to player
    const uint8_t *decode_table = decodes.table[0];
    bool on = false;            // drawing a pixel this clock

    void set_nusiz(uint8_t nusiz)
    {
        width = 1 << ((nusiz >> 4) & 0x03);
        decode_table = decodes.table[nusiz & 0x07];
        if(is_rendering && (render_counter >= width)) {
            is_rendering = false;
        }
    }

    void set_resmp(uint8_t resmp)
    {
        locked = resmp & Stella::RESMP_LOCK;
    }

    void reset_position(uint8_t new_counter, bool within_hblank)
    {
        counter = new_counter;

        if(is_rendering) {
            if(render_counter < 0) {
                render_counter = render_counter_offset + (new_counter - 157);
            } else {
                // Stella's description of missile width after RESMx during drawing
                switch(width) {
                    case 8:
                        render_counter = (new_counter - 157) + ((render_counter >= 4) ? 4 : 0);
                        break;
                    case 4:
                        render_counter = (new_counter - 157);
                        break;
                    case 2:
                        if(within_hblank) {
                            is_rendering = render_counter > 1;
                        } else if(render_counter == 0) {
                            render_counter++;
                        }
                        break;
                    default:
                        if(within_hblank) {
                            is_rendering = render_counter > 0;
                        }
                        break;
                }
            }
        }
    }

    // regular_clock is true for the once-per-pixel clock and false for
    // HMOVE extra clocks
    void tick(int horizontal_clock, bool regular_clock)
    {
        bool visible = is_rendering &&
            ((render_counter >= 0) ||
             (is_moving && regular_clock && (render_counter == -1) && (width < 4) && (((horizontal_clock + 1) % 4) == 3)));
        on = visible && enabled && !locked;

        if(decode_table[counter] && !locked) {
            is_rendering = true;
            render_counter = render_counter_offset;
        } else if(is_rendering) {
            if(render_counter == -1) {
                if(is_moving && regular_clock) {
                    // Regular clock during HMOVE, Cosmic Ark "starfield" mode
                    switch((horizontal_clock + 1) % 4) {
                        case 3:
                            effective_width = (width == 1) ? 2 : width;
                            if(width < 4) {
                                render_counter++;
                            }
                            break;
                        case 2:
                            effective_width = 0;
                            break;
                        default:
                            effective_width = width;
                            break;
                    }
                } else {
                    effective_width = width;
                }
            }
            render_counter++;
            if(render_counter >= (is_moving ? effective_width : width)) {
                is_rendering = false;
            }
        }

        advance_counter();
    }

    void resmp_tick(player_object& player)
    {
        if(locked && player.is_drawing_first_copy_at_4()) {
            counter = player.get_resmp_counter();
        }
    }
};

struct ball_object : movable_object
{
    static constexpr int render_counter_offset = -4;

    bool is_rendering = false;
    int render_counter = 0;
    int width = 1;
    int effective_width = 1;
    bool enabled_new = false;   // ENABL
    bool enabled_old = false;   // ENABL latched by a write to GRP1
    bool delayed = false;       // VDELBL
    bool on = false;            // drawing a pixel this clock

    bool enabled()
    {
        return delayed ? enabled_old : enabled_new;
    }

    void set_ctrlpf(uint8_t ctrlpf)
    {
        width = 1 << ((ctrlpf >> 4) & 0x03);
    }

    // Unlike players and missiles, the ball starts drawing right after RESBL
    void reset_position(uint8_t new_counter)
    {
        counter = new_counter;
        is_rendering = true;
        render_counter = render_counter_offset + (new_counter - 157);
    }

    void tick(bool regular_clock)
    {
        on = is_rendering && (render_counter >= 0) && enabled();

        bool starfield = is_moving && regular_clock;

        if(counter == 156) {
            is_rendering = true;
            render_counter = render_counter_offset;

            int delta = (counter + Stella::visible_pixels - last_movement_tick) % 4;
            if(starfield && (delta == 3) && (width < 4)) {
                render_counter++;
            }
            switch(delta) {
                case 3:
                    effective_width = (width == 1) ? 2 : width;
                    break;
                case 2:
                    effective_width = 0;
                    break;
                default:
                    effective_width = width;
                    break;
            }
        } else if(is_rendering) {
            render_counter++;
            if(render_counter >= (starfield ? effective_width : width)) {
                is_rendering = false;
            }
        }

        advance_counter();
    }
};

struct playfield_object
{
    uint32_t pattern = 0;       // 20 bits, bit 0 is the leftmost playfield pixel
    bool reflect = false;       // CTRLPF reflect bit
    bool reflect_latched = false;
    int x = 0;                  // visible pixel, 0 through 159
    bool on = false;            // drawing a pixel this clock

    void set_pf0(uint8_t pf0)
    {
        pattern = (pattern & 0x000FFFF0) | (pf0 >> 4);
    }

    void set_pf1(uint8_t pf1)
    {
        // PF1 is displayed most significant bit first
        pattern = (pattern & 0x000FF00F) |
            ((pf1 & 0x80) >> 3) |
            ((pf1 & 0x40) >> 1) |
            ((pf1 & 0x20) << 1) |
            ((pf1 & 0x10) << 3) |
            ((pf1 & 0x08) << 5) |
            ((pf1 & 0x04) << 7) |
            ((pf1 & 0x02) << 9) |
            ((pf1 & 0x01) << 11);
    }

    void set_pf2(uint8_t pf2)
    {
        pattern = (pattern & 0x00000FFF) | ((uint32_t)pf2 << 12);
    }

    void set_ctrlpf(uint8_t ctrlpf)
    {
        reflect = ctrlpf & Stella::CTRLPF_REFLECT_PLAYFIELD;
    }

    void tick(int visible_x)
    {
        using namespace Stella;

        x = visible_x;

        // The reflect bit is only sampled at the start of each half
        if((x == 0) || (x == visible_pixels / 2 - 1)) {
            reflect_latched = reflect;
        }

        // Each playfield bit is 4 pixels wide
        if(x & 0x03) {
            return;
        }

        int playfield_bit_number = x >> 2;
        if(playfield_bit_number < 20) {
            on = pattern & (1 << playfield_bit_number);
        } else if(reflect_latched) {
            on = pattern & (1 << (39 - playfield_bit_number));
        } else {
            on = pattern & (1 << (playfield_bit_number - 20));
        }
    }
};

struct delayed_write
{
    int clocks;
    uint8_t reg;
    uint8_t data;
};

struct stella 
{
    enum {
        DEBUG_TIA = 0x0001,
        DEBUG_TIMER = 0x0002,
        DEBUG_PIA = 0x0004,
        DEBUG_RAM = 0x0008,
    };
    static constexpr uint32_t debug = 0; // DEBUG_TIA;

    std::array<uint8_t, 128> RAM;
    std::vector<uint8_t> ROM;
    uint16_t ROM_address_mask;
    sysclock& clk;
    uint32_t horizontal_clock = 0;
    uint32_t scanline = 0;
    bool within_hblank = true;
    bool extended_hblank = false;       // HMOVE extends HBLANK by 8 clocks
    bool movement_in_progress = false;  // HMOVE extra clocks are being generated
    int movement_clock = 0;             // 0 through 15 and beyond, compared to HMxx

    // debugging ; delete later
    uint32_t vblank_start_clock;
    uint32_t vblank_start_scanline;
    uint32_t vsync_start_clock;
    uint32_t vsync_start_scanline;

    player_object P0;
    player_object P1;
    missile_object M0;
    missile_object M1;
    ball_object BL;
    playfield_object PF;

    std::vector<delayed_write> delayed_writes;
    enum {
        // Pseudo-registers for the delayed side effects of GRP0 and GRP1 writes
        SHUFFLE_P0 = 0x40,
        SHUFFLE_P1 = 0x41,
        SHUFFLE_BL = 0x42,
    };

    uint32_t interval_timer_subcounter = 2;
    uint32_t interval_timer_prescaler = 1;
    uint32_t interval_timer_counter = 0;
    uint32_t interval_timer = 0;
    bool timer_interrupt = false;

    uint8_t row_buffers[2][Stella::clocks_per_line];
    uint8_t *previous_row;
    uint8_t *current_row;

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

    clk_t paddle_discharge_clock[4];

    uint8_t paddle_value_bit(int paddle)
    {
        bool paddle_discharged = clk > paddle_discharge_clock[paddle];
        uint8_t paddle_bit = paddle_discharged ? 0x00 : 0x80;
        return paddle_bit;
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

    uint8_t tia_write[64];
    uint8_t tia_read[64];
    bool wait_for_hsync = false;
    bool vsync_enabled = false;
    bool mark_cpu_wait = false;

    // A frame is complete when VSYNC ends, or when the line counter wraps
    // without a VSYNC having happened
    uint32_t frames_completed = 0;
    bool vsync_this_frame = false;

    stella(const std::vector<uint8_t>& ROM, sysclock& clock) :
        ROM(std::move(ROM)),
        clk(clock)
    {
        previous_row = row_buffers[1];
        current_row = row_buffers[0];
        using namespace Stella;
        if(ROM.size() == 0x800) {
            ROM_address_mask = 0x7ff;
        } else if(ROM.size() == 0x1000) {
            ROM_address_mask = 0xfff;
        } else {
            std::cout << "dunno about ROM size " << ROM.size() << "\n";
            abort();
        }
        memset(row_buffers, 0, sizeof(row_buffers));
        memset(tia_write, 0, sizeof(tia_write));
        memset(tia_read, 0, sizeof(tia_read));
        PlatformInterface::Start(stereoU8SampleRate, preferredAudioBufferSizeBytes);
        sampling_rate = stereoU8SampleRate;
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
                return paddle_value_bit(reg - INPT0);
            } else if(reg == INPT2) {
                // read latched or unlatched input port 2
                return paddle_value_bit(reg - INPT0);
            } else if(reg == INPT1) {
                // read latched or unlatched input port 1
                return paddle_value_bit(reg - INPT0);
            } else if(reg == INPT0) {
                // read latched or unlatched input port 0
                if(debug & DEBUG_TIA) printf("read INPT0, bit is %d\n", paddle_value_bit(reg - INPT0));
                return paddle_value_bit(reg - INPT0);
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
            // Writes that Stella models as taking effect some clocks after
            // the write go through delay_write() and apply_delayed_write().
            if(reg == VSYNC) {
                if(data & VSYNC_SET) {
                    // printf("VSYNC was enabled at %d, %d\n", horizontal_clock, scanline);
                    vsync_start_clock = horizontal_clock;
                    vsync_start_scanline = scanline;
                    uint32_t vbclocks = ((scanline - vblank_start_scanline + 262) % 262) * 228 + horizontal_clock - vblank_start_clock;
                    // printf("%u clocks in VBLANK before VSYNC, %u lines\n", vbclocks, (vbclocks + 114) / 228);
                    vsync_enabled = true;
                } else {
                    if(vsync_enabled) {
                        // printf("VSYNC was disabled at %d, %d\n", horizontal_clock, scanline);
                        uint32_t clocks = ((scanline - vsync_start_scanline + 262) % 262) * 228 + horizontal_clock - vsync_start_clock;
                        // printf("%u clocks in VSYNC, %u lines\n", clocks, (clocks + 114)/ 228);
                        scanline = 0;
                        // write_screen();
                        vsync_enabled = false;
                        frames_completed++;
                        vsync_this_frame = true;
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
                delay_write(HMCLR, data, 2);
            } else if(reg == HMOVE) {
                delay_write(HMOVE, data, 6);
            } else if(reg == RESMP1) {
                M1.set_resmp(data);
            } else if(reg == RESMP0) {
                M0.set_resmp(data);
            } else if(reg == VDELBL) {
                tia_write[VDELBL] = data;
                BL.delayed = data & VDEL_ENABLED;
            } else if(reg == VDELP1) {
                tia_write[VDELP1] = data;
                P1.delayed = data & VDEL_ENABLED;
            } else if(reg == VDELP0) {
                tia_write[VDELP0] = data;
                P0.delayed = data & VDEL_ENABLED;
            } else if((reg == HMBL) || (reg == HMM1) || (reg == HMM0) || (reg == HMP1) || (reg == HMP0)) {
                delay_write(reg, data, 2);
            } else if((reg == ENABL) || (reg == ENAM1) || (reg == ENAM0)) {
                delay_write(reg, data, 1);
            } else if(reg == GRP1) {
                // Writing GRP1 also latches GRP0 and ENABL for VDELP0 and VDELBL
                delay_write(GRP1, data, 1);
                delay_write(SHUFFLE_P0, 0, 1);
                delay_write(SHUFFLE_BL, 0, 1);
            } else if(reg == GRP0) {
                // Writing GRP0 also latches GRP1 for VDELP1
                delay_write(GRP0, data, 1);
                delay_write(SHUFFLE_P1, 0, 1);
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
                BL.reset_position(reset_counter());
            } else if(reg == RESM1) {
                M1.reset_position(reset_counter(), within_hblank);
            } else if(reg == RESM0) { 
                M0.reset_position(reset_counter(), within_hblank);
            } else if(reg == RESP1) {
                P1.reset_position(reset_counter());
            } else if(reg == RESP0) {
                P0.reset_position(reset_counter());
            } else if((reg == PF2) || (reg == PF1) || (reg == PF0)) {
                delay_write(reg, data, 2);
            } else if((reg == REFP1) || (reg == REFP0)) {
                delay_write(reg, data, 1);
            } else if(reg == CTRLPF) {
                tia_write[CTRLPF] = data;
                PF.set_ctrlpf(data);
                BL.set_ctrlpf(data);
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
                P0.set_nusiz(data);
                M0.set_nusiz(data);
            } else if(reg == NUSIZ1) {
                tia_write[NUSIZ1] = data;
                P1.set_nusiz(data);
                M1.set_nusiz(data);
            } else if(reg == RSYNC) {
                /* ignored, resets hsync for testing */
            } else if(reg == WSYNC) {
                // printf("write %d to WSYNC\n", data); 
                wait_for_hsync = true;
            } else if(reg == VBLANK) {
                if(data & 0x80)
                {
                    for(int paddle = 0; paddle < 4; paddle++)
                    {
                        clk_t c = clk + PlatformInterface::RoGetPaddleValue(paddle) * 228llu * 240 / 65536;
                        paddle_discharge_clock[paddle] = c;
                        // printf("paddle %d discharged at clock %llu, line %llu\n", paddle, c, c / 228);
                    }
                }
                // printf("write %d to VBLANK\n", data); 
                delay_write(VBLANK, data, 1);
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
            // abort();
        }
    }

    // Counter value loaded by RESPx, RESMx, and RESBL, from Stella's TIA::resxCounter()
    uint8_t reset_counter()
    {
        using namespace Stella;

        if(within_hblank) {
            // 73 and later are only within HBLANK when HMOVE extended it
            return (horizontal_clock >= hblank_pixels + 5) ? 158 : 159;
        }
        return 157;
    }

    void delay_write(uint8_t reg, uint8_t data, int clocks)
    {
        delayed_writes.push_back({clocks, reg, data});
    }

    void apply_delayed_write(uint8_t reg, uint8_t data)
    {
        using namespace Stella;

        if(reg == HMOVE) {
            // Apply the motion registers to players, missiles, and ball
            movement_clock = 0;
            movement_in_progress = true;
            extended_hblank = true;
            P0.start_movement();
            P1.start_movement();
            M0.start_movement();
            M1.start_movement();
            BL.start_movement();
        } else if(reg == HMCLR) {
            // Reset all 5 motion registers to 0
            tia_write[HMBL] = 0;
            tia_write[HMM1] = 0;
            tia_write[HMM0] = 0;
            tia_write[HMP1] = 0;
            tia_write[HMP0] = 0;
            P0.set_horizontal_motion(0);
            P1.set_horizontal_motion(0);
            M0.set_horizontal_motion(0);
            M1.set_horizontal_motion(0);
            BL.set_horizontal_motion(0);
        } else if(reg == HMBL) {
            tia_write[HMBL] = data;
            BL.set_horizontal_motion(data);
        } else if(reg == HMM1) {
            tia_write[HMM1] = data;
            M1.set_horizontal_motion(data);
        } else if(reg == HMM0) {
            tia_write[HMM0] = data;
            M0.set_horizontal_motion(data);
        } else if(reg == HMP1) {
            tia_write[HMP1] = data;
            P1.set_horizontal_motion(data);
        } else if(reg == HMP0) {
            tia_write[HMP0] = data;
            P0.set_horizontal_motion(data);
        } else if(reg == ENABL) {
            tia_write[ENABL] = data;
            BL.enabled_new = data & ENABL_ENABLED;
        } else if(reg == ENAM1) {
            tia_write[ENAM1] = data;
            M1.enabled = data & ENABL_ENABLED;
        } else if(reg == ENAM0) {
            tia_write[ENAM0] = data;
            M0.enabled = data & ENABL_ENABLED;
        } else if(reg == GRP1) {
            tia_write[GRP1] = data;
            P1.grp_new = data;
        } else if(reg == GRP0) {
            tia_write[GRP0] = data;
            P0.grp_new = data;
        } else if(reg == SHUFFLE_P0) {
            P0.grp_old = P0.grp_new;
        } else if(reg == SHUFFLE_P1) {
            P1.grp_old = P1.grp_new;
        } else if(reg == SHUFFLE_BL) {
            BL.enabled_old = BL.enabled_new;
        } else if(reg == PF2) {
            tia_write[PF2] = data;
            PF.set_pf2(data);
        } else if(reg == PF1) {
            tia_write[PF1] = data;
            PF.set_pf1(data);
        } else if(reg == PF0) {
            tia_write[PF0] = data;
            PF.set_pf0(data);
        } else if(reg == REFP1) {
            tia_write[REFP1] = data;
            P1.reflected = data & REFP_REFLECT;
        } else if(reg == REFP0) {
            tia_write[REFP0] = data;
            P0.reflected = data & REFP_REFLECT;
        } else if(reg == VBLANK) {
            static bool in_vblank = false;
            tia_write[VBLANK] = data;
            if(data & VBLANK_ENABLED) {
                if(!in_vblank) {
                    // printf("VBLANK was enabled at %d, %d\n", horizontal_clock, scanline);
                    vblank_start_clock = horizontal_clock;
                    vblank_start_scanline = scanline;
                    in_vblank = true;
                }
            } else {
                if(in_vblank) {
                    // printf("VBLANK was disabled at %d, %d\n", horizontal_clock, scanline);
                    uint32_t clocks = ((scanline - vblank_start_scanline + 262) % 262) * 228 + horizontal_clock - vblank_start_clock;
                    // printf("%u clocks in VBLANK, %u lines\n", clocks, (clocks + 114) / 228);
                    in_vblank = false;
                }
            }
        }
    }

    void process_delayed_writes()
    {
        for(auto& w : delayed_writes) {
            w.clocks--;
        }
        size_t i = 0;
        while(i < delayed_writes.size()) {
            if(delayed_writes[i].clocks <= 0) {
                apply_delayed_write(delayed_writes[i].reg, delayed_writes[i].data);
                delayed_writes.erase(delayed_writes.begin() + i);
            } else {
                i++;
            }
        }
    }

    // After HMOVE, every fourth color clock gives objects that haven't
    // yet reached their HMxx count an extra clock, but only during HBLANK.
    void advance_movement()
    {
        if(!movement_in_progress) {
            return;
        }

        if((horizontal_clock & 0x03) == 0) {
            int clock = (movement_clock > 15) ? 0 : movement_clock;

            if(P0.movement_tick(clock) && within_hblank) {
                P0.tick();
            }
            if(P1.movement_tick(clock) && within_hblank) {
                P1.tick();
            }
            if(M0.movement_tick(clock) && within_hblank) {
                M0.tick(horizontal_clock, false);
            }
            if(M1.movement_tick(clock) && within_hblank) {
                M1.tick(horizontal_clock, false);
            }
            if(BL.movement_tick(clock) && within_hblank) {
                BL.tick(false);
            }

            movement_in_progress = P0.is_moving || P1.is_moving || M0.is_moving || M1.is_moving || BL.is_moving;
            movement_clock++;
        }
    }

    uint8_t evaluate_pixel_color()
    {
        using namespace Stella;

        bool within_vblank = tia_write[VBLANK] & VBLANK_ENABLED;
        uint8_t ctrlpf = tia_write[CTRLPF];

        bool pf = PF.on;
        bool p0 = P0.on;
        bool p1 = P1.on;
        bool m0 = M0.on;
        bool m1 = M1.on;
        bool bl = BL.on;

        // In score mode the playfield takes the player colors, left and right
        uint8_t pf_color = tia_write[COLUPF];
        if((ctrlpf & (CTRLPF_SCORE_MODE | CTRLPF_PLAYFIELD_ABOVE)) == CTRLPF_SCORE_MODE) {
            pf_color = (PF.x < visible_pixels / 2) ? tia_write[COLUP0] : tia_write[COLUP1];
        }

        // Priority, from Stella's TIA::renderPixel()
        uint8_t color;
        if(ctrlpf & CTRLPF_PLAYFIELD_ABOVE) {
            // PF/BL over P0/M0 over P1/M1 over BK
            if(pf) {
                color = pf_color;
            } else if(bl) {
                color = tia_write[COLUPF];
            } else if(p0) {
                color = tia_write[COLUP0];
            } else if(m0) {
                color = tia_write[COLUP0];
            } else if(p1) {
                color = tia_write[COLUP1];
            } else if(m1) {
                color = tia_write[COLUP1];
            } else {
                color = tia_write[COLUBK];
            }
        } else if(ctrlpf & CTRLPF_SCORE_MODE) {
            // P0/M0 over PF over P1/M1 over BL over BK
            if(p0) {
                color = tia_write[COLUP0];
            } else if(m0) {
                color = tia_write[COLUP0];
            } else if(pf) {
                color = pf_color;
            } else if(p1) {
                color = tia_write[COLUP1];
            } else if(m1) {
                color = tia_write[COLUP1];
            } else if(bl) {
                color = tia_write[COLUPF];
            } else {
                color = tia_write[COLUBK];
            }
        } else {
            // P0/M0 over P1/M1 over PF/BL over BK
            if(p0) {
                color = tia_write[COLUP0];
            } else if(m0) {
                color = tia_write[COLUP0];
            } else if(p1) {
                color = tia_write[COLUP1];
            } else if(m1) {
                color = tia_write[COLUP1];
            } else if(pf) {
                color = pf_color;
            } else if(bl) {
                color = tia_write[COLUPF];
            } else {
                color = tia_write[COLUBK];
            }
        }

        // Collision latches are not set during VBLANK
        if(!within_vblank) {
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
        }

        if(within_vblank) {
            return 0x00; // BLACK
        }

        return color;
    }

    clk_t last_pixel_clocked;

    // Must be called once and only once for every clock value
    // I.e. clk must be incrementing on each call
    bool advance_one_clock(bool mark_cpu_wait)
    {
        using namespace Stella;

        // The order of operations within a color clock follows Stella's
        // TIA::cycle().  Leave horizontal_clock and scanline until the end
        // since other operations use them.

        process_delayed_writes();

        if(horizontal_clock == 0) {
            // An HMOVE that lands here or later doesn't extend this line's HBLANK
            extended_hblank = false;
        }

        advance_movement();

        advance_interval_timer();

        advance_sound_clock();

        int color;

        if(within_hblank) {
            // Keep the playfield going through an HMOVE-extended HBLANK
            if(horizontal_clock >= hblank_pixels) {
                PF.tick(horizontal_clock - hblank_pixels);
            }
            color = 0x00;
        } else {
            PF.tick(horizontal_clock - hblank_pixels);
            P0.tick();
            P1.tick();
            M0.tick(horizontal_clock, true);
            M0.resmp_tick(P0);
            M1.tick(horizontal_clock, true);
            M1.resmp_tick(P1);
            BL.tick(true);
            color = evaluate_pixel_color();
        }

        if(mark_cpu_wait) {
            color = 0x0F;
        }

        current_row[horizontal_clock] = color;

        // HBLANK ends after clock 67, or after clock 75 when HMOVE extended it
        if((horizontal_clock == hblank_pixels - 1) && !extended_hblank) {
            within_hblank = false;
        }
        if((horizontal_clock == hblank_pixels + 7) && extended_hblank) {
            within_hblank = false;
        }

        // And then move forward the horizontal clock and scanline

        horizontal_clock++;
        if(horizontal_clock >= clocks_per_line) {
            horizontal_clock = 0;
            within_hblank = true;
            scanline++;
            std::swap(previous_row, current_row);
            if(scanline >= lines_per_frame) {
                scanline = 0;
                if(!vsync_this_frame) {
                    frames_completed++;
                }
                vsync_this_frame = false;
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
        fprintf(stderr, "usage: %s cartridge-rom-file [frame-count screenshot.ppm]\n", argv[0]);
        fprintf(stderr, "    with frame-count and screenshot.ppm, run that many frames, write the screen to the file, and exit\n");
        exit(EXIT_FAILURE);
    }
    int screenshot_frame = -1;
    const char *screenshot_filename = nullptr;
    if(argc >= 4) {
        screenshot_frame = atoi(argv[2]);
        screenshot_filename = argv[3];
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

    static uint8_t screen[228 * 262];
    int frame_count = 0;

    auto end_of_frame = [&]() {
        PlatformInterface::Frame(screen, 1.0f);
        frame_count++;
        if(frame_count == screenshot_frame) {
            write_screen(screen, screenshot_filename);
            exit(EXIT_SUCCESS);
        }
    };

    while(1) {
        if(false) {
            std::string dis = read_bus_and_disassemble(hw, cpu.pc);
            printf("%10llu %4u %s\n", (clk_t)clk, hw.horizontal_clock, dis.c_str());
            printf("PC: %4X, A: %02X, X: %02X, Y: %02X, S: %02X, P: %02X\n", cpu.pc, cpu.a, cpu.x, cpu.y, cpu.s, cpu.p);
        }
        auto previous_line = hw.scanline;
        auto previous_frames_completed = hw.frames_completed;
        cpu.cycle();
        if(hw.scanline != previous_line) {
            memcpy(screen + Stella::clocks_per_line * previous_line, hw.previous_row, Stella::clocks_per_line);
        }
        if(hw.frames_completed != previous_frames_completed) {
            end_of_frame();
        }
        // printf("clk = %llu\n", (clk_t)clk);
        if(hw.wait_for_hsync) {
            int current_line = hw.scanline;
            auto cycles = hw.advance_to_hsync(clk);
            clk.add_pixel_cycles(cycles);
            memcpy(screen + Stella::clocks_per_line * current_line, hw.previous_row, Stella::clocks_per_line);
            if(hw.frames_completed != previous_frames_completed) {
                end_of_frame();
            }
        }// else {
          //  hw.advance_to_clock(clk);
        //}
    }
}
