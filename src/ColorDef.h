#pragma once

namespace agbplay
{
    // color sets
    enum class Color : int {
        DEF_DEF = 1,    // default on default

        BANNER_TEXT,
        WINDOW_FRAME,

        LIST_ENTRY,
        LIST_SEL,

        VU_LOW,
        VU_MID,
        VU_HIGH,

        TRK_NUM,
        TRK_NUM_MUTED,
        TRK_LOC,
        TRK_LOC_CALL,
        TRK_DEL,
        TRK_NOTE,
        TRK_VOICE,
        TRK_PAN,
        TRK_VOL,
        TRK_MOD,
        TRK_PITCH,
        TRK_LOUDNESS,
        TRK_LOUDNESS_MUTED,
        TRK_LOUD_SPLIT,

        TRK_FGB_BGCW,   // C not pressed, C# not pressed
        TRK_FGC_BGCW,   // C not pressed, C# pressed

        TRK_FGB_BGW,    // D not pressed, D# not pressed
        TRK_FGC_BGW,    // D not pressed, D# pressed
        TRK_FGB_BGC,    // D pressed, D# not pressed
        TRK_FGC_BGC,    // D pressed, D# pressed

        TRK_FGW_BGW,    // E not pressed, F not pressed
        TRK_FGEC_BGW,   // E not pressed, F pressed
        TRK_FGW_BGC,    // E pressed, F not pressed
        TRK_FGEC_BGC,   // E pressed, F pressed
    };
}
