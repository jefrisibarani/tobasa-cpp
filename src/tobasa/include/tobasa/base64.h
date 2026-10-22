#pragma once

#include <vector>
#include <string>
#include "tobasa/common.h"
#include "tobasa/exp_imp.h"

namespace tbs {
namespace base64 {

/** @addtogroup TBS
 * @{
 */


/**
 * \brief Encodes a byte buffer to a Base64 string.
 * \param buf Pointer to the byte buffer.
 * \param bufLen Length of the byte buffer.
 * \return A Base64-encoded string.
 */
TBS_API std::string encodeFromBytes(byte_t const* buf, unsigned int bufLen);

/**
 * \brief Decodes a Base64-encoded string to a byte buffer.
 * \param str Base64-encoded string to decode.
 * \return A vector of bytes.
 */
TBS_API std::vector<byte_t> decodeIntoBytes(std::string const&);


/** 
 * @brief Base64 encode
 * @param data       The data to encode
 * @return           Base64 encoded string
 */
TBS_API std::string encode(const std::vector<byte_t>& data);

/** 
 * @brief Base64 encode
 * @param data       The data to encode
 * @return           Base64 encoded string
 */
TBS_API std::string encode(const std::string& data);

/** Base64 decode
 * @param data       Base64 encoded data
 * @return           Decoded string
 */
TBS_API std::string decode(const std::string& data);

/** @}*/

} // namespace base64
} // namespace tbs