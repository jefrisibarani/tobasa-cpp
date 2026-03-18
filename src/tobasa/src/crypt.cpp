#include <vector>
#include <cassert>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <iterator>
#include "tobasa/hextodec.h"
#include "tobasa/crypt/md5.h"
#include "tobasa/crypt/rijndael.h"
#include "tobasa/crypt/hmac_sha2.h"
#include "tobasa/crypt.h"
#include "tobasa/crypt/sha1.h"
#include "tobasa/base64.h"
#include "tobasa/exception.h"

namespace tbs {
namespace crypt {

std::string passwordEncrypt(
               const std::string& clearPassword,
               const std::string& salt)
{
   using namespace aes;

   std::string outStr;

   // generate SHA2-256 (32 bytes) of salt
   // keyHash will also be used as Rijndael's initialization vector (IV)
   byte_t keyHash[32];
   hashSHA(ShaType::SHA256, salt, keyHash);

   // output buffer
   auto inputLen = static_cast<unsigned long>(clearPassword.length());
   std::vector<byte_t> outBuffer( inputLen + 16L);

   // Rijndael CBC mode must have initialization vector => keyHash
   Rijndael rijndael;
   int retCode = rijndael.init(
                     Rijndael::CBC,          //
                     Rijndael::Encrypt,      //
                     keyHash,                // Key
                     Rijndael::Key32Bytes,   // use 256 bits key
                     keyHash);               // IV will be padded to 16 bytes

   if (retCode != RIJNDAEL_SUCCESS)
      throw CryptException("Error initializing AES");

   int outLen = rijndael.padEncrypt(
                  (byte_t*)clearPassword.c_str(),  //
                  inputLen,                        //
                  outBuffer.data());               //

   if (outLen >= 0)
      outStr = hexEncode(outBuffer.data(), outLen);
   else
      throw CryptException("Error occured on AES encrypt");


   return outStr;
}


std::string passwordDecrypt(
               const std::string& encryptedPassword,
               const std::string& salt)
{
   using namespace aes;

   size_t pwdLenOri = encryptedPassword.size();
   if (pwdLenOri % 2)
      throw CryptException("Invalid encrypted password input");

   // convert hexadecimal encoded encryptedPassword to a byte array
   size_t pwdLen = pwdLenOri / 2;
   std::vector<byte_t> pasBuffer(pwdLen);
   hexDecode(encryptedPassword, pasBuffer.data());

   // generate SHA2-256 (32 bytes) of salt
   // keyHash will also be used as Rijndael's initialization vector (IV)
   byte_t keyHash[32];
   hashSHA(ShaType::SHA256, salt, keyHash);

   // prepare output buffer
   std::vector<byte_t> outBuffer(pwdLen);

   // Rijndael CBC mode must have initialization vector => keyHash
   Rijndael rijndael;
   int retCode = rijndael.init(
                     aes::Rijndael::CBC,
                     aes::Rijndael::Decrypt,
                     keyHash,                      // Key
                     aes::Rijndael::Key32Bytes,    // use 256 bits key
                     keyHash);                     // IV will be padded to 16 bytes

   if (retCode != RIJNDAEL_SUCCESS)
      throw CryptException("Error initializing AES");

   int outLen = rijndael.padDecrypt(pasBuffer.data(), static_cast<int>(pwdLen), outBuffer.data());

   if (outLen < 0)
      throw CryptException("Error occured on AES decrypt");

   std::string outStr((char*)outBuffer.data(), outLen);
   return outStr;
}


std::string hashMD5(const std::string& message)
{
   using namespace md5;

   byte_t hashBuffer[MD5_LENGTH];
   auto msgLength = static_cast<unsigned long>(message.length());
   md5_get_digest((byte_t*)message.c_str(), msgLength , hashBuffer);

   std::string outStr = hexEncode(hashBuffer, MD5_LENGTH);

   return outStr;
}


void hashSHA1(const std::string& message, byte_t* hashOut)
{
   if ( hashOut == nullptr)
      throw CryptException("Invalid SHA-1 output buffer");

   auto msgLength = static_cast<unsigned int>(message.length());
   sha1::sha1_hash((uint8_t*)message.data(), msgLength, hashOut);
}


std::string hashSHA1(const std::string& message)
{
   byte_t messageHash[SHA1_DIGEST_SIZE];
   hashSHA1(message, messageHash);
   return hexEncode(messageHash, SHA1_DIGEST_SIZE);
}


std::vector<byte_t> hashSHA1Bytes(const std::string& message)
{
   byte_t messageHash[SHA1_DIGEST_SIZE];
   hashSHA1(message, messageHash);
   return std::vector<byte_t>(std::begin(messageHash), std::end(messageHash));
}


void hashSHA(ShaType shaType, const std::string& message, byte_t* hashOut)
{
   using namespace sha;

   if ( hashOut == nullptr)
      throw CryptException("Invalid SHA-2 output buffer");

   auto msgLength = static_cast<unsigned int>(message.length());

   switch(shaType)
   {
   case ShaType::SHA224:
      sha224((uint8*)message.data(), msgLength, hashOut);
      break;
   case ShaType::SHA256:
      sha256((uint8*)message.data(), msgLength, hashOut);
      break;
   case ShaType::SHA384:
      sha384((uint8*)message.data(), msgLength, hashOut);
      break;
   case ShaType::SHA512:
      sha512((uint8*)message.data(), msgLength, hashOut);
      break;
   default:
      throw CryptException("Invalid SHA-2 type");
   }
}


std::string hashSHA(ShaType shaType, const std::string& message)
{
   using namespace sha;

   switch(shaType)
   {
   case ShaType::SHA224:
      {
         byte_t messageHash[SHA224_DIGEST_SIZE];
         hashSHA(shaType, message, messageHash);
         //sha224((uint8*)message.data(), message.length(), messageHash);
         return hexEncode(messageHash, SHA224_DIGEST_SIZE);
      }
   case ShaType::SHA256:
      {
         byte_t messageHash[SHA256_DIGEST_SIZE];
         hashSHA(shaType, message, messageHash);
         //sha256((uint8*)message.data(), message.length(), messageHash);
         return hexEncode(messageHash, SHA256_DIGEST_SIZE);
      }
   case ShaType::SHA384:
      {
         byte_t messageHash[SHA384_DIGEST_SIZE];
         hashSHA(shaType, message, messageHash);
         //sha384((uint8*)message.data(), message.length(), messageHash);
         return hexEncode(messageHash, SHA384_DIGEST_SIZE);
      }
   case ShaType::SHA512:
      {
         byte_t messageHash[SHA512_DIGEST_SIZE];
         hashSHA(shaType, message, messageHash);
         //sha512((uint8*)message.data(), message.length(), messageHash);
         return hexEncode(messageHash, SHA512_DIGEST_SIZE);
      }
   default:
      throw CryptException("Invalid SHA-2 type");
   }
}


std::vector<byte_t> hmacSHABytes(
               ShaType                    shaType,
               const std::string&         message,
               const std::vector<byte_t>& keyIn,
               int                        macSize)
{
   using namespace sha;

   int hmacSize   = 0;
   auto keySize   = static_cast<unsigned int>(keyIn.size());
   auto msgLength = static_cast<unsigned int>(message.length());

   switch(shaType)
   {
   case ShaType::SHA224:
      {
         byte_t hashMacOut[SHA224_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA224_DIGEST_SIZE) ? macSize : SHA224_DIGEST_SIZE;
         hmac_sha224(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return std::vector<byte_t>(std::begin(hashMacOut), std::end(hashMacOut));

      }
   case ShaType::SHA256:
      {
         byte_t hashMacOut[SHA256_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA256_DIGEST_SIZE) ? macSize : SHA256_DIGEST_SIZE;
         hmac_sha256(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return std::vector<byte_t>(std::begin(hashMacOut), std::end(hashMacOut));
      }
   case ShaType::SHA384:
      {
         byte_t hashMacOut[SHA384_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA384_DIGEST_SIZE) ? macSize : SHA384_DIGEST_SIZE;
         hmac_sha384(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return std::vector<byte_t>(std::begin(hashMacOut), std::end(hashMacOut));
      }
   case ShaType::SHA512:
      {
         byte_t hashMacOut[SHA512_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA512_DIGEST_SIZE) ? macSize : SHA512_DIGEST_SIZE;
         hmac_sha512(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return std::vector<byte_t>(std::begin(hashMacOut), std::end(hashMacOut));
      }
   default:
      throw CryptException("Invalid HMAC SHA-2 type");
   }
}


std::string hmacSHA(
               ShaType                    shaType,
               const std::string&         message,
               const std::vector<byte_t>& keyIn,
               int                        macSize)
{
   using namespace sha;

   int hmacSize   = 0;
   auto keySize   = static_cast<unsigned int>(keyIn.size());
   auto msgLength = static_cast<unsigned int>(message.length());

   switch(shaType)
   {
   case ShaType::SHA224:
      {
         byte_t hashMacOut[SHA224_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA224_DIGEST_SIZE) ? macSize : SHA224_DIGEST_SIZE;
         hmac_sha224(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return hexEncode(hashMacOut, hmacSize);
      }
   case ShaType::SHA256:
      {
         byte_t hashMacOut[SHA256_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA256_DIGEST_SIZE) ? macSize : SHA256_DIGEST_SIZE;
         hmac_sha256(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return hexEncode(hashMacOut, hmacSize);
      }
   case ShaType::SHA384:
      {
         byte_t hashMacOut[SHA384_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA384_DIGEST_SIZE) ? macSize : SHA384_DIGEST_SIZE;
         hmac_sha384(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return hexEncode(hashMacOut, hmacSize);
      }
   case ShaType::SHA512:
      {
         byte_t hashMacOut[SHA512_DIGEST_SIZE];
         hmacSize = (macSize > 0 && macSize <= SHA512_DIGEST_SIZE) ? macSize : SHA512_DIGEST_SIZE;
         hmac_sha512(keyIn.data(), keySize, (uint8*)message.data(), msgLength, hashMacOut, hmacSize);
         return hexEncode(hashMacOut, hmacSize);
      }
   default:
      throw CryptException("Invalid HMAC SHA-2 type");
   }
}


std::string hmacSHA(
               ShaType            shaType,
               const std::string& message,
               const std::string& keyIn,
               bool               hexKey)
{
   std::vector<byte_t> keyHashBuffer;
   size_t keyLength = 0;

   if (hexKey)
   {
      // convert hexadecimal encoded message to a byte array.
      size_t keyLenOri = keyIn.size();

      if (keyLenOri % 2)
         throw CryptException("Invalid HMAC SHA-2 key input");

      keyLength = keyIn.size() / 2;
      keyHashBuffer.resize(keyLength);
      hexDecode(keyIn, keyHashBuffer.data());
   }
   else
   {
      std::string keyReal;
      keyReal = keyIn.empty() ? "pemudaharapanbangsa" : keyIn;
      for (size_t i = 0; i < keyReal.length(); i++)
      {
         char ch = keyReal.at(i);
         keyHashBuffer.push_back((byte_t)ch);
      }

      //keyLength = keyHashBuffer.size();
   }

   return hmacSHA(shaType, message, keyHashBuffer);
}


std::vector<byte_t> hmacSHABytes(
               ShaType            shaType,
               const std::string& message,
               const std::string& keyIn,
               bool               hexKey)
{
   std::vector<byte_t> keyHashBuffer;
   size_t keyLength = 0;

   if (hexKey)
   {
      // convert hexadecimal encoded message to a byte array.
      size_t keyLenOri = keyIn.size();

      if (keyLenOri % 2)
         throw CryptException("Invalid HMAC SHA-2 key input");

      keyLength = keyIn.size() / 2;
      keyHashBuffer.resize(keyLength);
      hexDecode(keyIn, keyHashBuffer.data());
   }
   else
   {
      std::string keyReal;
      keyReal = keyIn.empty() ? "pemudaharapanbangsa" : keyIn;
      for (size_t i = 0; i < keyReal.length(); i++)
      {
         char ch = keyReal.at(i);
         keyHashBuffer.push_back((byte_t)ch);
      }
   }

   return hmacSHABytes(shaType, message, keyHashBuffer);
}


std::string getCryptoRandomHex(size_t byteLength)
{
   std::random_device rd;  // secure entropy source

   std::ostringstream oss;
   oss << std::hex << std::setfill('0');

   for (size_t i = 0; i < byteLength; ++i)
   {
      unsigned char byte = static_cast<unsigned char>(rd() & 0xFF);
      oss << std::setw(2) << static_cast<int>(byte);
   }

   return oss.str();
}


std::string getCryptoRandomBase64Url(size_t byteLength)
{
   static const char b64_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

   std::random_device rd;  // secure entropy source

   // collect raw random bytes
   std::string bytes;
   bytes.resize(byteLength);
   for (size_t i = 0; i < byteLength; ++i)
   {
      bytes[i] = static_cast<char>(rd() & 0xFF);
   }

   // encode to base64
   std::string out;
   int val = 0, valb = -6;
   for (unsigned char c : bytes)
   {
      val = (val << 8) + c;
      valb += 8;
      while (valb >= 0)
      {
         out.push_back(b64_table[(val >> valb) & 0x3F]);
         valb -= 6;
      }
   }
   if (valb > -6)
   {
      out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
   }
   while (out.size() % 4)
   {
      out.push_back('=');
   }

   // URL-safe transform
   for (char &ch : out)
   {
      if (ch == '+') ch = '-';
      else if (ch == '/') ch = '_';
   }
   // remove '=' padding
   while (!out.empty() && out.back() == '=')
   {
      out.pop_back();
   }

   return out;
}


} // namespace crypt
} // namespace tbs