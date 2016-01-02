#include <stdio.h>
#include <stdbool.h>

static int reg;
static bool noise_gen() {
    if ((reg & 1) == 1) {
        reg >>= 1;
        reg ^= 0x60;
        return true;
    } else {
        reg >>= 1;
        return false;
    }
}

int main(void) {
    reg = 0x40;
    int count = 0x80;

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
