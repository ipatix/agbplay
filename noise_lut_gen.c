#include <stdio.h>
#include <stdbool.h>

#define FINE_NOISE

static int reg;
static bool noise_gen() {
    if ((reg & 1) == 1) {
        reg >>= 1;
#ifdef FINE_NOISE
        reg ^= 0x6000;
#else
        reg ^= 0x60;
#endif
        return true;
    } else {
        reg >>= 1;
        return false;
    }
}

int main(void) {
#ifdef FINE_NOISE
    reg = 0x4000;
    int count = 0x8000;
#else
    reg = 0x40;
    int count = 0x80;
#endif

    printf("const float lut[] = {\n");
    for (int i = count; i > 0; i -= 0x20) {
        printf("    ");
        if (i > 0x20) {
            for (int j = 0x20; j > 0; j--) {
                printf(noise_gen() ? "0.5f, " : "-0.5f, ");
            }
        } else {
            for (int j = 0x1F; j > 0; j--) {
                printf(noise_gen() ? "0.5f, " : "-0.5f, ");
            }
            printf(noise_gen() ? "0.5f" : "-0.5f");
        }
        printf("\n");
    }
    printf("};\n");
};
