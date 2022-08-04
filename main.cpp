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
};

typedef uint64_t clk_t;

clk_t vsync_reset_clock = 0;

struct sysclock // If I called this "clock" XCode would error out because I shadowed MacOSX's "clock"
{
    clk_t clock;
    operator clk_t() const { return clock; }
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

    uint8_t tia_write[64];
    uint8_t tia_read[64];

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
            }
        } else if(isPIA(addr)) {
            printf("read from PIA %04X\n", addr);
            addr &= 0x1F;
            if(addr == SWCHB) {
                return 0x0;
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

void scanout_to_current_clock(sysclock& clk, stella &hw)
{
    static constexpr uint32_t vsync_lines = 3;
    static constexpr uint32_t vblank_lines = 37;
    static constexpr uint32_t visible_line_start = (vsync_lines + vblank_lines);
    static constexpr uint32_t visible_lines = 192;
    static constexpr uint32_t hblank_pixels = 68;
    static constexpr uint32_t visible_pixels = 160;
    static constexpr uint32_t pixels_per_line = (hblank_pixels + visible_pixels);

    while(last_pixel_clocked < clk - 1) {
        uint32_t clock_within_frame = (last_pixel_clocked - vsync_reset_clock);
        uint32_t scanout_line = clock_within_frame / pixels_per_line;
        uint32_t scanout_pixel = clock_within_frame % pixels_per_line;
        if((scanout_line >= visible_line_start) && (scanout_line < visible_line_start + visible_lines)) {
            uint32_t visible_line = scanout_line - visible_line_start;
            if((scanout_pixel >= hblank_pixels) && (scanout_pixel < hblank_pixels + visible_pixels)) {
                uint32_t visible_pixel = scanout_pixel - hblank_pixels;
                printf("%d %d\n", visible_pixel, visible_line);
            }
        }
        last_pixel_clocked++;
    }
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
        scanout_to_current_clock(clk, hw);
    }
}
