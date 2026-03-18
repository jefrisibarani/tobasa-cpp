#pragma once

#include "tobasasql/sql_connection_common.h"
#include "tobasasql/mysql_common.h"
#include "tobasasql/sql_dataset.h"
#include <mysql/mysql.h>

namespace tbs {
namespace sql {

class MysqlCommand;

/**
 * \ingroup SQL
 * \brief MySQL SQL Connection class.
 * \details
 * Backend implementation using LibMariaDB C API.  
 * - Connects to MySQL server using host, port, username, password, and database.
 */
class MysqlConnection : public ConnectionCommon
{
public:
   MysqlConnection();
   ~MysqlConnection();

   std::string name();
   bool connect(const std::string& connString);
   bool disconnect();
   ConnectionStatus status();

   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details
    *  On successful execution of a data-modifying statement (INSERT, UPDATE, DELETE),
    *  returns the number of affected rows (≥ 0).  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns -1.  
    *
    *  If the SQL command produces a result set (e.g. SELECT), the result is ignored
    *  and the affected row count is reported as 0.
    *
    * \param sql        SQL command query
    * \param parameters Collection of parameters to bind to the query.
    */
   int execute(
      const std::string& sql,
      const MysqlParameterCollection& parameters = MysqlParameterCollection());

   /** 
    * \brief Executes a parameterized SQL query and returns the first column of the first row.
    * \details
    *  If the query executes successfully, returns the value as a string (may be empty).
    *  If execution fails or the connection is invalid, throws SqlException.
    * \param sql        SQL command query with placeholders.
    * \param parameters Collection of parameters to bind to the query.
    */
   std::string executeScalar(
      const std::string& sql,
      const MysqlParameterCollection& parameters = MysqlParameterCollection());

   std::string versionString();

   std::string databaseName();

   BackendType backendType() const;

   std::string dbmsName();

   /// EXPERIMENTAL. DO NOT USE THIS!
   int64_t lastInsertRowid();

   // -------------------------------------------------------
   // Specific implementation functions
   // -------------------------------------------------------

   /// Get last backend error.
   std::string lastBackendError();

   MYSQL* nativeConnection();

private:

   void throwExceptionOnError();

   /// MySQL native connection object.
   MYSQL* _pMYcon;
};

} // namespace sql
} // namespace tbs