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
    constexpr static uint6_t ROMbase = 0xF000;
    constexpr static uint6_t RAMbase = 0x0080;
    constexpr static uint6_t RAMsize = 0x0080;
};

typedef uint64_t clk_t;

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
    std::array<128> RAM;
    std::vector<uint8_t> ROM;
    uint16_t ROM_address_mask;

    stella(const std::vector<uint8_t>& ROM) :
        ROM(std::move(ROM))
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

    uint8_t read(uint16_t addr)
    {
        using namespace Stella;
        if(addr >= ROMbase) {
            uint8_t data = ROM.at(addr & ROM_address_mask);
            // printf("read %02X from ROM %04X\n", data, addr);
            return data;
        }
        if((addr >= RAMbase) && (addr <= RAMbase + RAMsize)) {
            uint8_t data = RAM.at(addr - RAMbase);
            // printf("read %02X from ROM %04X\n", data, addr);
            return data;
        }
        printf("unhandled read from %04X\n", addr);
        abort();
    }

    void write(uint16_t addr, uint8_t data)
    {
        using namespace Stella;
        printf("unhandled write of %02X to %04X\n", data, addr);
        abort();
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

    stella hw(ROM);
    sysclock clk;
    CPU6502 cpu(clk, hw);
    cpu.reset();
    while(1) {
        std::string dis = read_bus_and_disassemble(hw, cpu.pc);
        printf("%s\n", dis.c_str());
        printf("CPU PC = %04X\n", cpu.pc);
        cpu.cycle();
    }
}
