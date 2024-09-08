#pragma once

#define NUM_NOTES      128
#define PROG_UNDEFINED 0xFF
#define MAX_TRACKS     16

#define BPM_PER_FRAME 150
#define AGB_FPS       60

// for increased quality we process in subframes (including the base frame)
#define INTERFRAMES 4

#define STREAM_SAMPLERATE  48000
#define SONG_FADE_OUT_TIME 10000
#define SONG_FINISH_TIME   1000

#define WINDOW_MIN_WIDTH  80
#define WINDOW_MIN_HEIGHT 24
#define WINDOW_MAX_WIDTH  512
#define WINDOW_MAX_HEIGHT 128
