#pragma once

#include <string>
#include "tobasasql/common_types.h"
#include "tobasasql/settings.h" // for Database

namespace tbs {
namespace util {

/** \addtogroup SQL
 * @{
 */

/// Check if value need needs quoting.
bool needsQuoting(const std::string& value);

/// Quote identifier.
std::string quoteIdent(const std::string& value);

/// Get column type class in string.
std::string columnTypeClassToString(sql::TypeClass typeClass);

/**
 * Builds a database connection string from the supplied database options.
 * If securitySalt is empty, the password in dbOption is treated as plaintext.
 *
 * @param dbOption Database connection options.
 * @param securitySalt Salt used when decrypting an encrypted password.
 * @return The generated database connection string.
 */
std::string getConnectionString(const sql::conf::Database& dbOption, const std::string& securitySalt);

/// Convert raw binary data to a printable bit-sequence string.
/// Example: a byte 0xAA becomes "10101010" and 0x01 becomes "00000001".
std::string binaryBytesToString(const void* data, int length);

/// Convert raw binary data to its hexadecimal representation.
/// Example: bytes {0xAA, 0x01} become "AA01".
std::string binaryBytesToHexString(const void* data, int length);


/** @}*/

} // namespace util
} // namespace tbs