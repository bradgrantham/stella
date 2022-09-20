#include <cinttypes>

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
        VBLANK_ENABLED = 0x02,

        REFP_REFLECT = 0x08,

        CTRLPF_REFLECT_PLAYFIELD = 0x01,
        CTRLPF_SCORE_MODE = 0x02,
        CTRLPF_PLAYFIELD_ABOVE = 0x04,

        ENABL_ENABLED = 0x01,

        VDEL_ENABLED = 0x01,

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
        SWCHA_JOYSTICK1_RIGHT = 0x08,
        INPT5_JOYSTICK1_BUTTON = 0x80,

        SWCHB_RESET_SWITCH = 0x01,
        SWCHB_SELECT_SWITCH = 0x02,
        SWCHB_TVTYPE_SWITCH = 0x08,
        SWCHB_P0_DIFFICULTY_SWITCH = 0x40,
        SWCHB_P1_DIFFICULTY_SWITCH = 0x80,
    };

    static constexpr uint32_t vsync_lines = 3;
    static constexpr uint32_t vblank_lines = 37;
    // static constexpr uint32_t visible_line_start = (vsync_lines + vblank_lines);
    static constexpr uint32_t visible_lines = 192;
    static constexpr uint32_t overscan_lines = 30;
    static constexpr uint32_t lines_per_frame = (vsync_lines + vblank_lines + visible_lines + overscan_lines);
    static constexpr uint32_t hblank_pixels = 68;
    static constexpr uint32_t visible_pixels = 160;
    static constexpr uint32_t clocks_per_line = (hblank_pixels + visible_pixels);
    // static constexpr uint32_t clocks_per_frame = lines_per_frame * clocks_per_line;

    int get_signed_move(uint8_t HM)
    {
        int motion = HM >> 4U;
        if(motion > 7) {
            motion -= 16;
        }
        return motion;
    }

};

