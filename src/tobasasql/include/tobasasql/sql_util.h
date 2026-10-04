#pragma once

#include <cctype>
#include <string>
#include "tobasasql/common_types.h"
#include "tobasasql/settings.h" // for Database
#include "tobasasql/sql_parameter_rewriter.h"

namespace tbs {
namespace sql {


/** \addtogroup SQL
 * @{
 */

/// Check if value need needs quoting.
bool needsQuoting(const std::string& value);

/// Quote identifier.
std::string quoteIdent(const std::string& value);

/// Convert BackendType enum to std::string.
std::string backendTypeToString(BackendType type);

/// Convert std::string to BackendType.
BackendType backendTypeFromString(const std::string& type);

/// Convert DataType enum to std::string.
std::string dataTypeToString(DataType type);

/// Get column type class in string.
std::string columnTypeClassToString(TypeClass typeClass);

/// Get sql DataType from declared type
DataType dataTypeFromDeclaredType(const std::string& colType);

TypeClass typeClassFromDataType(DataType type);

/**
 * Builds a database connection string from the supplied database options.
 * If securitySalt is empty, the password in dbOption is treated as plaintext.
 *
 * @param dbOption Database connection options.
 * @param securitySalt Salt used when decrypting an encrypted password.
 * @return The generated database connection string.
 */
std::string getConnectionString(const conf::Database& dbOption, const std::string& securitySalt);

/**
 * Rewrites colon-style named parameters like :id into the database's native
 * placeholder style when the SQL is written in a backend-neutral form.
 *
 * Example:
 *   "SELECT * FROM users WHERE id = :id"
 * becomes:
 *   "SELECT * FROM users WHERE id = $1" for PostgreSQL, or "?" for SQLite/MySQL.
 *
 * If the SQL already uses native placeholders (for example PostgreSQL $1, $2),
 * it is left alone. This keeps fully native SQL working without unnecessary
 * rewriting.
 */
template<typename SqlParameterCollection>
std::string expandNamedParams(const std::string& sql,
   ParameterStyle style, const SqlParameterCollection& parameters, BackendType backendType)
{
   if (style != ParameterStyle::named || parameters.empty())
      return sql;

   SqlParameterRewriter writer(backendType);
   return writer.rewrite(sql);
}

/** @}*/

} // namespace sql
} // namespace tbs