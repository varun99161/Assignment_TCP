/**
 * @file hash.c
 * @brief SHA-256 hashing implementation using OpenSSL.
 *
 * This file provides a simple wrapper around OpenSSL's SHA256() function
 * to compute a 32-byte SHA‑256 digest for a given input buffer.
 */

#include "hash.h"
#include <openssl/sha.h>

/**
 * @brief Compute SHA‑256 hash of input data.
 *
 * This function is a thin wrapper around `SHA256()` from OpenSSL.  
 * It performs a complete one-shot SHA‑256 digest on the provided buffer.
 *
 * @param data  Pointer to input bytes.
 * @param len   Number of bytes in the input buffer.
 * @param out   Output buffer that must be at least 32 bytes.
 *              The resulting SHA‑256 digest is stored here.
 *
 * @note The caller must ensure `out` has space for exactly 32 bytes.
 * @note This function does not allocate memory, does not fail,
 *       and always writes exactly 32 bytes to `out`.
 */
void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    SHA256(data, len, out);
}
