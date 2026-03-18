/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

#pragma once

#include <cstdint>

namespace tbs {
namespace crypt {
namespace sha1 {

/** SHA-1 Context */
typedef struct {
    uint32_t h[5];
    /**< Context state */
    uint32_t count[2];
    /**< Counter       */
    uint8_t buffer[64]; /**< SHA-1 buffer  */
} sha1_ctx;

#define SHA1_BLOCK_SIZE 64
/** SHA-1 Digest size in bytes */
#define SHA1_DIGEST_SIZE 20

void sha1_init(sha1_ctx *context);

void sha1_update(sha1_ctx *context, const uint8_t* message, size_t len);

void sha1_final(sha1_ctx *context, uint8_t digest[SHA1_DIGEST_SIZE]);

void sha1_transform(sha1_ctx *context, const uint8_t buffer[64]);

void sha1_hash(const uint8_t* message, unsigned int len, uint8_t* digest);

} // namespace sha
} // namespace crypt
} // namespace tbs
