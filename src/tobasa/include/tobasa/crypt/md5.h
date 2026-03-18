/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security,
 * Inc. MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Written by Solar Designer <solar at openwall.com> in 2001, and placed
 * in the public domain.  There's absolutely no warranty.
 *
 * See md5.c for more information.
 */

#pragma once

namespace tbs {
namespace crypt {
namespace md5 {

constexpr auto MD5_LENGTH     = 16;
constexpr auto MD5_PASSWD_LEN = 35;

typedef unsigned char md5_uint8;    /* 8 bits  */
typedef unsigned int  md5_uint32;   /* 32 bits */

struct md5_context
{
   md5_uint32 lo, hi;
   md5_uint32 a, b, c, d;
   unsigned char buffer[64];
   md5_uint32 block[MD5_LENGTH];
};

extern void md5_init(md5_context *ctx);
extern void md5_update(md5_context *ctx, void *data, unsigned long size);
extern void md5_final(md5_context *ctx,unsigned char *result);

void md5_get_digest(
        void*         data,
        unsigned long size,
        unsigned char result[MD5_LENGTH]);

} // namespace md5
} // namespace crypt
} // namespace tbs