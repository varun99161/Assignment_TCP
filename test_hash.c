#include "hash.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static void dump_hex(const uint8_t *h) {
    for (int i = 0; i < 32; i++)
        printf("%02x", h[i]);
    printf("\n");
}

int main() {

    uint8_t out[32];
    int failures = 0;

    // ============================
    // Test 1
    // ============================
    uint8_t expected_abc[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,
        0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,
        0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };

    sha256((uint8_t*)"abc", 3, out);

    printf("Test 1: abc\n");
    printf("Computed : "); dump_hex(out);
    printf("Expected : "); dump_hex(expected_abc);
    if (memcmp(out, expected_abc, 32) == 0) {
        printf("Result   : PASS\n\n");
    } else {
        printf("Result   : FAIL\n\n");
        failures++;
    }

    // ============================
    // Test 2
    // ============================
    uint8_t expected_hello[32] = {
        0x2c,0xf2,0x4d,0xba,0x5f,0xb0,0xa3,0x0e,
        0x26,0xe8,0x3b,0x2a,0xc5,0xb9,0xe2,0x9e,
        0x1b,0x16,0x1e,0x5c,0x1f,0xa7,0x42,0x5e,
        0x73,0x04,0x33,0x62,0x93,0x8b,0x98,0x24
    };

    sha256((uint8_t*)"hello", 5, out);

    printf("Test 2: hello\n");
    printf("Computed : "); dump_hex(out);
    printf("Expected : "); dump_hex(expected_hello);
    if (memcmp(out, expected_hello, 32) == 0) {
        printf("Result   : PASS\n\n");
    } else {
        printf("Result   : FAIL\n\n");
        failures++;
    }

    // ============================
    // Test 3
    // ============================
    uint8_t expected_test[32] = {
        0x9f,0x86,0xd0,0x81,0x88,0x4c,0x7d,0x65,
        0x9a,0x2f,0xea,0xa0,0xc5,0x5a,0xd0,0x15,
        0xa3,0xbf,0x4f,0x1b,0x2b,0x0b,0x82,0x2c,
        0xd1,0x5d,0x6c,0x15,0xb0,0xf0,0x0a,0x08
    };

    sha256((uint8_t*)"test", 4, out);

    printf("Test 3: test\n");
    printf("Computed : "); dump_hex(out);
    printf("Expected : "); dump_hex(expected_test);
    if (memcmp(out, expected_test, 32) == 0) {
        printf("Result   : PASS\n\n");
    } else {
        printf("Result   : FAIL\n\n");
        failures++;
    }

    // ============================
    // Test 4
    // ============================
    uint8_t wrong_expected_abc[32] = {
        0xbb,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,
        0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,
        0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };

    sha256((uint8_t*)"abc", 3, out);

    printf("Test 4: abc (intentional failure)\n");
    printf("Computed : "); dump_hex(out);
    printf("Expected : "); dump_hex(wrong_expected_abc);
    if (memcmp(out, wrong_expected_abc, 32) == 0) {
        printf("Result   : PASS (unexpected)\n\n");
    } else {
        printf("Result   : FAIL (as intended)\n\n");
        failures++;  
    }

    if (failures == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Tests finished with %d failure(s)\n", failures);
        return 1;
    }
}
