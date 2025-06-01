#include "ed25519.h"
#include "pico/rand.h"
#ifndef ED25519_NO_SEED

int ed25519_create_seed(unsigned char *seed) {
    for (int i = 0; i < 8; i++) {
        uint32_t r = get_rand_32();
        seed[i * 4]     = (r >> 0) & 0xFF;
        seed[i * 4 + 1] = (r >> 8) & 0xFF;
        seed[i * 4 + 2] = (r >> 16) & 0xFF;
        seed[i * 4 + 3] = (r >> 24) & 0xFF;
    }
    return 0;
}
#endif
