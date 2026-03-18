#pragma once
#include <string>
#include <vector>
#include <cctype>

#include "tobasasql/common_types.h"

namespace tbs {
namespace sql {

/**
 * \ingroup SQL
 * \brief Rewrites SQL statements with named parameters into DB-native parameter syntax.
 * \details
 * This utility scans SQL text for named parameters in the form `:name` and 
 * rewrites them into the parameter style required by the target database 
 * (e.g., `?` for MySQL/SQLite, `@p1` for SQL Server, `$1` for PostgreSQL).
 *
 * The class preserves database-specific syntax (such as PostgreSQL type casts `::`) 
 * and string literals, ensuring only valid parameter markers are rewritten.
 *
 * Typical usage:
 * - Construct with a SQL query containing named parameters.
 * - Call \c rewrite() to obtain the DB-specific SQL text.
 * - Use \c parameters() to retrieve the ordered list of parameter names.
 *
 * Example:
 * \code
 *   SqlParameterRewriter rewriter(BackendType::PostgreSQL);
 *   std::string sql = "SELECT * FROM users WHERE id = :id AND enabled = :enabled";
 *
 *   std::string newsql = rewriter.rewrite();
 *   // newsql -> "SELECT * FROM users WHERE id = $1 AND enabled = $2"
 *
 *   auto params = rewriter.parameters();  
 *   // params -> ["id", "enabled"]
 * \endcode
 *
 * This allows queries to be written in a DB-agnostic way with `:name` placeholders 
 * while supporting multiple database backends transparently.
 */
class SqlParameterRewriter 
{
public:

   SqlParameterRewriter(BackendType dbms)
      : _dbms(dbms) {}

   /**
    * Parse a SQL string containing named parameters (e.g. :id, :name).
    * Replaces them with DBMS-specific placeholders.
    *
    * @param sql input SQL with :named parameters
    * @return SQL with DBMS-specific placeholders
    */
   std::string rewrite(const std::string& sql);

   /**
    * Get the list of extracted parameter names in order.
    */
   const std::vector<std::string>& parameters() const;

private:

   BackendType _dbms;

   std::vector<std::string> _params;

   enum State { Normal, SingleQuote, DoubleQuote, LineComment, BlockComment, DollarQuote };

   std::string makePlaceholder(int index) const;
};



} // namespace sql
} // namespace tbs