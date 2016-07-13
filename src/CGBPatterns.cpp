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
