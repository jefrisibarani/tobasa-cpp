#ifdef TOBASA_HTTP_USE_HTTP2

#include "tobasa/util_string.h"
#include "tobasahttp/server/http2_helper.h"

namespace tbs {
namespace http2 {

/* Conditional logic w/ lookup tables to check if id is one of the
   the block listed cipher suites for HTTP/2 described in RFC 7540.
   https://github.com/jay/http2_blacklisted_ciphers
*/
#define IS_CIPHER_BANNED_METHOD2(id)                                           \
  ((0x0000 <= id && id <= 0x00FF &&                                            \
    "\xFF\xFF\xFF\xCF\xFF\xFF\xFF\xFF\x7F\x00\x00\x00\x80\x3F\x00\x00"         \
    "\xF0\xFF\xFF\x3F\xF3\xF3\xFF\xFF\x3F\x00\x00\x00\x00\x00\x00\x80"         \
        [(id & 0xFF) / 8] &                                                    \
      (1 << (id % 8))) ||                                                      \
   (0xC000 <= id && id <= 0xC0FF &&                                            \
    "\xFE\xFF\xFF\xFF\xFF\x67\xFE\xFF\xFF\xFF\x33\xCF\xFC\xCF\xFF\xCF"         \
    "\x3C\xF3\xFC\x3F\x33\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"         \
        [(id & 0xFF) / 8] &                                                    \
      (1 << (id % 8))))

namespace {

bool selectProto(const unsigned char **out, unsigned char *outlen,
                  const unsigned char *in, unsigned int inlen,
                  const StringRef &key) 
{
   for (auto p = in, end = in + inlen; p + key.size() <= end; p += *p + 1) 
   {
      if (std::equal(std::begin(key), std::end(key), p)) 
      {
         *out = p + 1;
         *outlen = *p;
         return true;
      }
  }
  return false;
}
} // namespace

bool checkHttp2CipherBlockList(int id)
{
   return IS_CIPHER_BANNED_METHOD2(id);
}

bool checkH2IsSelected(const StringRef &proto) 
{
  return util::streq(NGHTTP2_H2, proto)    || 
         util::streq(NGHTTP2_H2_16, proto) ||
         util::streq(NGHTTP2_H2_14, proto);
}

bool selectH2(const unsigned char **out, unsigned char *outlen,
               const unsigned char *in, unsigned int inlen) 
{
   return selectProto(out, outlen, in, inlen, NGHTTP2_H2_ALPN)    ||
          selectProto(out, outlen, in, inlen, NGHTTP2_H2_16_ALPN) ||
          selectProto(out, outlen, in, inlen, NGHTTP2_H2_14_ALPN);
}

const unsigned char* getAlpnProtos(unsigned int &outLen)
{
   static const unsigned char sAlpnProtos[] = {
      2, 'h', '2',                        // "h2"
      8, 'h','t','t','p','/','1','.','1'  // "http/1.1"
   };
   outLen = sizeof(sAlpnProtos);
   return sAlpnProtos;
}

} // namespace http2
} // namespace tbs

#endif //TOBASA_HTTP_USE_HTTP2