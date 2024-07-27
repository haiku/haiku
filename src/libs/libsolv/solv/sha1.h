/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

#include <stdint.h>

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} SHA1_CTX;

#define SHA1_DIGEST_SIZE 20

void solv_SHA1_Init(SHA1_CTX* context);
void solv_SHA1_Update(SHA1_CTX* context, const uint8_t* data, const size_t len);
void solv_SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE]);
