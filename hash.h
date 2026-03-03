#ifndef HASH_H
#define HASH_H

/**
 * @file hash.h
 * @brief Minimal SHA-256 hashing interface.
 *
 * Exposes a single helper function to compute a 32-byte SHA‑256 digest
 * for a given input buffer. The implementation can be backed by OpenSSL
 * (e.g., SHA256()) or any compatible provider.
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Compute the SHA‑256 hash of the input buffer.
 *
 * Performs a one-shot SHA‑256 digest of the bytes at @p data with length @p len,
 * writing the 32-byte result into @p out.
 *
 * @param data Pointer to the input bytes (can be NULL if @p len is 0).
 * @param len  Number of input bytes to hash.
 * @param out  Pointer to a buffer of at least 32 bytes; receives the digest.
 *
 * @note This function does not allocate memory and always writes 32 bytes to @p out.
 * @note The caller owns @p out and must ensure it points to valid writable memory.
 */
void sha256(const uint8_t *data, size_t len, uint8_t out[32]);

#endif /* HASH_H */

