#include "CGBPatterns.h"

using namespace agbplay;

// square wave LUT

const float CGBPatterns::pat_sq12[] = {
    0.875f, -0.125f, -0.125f, -0.125f, -0.125f, -0.125f, -0.125f, -0.125f
};
const float CGBPatterns::pat_sq25[] = {
    0.75f, 0.75f, -0.25f, -0.25f, -0.25f, -0.25f, -0.25f, -0.25f
};
const float CGBPatterns::pat_sq50[] = {
    0.50f, 0.50f, 0.50f, 0.50f, -0.50f, -0.50f, -0.50f, -0.50f
};
const float CGBPatterns::pat_sq75[] = {
    0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, -0.75, -0.75f
};

const uint8_t CGBPatterns::NoiseKeyToFreqLUT[] =
{
    0xD7, 0xD6, 0xD5, 0xD4,
    0xC7, 0xC6, 0xC5, 0xC4,
    0xB7, 0xB6, 0xB5, 0xB4,
    0xA7, 0xA6, 0xA5, 0xA4,
    0x97, 0x96, 0x95, 0x94,
    0x87, 0x86, 0x85, 0x84,
    0x77, 0x76, 0x75, 0x74,
    0x67, 0x66, 0x65, 0x64,
    0x57, 0x56, 0x55, 0x54,
    0x47, 0x46, 0x45, 0x44,
    0x37, 0x36, 0x35, 0x34,
    0x27, 0x26, 0x25, 0x24,
    0x17, 0x16, 0x15, 0x14,
    0x07, 0x06, 0x05, 0x04,
    0x03, 0x02, 0x01, 0x00,
};

const std::bitset<NOISE_FINE_LEN> CGBPatterns::pat_noise_fine = []() {
    std::bitset<NOISE_FINE_LEN> patt;
    int reg = 0x4000;
    for (size_t i = 0; i < NOISE_FINE_LEN; i++) {
        if ((reg & 1) == 1) {
            reg >>= 1;
            reg ^= 0x6000;
            patt[i] = true;
        } else {
            reg >>= 1;
            patt[i] = false;
        }
    }
    return patt;
}();

const std::bitset<NOISE_ROUGH_LEN> CGBPatterns::pat_noise_rough = []() {
    std::bitset<NOISE_ROUGH_LEN> patt;
    int reg = 0x40;
    for (size_t i = 0; i < NOISE_ROUGH_LEN; i++) {
        if ((reg & 1) == 1) {
            reg >>= 1;
            reg ^= 0x60;
            patt[i] = true;
        } else {
            reg >>= 1;
            patt[i] = false;
        }
    }
    return patt;
}();
