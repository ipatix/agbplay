#pragma once

#define NUM_NOTES 128
#define PROG_UNDEFINED 0xFF
#define UNKNOWN_TABLE 0
#define MIN_SONG_NUM 20

#define BPM_PER_FRAME 150

#define N_CHANNELS 2
#define STREAM_SAMPLERATE 48000

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
