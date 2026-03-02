
#include "hash.h"
#include <openssl/sha.h>

void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    SHA256(data, len, out);
}
