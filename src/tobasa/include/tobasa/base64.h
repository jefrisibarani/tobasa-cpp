#pragma once

#include <vector>
#include <string>

namespace tbs {
namespace base64 {

typedef unsigned char BYTE;

/**
 * \brief Encodes a byte buffer to a Base64 string.
 * \param buf Pointer to the byte buffer.
 * \param bufLen Length of the byte buffer.
 * \return A Base64-encoded string.
 */
std::string encodeFromBytes(BYTE const* buf, unsigned int bufLen);

/**
 * \brief Decodes a Base64-encoded string to a byte buffer.
 * \param str Base64-encoded string to decode.
 * \return A vector of bytes.
 */
std::vector<BYTE> decodeIntoBytes(std::string const&);


/** 
 * @brief Base64 encode
 * @param data       The data to encode
 * @return           Base64 encoded string
 */
std::string encode(const std::vector<BYTE>& data);

/** 
 * @brief Base64 encode
 * @param data       The data to encode
 * @return           Base64 encoded string
 */
std::string encode(const std::string& data);

/** Base64 decode
 * @param data       Base64 encoded data
 * @return           Decoded string
 */
std::string decode(const std::string& data);

} // namespace base64
} // namespace tbs