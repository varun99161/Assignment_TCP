
#include "../src/hash.h"
#include <assert.h>
#include <string.h>

int main() {
    uint8_t h1[32], h2[32];
    sha256((uint8_t*)"test", 4, h1);
    sha256((uint8_t*)"test", 4, h2);
    assert(memcmp(h1, h2, 32) == 0);
    return 0;
}
