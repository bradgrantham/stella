#include <cstdio>
#include <cinttypes>

#include "stella.h"

typedef uint64_t clk_t;

/*
concept
    two-phase clock
    activate and deactivate each clock in turn
    logic inputs and logic outputs
    do topo sort?  Or is it implicit?  I think it's implicit.
    does the circuit get "evaluate()" or app just writes an input and the outputs are recalculated?
        May be lots of recalculation
        if writes to an input cause recalculation of output, then inputs need to be functional
    phi0 = 1 ; evaluate ; phi0 = 0 ; evaluate? ; phi1 = 1 ; evaluate ; phi1 = 0; evaluate?
    but there is a loop, so what happens then?  Break the loop at input to shift register, i.e. force it to be last in topologic sort?
    many components don't have clock as input, so evaluate always evaluates
    this may not be fast enough for real time.
    can all components be templates?  need sized output 
        logic_component<int OUTPUTCOUNT>
            logic_component(std::array<logic_component&> outputs) // may need to be pointers
            set_outputs(bool value) - loops over outputs
    if all are set as outputs and components have implicit inputs, need to build the network backwards
    There are 60+ components - ideally there would be a very compact representation, like
        // q2, a2
        lc::circuit audio;
        lc::di q1(audio, {a2.input0, q2.input});
        lc::and a0(audio, {)
        lc::and a1(audio, )
        lc::input phi0(audio, {q1.phi0, ...})
        lc_inv i0(audio, {a1.input0, a2.input0}) // sets i0's outputs to c1, c2 on evaluation
        phi0.input = true; audio.evaluate(); phi0.input = false; audio.evaluate(); phi1.input = true ...
            then read bare output of q9 for audio bit
        but then what to do about that last input to the shift register? - copy it by hand from output to input, so q1's input won't have any explicit output.


component types:
    phase-clocked DI (need to break down into components?)
    n-XOR
    inverter
    S'R' flipflop
    n-AND (frequently 2 but more in case of matrix wire-ands)
    audio control reg (need to break down into components?)
        input is a bool set directly
    one tri-state inverter
*/

struct TIAChannel
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

uint8_t TIAChannel::advance_audio_clock(uint8_t AUDC)
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

uint8_t TIAChannel::advance_clock(uint8_t AUDV, uint8_t AUDF, uint8_t AUDC, int& counter)
{
    using namespace Stella;

    // Does writing new AUDF reset the counter?  Or is it only used to reload the counter?
    if(counter > 0) {
        counter--;
    } else {
        sound_bit = advance_audio_clock(AUDC);
        counter = AUDF;
    }

    return 128 + (sound_bit ? -128 : 127 ) * AUDV / 15;
}

uint8_t tia_write[64];
int audio_counter[2] = {0, 0};
uint64_t next_sample_index = 0;
uint8_t audio_levels[2] = {128, 128};
clk_t next_sample_clock = 0;
uint32_t sampling_rate = 44100;
clk_t previous_audio_processing_clock = 0;
clk_t next_audio_clock = 0;
clk_t clock_rate = 3579540;
static constexpr clk_t video_clocks_per_audio_clock = 114;
TIAChannel channels[2];

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
        audio_levels[0] = channels[0].advance_clock(tia_write[AUDV0], tia_write[AUDF0], tia_write[AUDC0], audio_counter[0]);
        audio_levels[1] = channels[1].advance_clock(tia_write[AUDV1], tia_write[AUDF1], tia_write[AUDC1], audio_counter[1]);
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
