#pragma once

#include <string>
#include <vector>

namespace tbs {

/// Encryption functions.
namespace crypt {

/** @defgroup CRYPT Hashing and encryption
 * @{
 */

using byte_t = unsigned char;

/// @brief SHA Types
enum class ShaType
{
   SHA224,
   SHA256,
   SHA384,
   SHA512
};


/** 
 * @brief Encrypt password.
 * This function uses SHA256 to hash the salt, then encrypt with Rijndael
 * @param clearPassword  Password in clear text
 * @param salt           The salt
 * @return               32 characters hex encoded encrypted password (128 bit)
 * Throw std::exception on error
 */
std::string passwordEncrypt(
               const std::string& clearPassword,
               const std::string& salt = "");


/** 
 * @brief Decrypt hex encoded encrypted password.
 * This function uses SHA256 to hash the salt, then decrypt with Rijndael
 * @param encryptedPassword  32 characters hex encoded encrypted password (128 bit)
 * @param salt               The salt
 * @return                   Decrypted password
 * Throw std::exception on error
 */
std::string passwordDecrypt(
               const std::string& encryptedPassword,
               const std::string& salt = "");


/** 
 * @brief Calculates the hash of data using the MD5 algorithm.
 * @param message    The data to hash
 * @return           32 characters hex encoded MD5 hash (16 bytes/128 bit)
 */
std::string hashMD5(const std::string& message);


/** 
 * @brief Calculates the hash of data using the SHA1 algorithm.
 * @param message    The data to hash
 * @return           40 characters hex encoded SHA1 hash (20 bytes/160 bit)
 */
std::string hashSHA1(const std::string& message);


/** 
 * @brief Calculates the hash of data using the SHA-1 algorithm.
 * @param message    The data to hash
 * @param hashOut    Buffer to receive the hash (20 bytes long)
 * Throw std::exception on error
 */
void hashSHA1(const std::string& message, byte_t* hashOut);


/** 
 * @brief Calculates the hash of data using the SHA1 algorithm.
 * @param message    The data to hash
 * @return           40 characters hex encoded SHA1 hash (20 bytes/160 bit)
 */
std::string hashSHA1(const std::string& message);

/** 
 * @brief Calculates the hash of data using the SHA1 algorithm.
 * @param message    The data to hash
 * @return           std::vector containing the hash
 */
std::vector<byte_t> hashSHA1Bytes(const std::string& message);


/** 
 * @brief Calculates the hash of data using the SHA-2 algorithm.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to hash
 * @param hashOut    Buffer to receive the hash
 *                   SHA-224 : must be at least 28 bytes long
 *                   SHA-256 : must be at least 32 bytes long
 *                   SHA-384 : must be at least 48 bytes long
 *                   SHA-512 : must be at least 64 bytes long
 * Throw std::exception on error
 */
void hashSHA(
         ShaType            shaType,
         const std::string& message,
         byte_t*            hashOut);


/** 
 * @brief Calculates the hash of data using the SHA-2 algorithm.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to hash
 * @return           Hex encoded characters hash,
 *                   SHA-224 : 56  characters (28 bytes/224 bit)
 *                   SHA-256 : 64  characters (32 bytes/256 bit)
 *                   SHA-384 : 96  characters (48 bytes/384 bit)
 *                   SHA-512 : 128 characters (64 bytes/512 bit)
 * Throw std::exception on error
 */
std::string hashSHA(ShaType shaType, const std::string& message);


/** 
 * @brief Calculates the HMAC SHA-2 of data.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to HMAC
 * @param keyIn      The HMAC key
 * @param macSize    output hash in bytes, 0 uses default
 * @return           Hex encoded characters HMAC SHA-2 hash,
 *                   SHA-224 : 56  characters (28 bytes/224 bit)
 *                   SHA-256 : 64  characters (32 bytes/256 bit)
 *                   SHA-384 : 96  characters (48 bytes/384 bit)
 *                   SHA-512 : 128 characters (64 bytes/512 bit)
 * Throw std::exception on error
 */
std::string hmacSHA(
               ShaType                    shaType,
               const std::string&         message,
               const std::vector<byte_t>& keyIn,
               int                        macSize = 0);


/** 
 * @brief Calculates the HMAC SHA-2 of data.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to HMAC
 * @param keyIn      The HMAC key
 * @param macSize    output hash in bytes, 0 uses default
 * @return           HMAC SHA-2 result structure
 *                   SHA-224 : 56  characters (28 bytes/224 bit)
 *                   SHA-256 : 64  characters (32 bytes/256 bit)
 *                   SHA-384 : 96  characters (48 bytes/384 bit)
 *                   SHA-512 : 128 characters (64 bytes/512 bit)
 * Throw std::exception on error
 */
std::vector<byte_t> hmacSHABytes(
               ShaType                    shaType,
               const std::string&         message,
               const std::vector<byte_t>& keyIn,
               int                        macSize = 0);


/** 
 * @brief Calculates the HMAC SHA-2 of data.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to HMAC
 * @param keyIn      The key, string or hex encoded byes
 * @param hexKey     Boolean, if true, key must be hex encoded byte
 * @return           Hex encoded characters HMAC SHA-2 hash,
 *                   SHA-224 : 56  characters (28 bytes/224 bit)
 *                   SHA-256 : 64  characters (32 bytes/256 bit)
 *                   SHA-384 : 96  characters (48 bytes/384 bit)
 *                   SHA-512 : 128 characters (64 bytes/512 bit)
 * Throw std::exception on error
 */
std::string hmacSHA(
               ShaType            shaType,
               const std::string& message,
               const std::string& keyIn,
               bool               hexKey = false);

/** 
 * @brief Calculates the HMAC SHA-2 of data.
 * @param shaType    ShaType: SHA224, SHA256, SHA384, SHA512
 * @param message    The data to HMAC
 * @param keyIn      The key, string or hex encoded byes
 * @param hexKey     Boolean, if true, key must be hex encoded byte
 * @return           HMAC SHA-2 hash bytes
 *                   SHA-224 : 56  characters (28 bytes/224 bit)
 *                   SHA-256 : 64  characters (32 bytes/256 bit)
 *                   SHA-384 : 96  characters (48 bytes/384 bit)
 *                   SHA-512 : 128 characters (64 bytes/512 bit)
 * Throw std::exception on error
 */
std::vector<byte_t> hmacSHABytes(
               ShaType            shaType,
               const std::string& message,
               const std::string& keyIn,
               bool               hexKey = false);

/**
 * Generates a cryptographically strong random string encoded in hexadecimal.
 * Each random byte is converted into 2 hex characters (0–9a–f).
 *
 * @param byteLength number of random bytes to generate
 * @return a hex string of length (byteLength × 2)
 */
std::string getCryptoRandomHex(size_t byteLength=16);

/**
 * Generates a cryptographically strong random string encoded in Base64-URL.
 * Base64-URL is a URL- and filename-safe variant:
 * '+' → '-', '/' → '_', and '=' padding is removed.
 *
 * @param byteLength number of random bytes to generate
 * @return a Base64-URL string, typically shorter than standard Base64
 *
 *   8 -> 11–12 , 16 -> 22 , 32 -> 43, 64 -> 86
 */
std::string getCryptoRandomBase64Url(size_t byteLength=16);

/** @}*/

} // namespace crypt
} // namespace tbs
