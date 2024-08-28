#include <numbers>
#include <cmath>
#include <fmt/core.h>

template<typename OT, typename IT>
struct NotchFilter {
    void setParameters(IT samplerate, IT notchFreq, IT Q) {
        const IT omega0 = 2.0f * std::numbers::pi_v<IT> * notchFreq / samplerate;
        const IT alpha = std::sin(omega0) * std::sinh(0.5f / Q);

        a0 = 1.0f + alpha;
        a1 = -2.0f * std::cos(omega0);
        a2 = 1.0f - alpha;
        b0 = 1.0f;
        b1 = -2.0f * std::cos(omega0);
        b2 = 1.0f;

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1.0f;

        zff_1 = 0.0f;
        zff_2 = 0.0f;
        zfb_1 = 0.0f;
        zfb_2 = 0.0f;
    }

    OT process(OT input) {
        // biquad: direct form ii: transposed 
        const IT output = input * b0 + zfb_1;
        zfb_1 = input * b1 + zfb_2 - a1 * output;
        zfb_2 = input * b2 - a2 * output;
        //const T output = std::fma(input, b0, zfb_1);
        //zfb_1 = std::fma(input, b1, std::fma(-a1, output, zfb_2));
        //zfb_2 = std::fma(input, b2, -a2 * output);
        return static_cast<OT>(output);
    }

    IT a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    IT b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    IT zff_1 = 0.0f, zff_2 = 0.0f;
    IT zfb_1 = 0.0f, zfb_2 = 0.0f;
};
