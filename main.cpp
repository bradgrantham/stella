#include <vector>
#include <array>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include "cpu6502.h"
#include "dis6502.h"


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

        // TIA read
         CXM0P = 0x30,
         CXM1P = 0x31,
         CXP0FB = 0x32,
         CXP1FB = 0x33,
         CXM0FB = 0x34,
         CXM1FB = 0x35,
         CXBLPF = 0x36,
         CXPPMM = 0x37,
         INPT0 = 0x38,
         INPT1 = 0x39,
         INPT2 = 0x3A,
         INPT3 = 0x3B,
         INPT4 = 0x3C,
         INPT5 = 0x3D,

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

};

typedef uint64_t clk_t;

clk_t vsync_reset_clock = 0;

struct sysclock // If I called this "clock" XCode would error out because I shadowed MacOSX's "clock"
{
    clk_t clock;
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
    std::array<uint8_t, 128> RAM;
    std::vector<uint8_t> ROM;
    uint16_t ROM_address_mask;
    sysclock& clk;

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
        printf("timer now %d\n", interval_timer);
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
            printf("read %02X from RAM %04X\n", data, addr);
            return data;
        } else if(isTIA(addr)) {
            printf("read from TIA %04X\n", addr);
            addr &= 0x3F;
            if(addr == INPT5) {
                // read latched or unlatched input port 5
                return 0x0;
            } else if(addr == INPT4) {
                // read latched or unlatched input port 4
                return 0x0;
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
            printf("read from PIA %04X\n", addr);
            addr &= 0x1F;
            if(addr == SWCHB) {
                return 0x0;
            } else if(addr == INTIM) {
                uint8_t data = interval_timer / interval_timer_prescaler;
                printf("read interval timer, %2X\n", data);
                return data;
            } else if(addr == SWCHA) {
                printf("read joystick bits\n");
                return 0x0;
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
            printf("wrote %02X to RAM %04X\n", data, addr);
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
            printf("wrote %02X to TIA %04X\n", data, addr);
            addr &= 0x3F;
            if(addr == VSYNC) {
                if(data == 0x1) {
                    vsync_reset_clock = clk;
                }
            } else if(addr == CXCLR) {
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
            } else if(addr == RESMP1) {
                // XXX reset missle 1 to player 1 and hide missle
            } else if(addr == RESMP0) {
                // XXX reset missle 0 to player 0 and hide missle
                tia_write[RESMP0] = data;
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
                // XXX set ball to current horizontal
            } else if(addr == RESM1) {
                // XXX set missile 1 to current horizontal
            } else if(addr == RESM0) { 
                // XXX set missile 0 to current horizontal
            } else if(addr == RESP1) {
                // XXX set player 1 to current horizontal
            } else if(addr == RESP0) {
                // XXX set player 0 to current horizontal
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
            } else if(addr == RSYNC) { printf("write %d to RSYNC\n", data); 
            } else if(addr == WSYNC) { printf("write %d to WSYNC\n", data); 
                wait_for_hsync = true;
            } else if(addr == VBLANK) { printf("write %d to VBLANK\n", data); 
            } else if(addr == 0x2D) {
                // ignore
            } else if(addr == 0x2E) {
                // ignore
            } else if(addr == 0x2F) {
                // ignore
            } else if((addr >= 0x30) && (addr <= 0x3F)) {
                // ignore
            }
        } else {
            printf("unhandled write of %02X to %04X\n", data, addr);
            abort();
        }
    }
};

clk_t last_pixel_clocked;

uint8_t screen[160 * 192];

void set_colu(int x, int y, uint8_t colu)
{
    using namespace Stella;
    screen[x + y * visible_pixels] = colu;
}

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

void write_screen()
{
    using namespace Stella;

    static int frame_number = 0;
    static char filename[512];

    sprintf(filename, "image%05d.ppm", frame_number);
    FILE *screenfile = fopen(filename, "wb");
    fprintf(screenfile, "P6 %d %d 255\n", visible_pixels, visible_lines);
    for(int y = 0; y < visible_lines; y++) {
        for(int x = 0; x < visible_pixels; x++) {
            uint8_t colu = screen[x + y * visible_pixels];
            uint8_t *rgb = colu_to_rgb[colu];
            fwrite(rgb, 3, 1, screenfile);
        }
    }
    fclose(screenfile);
    printf("wrote image %s\n", filename);

    frame_number++;
}

// Must be called once and only once for every clock value
// I.e. clk must be incrementing on each call
bool advance_one_pixel(stella &hw)
{
    using namespace Stella;
    uint32_t clock_within_frame = (last_pixel_clocked - vsync_reset_clock);
    uint32_t scanout_line = (clock_within_frame / pixels_per_line) % lines_per_frame;
    uint32_t scanout_pixel = clock_within_frame % pixels_per_line;

    bool hblank_started = (scanout_pixel == 0);

    if((scanout_line >= visible_line_start) && (scanout_line < visible_line_start + visible_lines)) {
        uint32_t visible_line = scanout_line - visible_line_start;
        if((scanout_pixel >= hblank_pixels) && (scanout_pixel < hblank_pixels + visible_pixels)) {
            uint32_t visible_pixel = scanout_pixel - hblank_pixels;
            printf("pixel %d %d\n", visible_pixel, visible_line);
            set_colu(visible_pixel, visible_line, hw.tia_write[COLUBK]); // XXX process rest of registers
            if((visible_pixel == visible_pixels - 1) && (visible_line == visible_lines - 1)) {
                write_screen();
            }
        }
    }

    return hblank_started;
}

void scanout_to_current_clock(const sysclock& clk, stella &hw)
{
    using namespace Stella;
    for(clk_t c = last_pixel_clocked; c < clk; c++) {
        advance_one_pixel(hw);
    }
    last_pixel_clocked = clk - 1;
}

clk_t scanout_to_hsync(const sysclock& clk, stella &hw)
{
    using namespace Stella;
    printf("scanout to hsync\n");
    bool hsync_started;
    do {
        hsync_started = advance_one_pixel(hw);
        last_pixel_clocked++;
    } while(!hsync_started);
    return last_pixel_clocked - clk;
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
    create_colormap();

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
        printf("%s\n", dis.c_str());
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
