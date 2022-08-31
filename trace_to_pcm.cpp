#include <cstdio>
#include <cinttypes>

#include "stella.h"

typedef uint64_t clk_t;

uint8_t advance_channel_clock(uint8_t AUDV, uint8_t AUDF, uint8_t AUDC, int& counter, uint16_t& bitstream)
{
    using namespace Stella;

    // Does writing new AUDF reset the counter?  Or is it only used to reload the counter?
    if(counter > 0) {
        counter--;
    } else {
        counter = AUDF;
        // Do polys
    }

    // Do polynomial based on AUDC
    int noise_length; // 4, 5, 9
    int z; // 0 or 1
    switch(AUDC) {
        case 0xB:
        case 0x0:
        {
            // Does this actually shift 1 in, or does it just force the output to 1?
            z = 1;
            noise_length = 4;
            break;
        }
        case 0x1:
        {
            noise_length = 4;
            z = ((bitstream >> 6) ^ (bitstream >> 5)) & 0x1;
            break;
        }
        case 0x2:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0x3:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0x4:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0x5:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0x6:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0x8:
        {
            noise_length = 9;
            z = ((bitstream >> 4) ^ (bitstream >> 0)) & 0x1;
            break;
        }
        case 0x7:
        case 0x9:
        {
            noise_length = 5;
            z = ((bitstream >> 6) ^ (bitstream >> 4)) & 0x1;
            break;
        }
        case 0xA:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0xC:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0xD:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0xE:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
        case 0xF:
            // XXX TODO
            noise_length = 4;
            z = 1;
            break;
    }
    int noise_bit = 9 - noise_length;
    int sound_bit = (bitstream >> noise_bit) & 0x01;

    // Does this need to be applied using an additional counter?
    bitstream = (z << 8) | (bitstream >> 1);

    return 128 + (sound_bit ? -128 : 127 ) * 15 / AUDV;
}

uint8_t tia_write[64];
int audio_counter[2] = {0, 0};
uint16_t audio_bitstream[2] = {0, 0};
uint64_t next_sample_index = 0;
uint8_t audio_levels[2] = {128, 128};
clk_t next_sample_clock = 0;
uint32_t sampling_rate = 44100;
clk_t previous_audio_processing_clock = 0;
clk_t next_audio_clock = 0;
clk_t clock_rate = 3579540;
static constexpr clk_t video_clocks_per_audio_clock = 114;

void emit_samples(uint8_t channel1, uint8_t channel2)
{
    // XXX TODO Enqueue and flush using PlatformInterface
    fwrite(&channel1, 1, 1, stdout);
    fwrite(&channel2, 1, 1, stdout);
}

void advance_audio_clock()
{
    using namespace Stella;
    clk_t current_audio_processing_clock = previous_audio_processing_clock + 1;

    // fprintf(stderr, "%llu, %llu, %llu\n", current_audio_processing_clock, next_audio_clock, next_sample_clock);

    if(next_audio_clock < current_audio_processing_clock) {
        audio_levels[0] = advance_channel_clock(tia_write[AUDV0], tia_write[AUDF0], tia_write[AUDC0], audio_counter[0], audio_bitstream[0]);
        audio_levels[1] = advance_channel_clock(tia_write[AUDV1], tia_write[AUDF1], tia_write[AUDC1], audio_counter[1], audio_bitstream[1]);
        next_audio_clock = current_audio_processing_clock + video_clocks_per_audio_clock;
    }

    if(next_sample_clock < current_audio_processing_clock) {
        emit_samples(audio_levels[0], audio_levels[1]);
        next_sample_index ++;
        next_sample_clock = next_sample_index * clock_rate / sampling_rate;
    }

    previous_audio_processing_clock = current_audio_processing_clock;
}


int main(int argc, const char **argv)
{
    clk_t current_clock = 0;

    unsigned char regname[512];
    clk_t next_write_clock;
    uint32_t next_write_address;
    uint32_t next_write_value;

    while(scanf("%[^,],%llu,%d,%d ", regname, &next_write_clock, &next_write_address, &next_write_value) == 4) {
        while(current_clock < next_write_clock) {
            advance_audio_clock();
            current_clock++;
        }
        tia_write[next_write_address] = next_write_value;
    }
}
