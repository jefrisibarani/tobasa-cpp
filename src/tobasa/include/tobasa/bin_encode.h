#pragma once

#include <string>
#include "tobasa/common.h"
#include "tobasa/exp_imp.h"

namespace tbs {
namespace conv {

/** @addtogroup CONV Data Conversion
 * @{
 */

/**
 * @brief Converts a single-digit hex character to its decimal equivalent.
 * @param hex Single-digit hex character ('0' to '9', 'A' to 'F', or 'a' to 'f').
 * @return int Decimal equivalent of the input hex character.
 */
TBS_API int hexToDec(const char hex);

/**
 * @brief Converts a two-digit hex number to its decimal equivalent.
 * @param buf Pointer to a string containing a two-digit hex number.
 * @return Decimal equivalent of the input.
 */
TBS_API int hexToDec(const char* buf);

/**
 * @brief Converts a two-digit hex number represented as a string to its decimal equivalent.
 * @param str String containing a two-digit hex number.
 * @return Decimal equivalent of the input.
 */
TBS_API int hexToDec(const std::string& str);

/**
 * @brief Converts a decimal integer to a 2-character hex string (not prefixed by 0x).
 * @param dec Decimal integer value to convert.
 * @param buf Pointer to a character array to store the resulting hex string (2 characters).
 */
TBS_API void decToHex(byte_t dec, char* buf);

/**
 * @brief Converts a decimal integer to two characters representing its hex value.
 * @param dec Decimal integer value to convert.
 * @param ch1 Pointer to store the first character of the resulting hex value.
 * @param ch2 Pointer to store the second character of the resulting hex value.
 */
TBS_API void decToHex(byte_t dec, char* ch1, char* ch2);

/**
 * @brief Converts a decimal integer to a 2-character hex string (not prefixed by 0x).
 * @param dec Decimal integer value to convert.
 * @return 2-character hex representation of the decimal value.
 */
TBS_API std::string decToHex(byte_t dec);

/**
 * @brief Decodes a hexadecimal encoded string into a byte array.
 * 
 * @param hexString The input string containing hexadecimal encoded data.
 * @param outBuffer Pointer to the output byte array to store the decoded data.
 */
TBS_API void hexDecode(const std::string& hexString, byte_t* outBuffer);

/**
 * @brief Encodes a byte array into a hexadecimal string.
 * 
 * @param buffer Pointer to the byte array to be encoded.
 * @param bufferLength The length of the byte array.
 * @return hexadecimal encoded string
 * 
 * @example
 * A byte array containing {0xAA, 0x01} returns "AA01".
 */
TBS_API std::string hexEncode(byte_t buffer[], size_t bufferLength);

/**
 * @brief Encodes a byte array into a hexadecimal string.
 *
 * @param buffer Pointer to the byte array to be encoded.
 * @param bufferLength The length of the byte array.
 * @param output Reference to the string that receives the hexadecimal encoding.
 */
TBS_API void hexEncode(byte_t buffer[], size_t bufferLength, std::string& output);

/**
 * @brief Converts a byte array into a printable bit-sequence string.
 *
 * Each byte is represented by eight bits, from most significant to least
 * significant bit.
 *
 * @param data Pointer to the binary data.
 * @param length Number of bytes in the data.
 * @return Bit-sequence string.
 *
 * @example
 * A byte array containing {0xAA, 0x01} returns "1010101000000001".
 */
TBS_API std::string binaryBytesToString(const byte_t* data, size_t length);

/** @}*/

} // namespace conv
} // namespace tbs