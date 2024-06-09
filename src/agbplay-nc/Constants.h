#pragma once

#define NUM_NOTES 128
#define PROG_UNDEFINED 0xFF
#define MAX_TRACKS 32       // on real GBA this is 16 only, let's do homebrew a favor

#define BPM_PER_FRAME 150
#define AGB_FPS 60

// for increased quality we process in subframes (including the base frame)
#define INTERFRAMES 4

#define STREAM_SAMPLERATE 48000
#define SONG_FADE_OUT_TIME 10000
#define SONG_FINISH_TIME 1000

#define __STRM_BPSM ((STREAM_SAMPLERATE / 24) - 1)
#define __STRM_BSA (__STRM_BPSM | (__STRM_BPSM >> 1))
#define __STRM_BSB (__STRM_BSA | (__STRM_BSA >> 2))
#define __STRM_BSC (__STRM_BSB | (__STRM_BSB >> 4))
#define __STRM_BSD (__STRM_BSC | (__STRM_BSC >> 8))
#define __STRM_BSE (__STRM_BSD | (__STRM_BSD >> 16))

#define STREAM_BUF_SIZE (__STRM_BSE+1)

#define WINDOW_MIN_WIDTH 80
#define WINDOW_MIN_HEIGHT 24
#define WINDOW_MAX_WIDTH 512
#define WINDOW_MAX_HEIGHT 128
