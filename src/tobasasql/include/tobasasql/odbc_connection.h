#pragma once

#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include "tobasasql/sql_connection_common.h"
#include "tobasasql/odbc_parameter.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ODBC (MSSQL) SQL Connection class.
 * \details
 * Backend implementation using ODBC API, **specific to Microsoft SQL Server**.  
 * - Connects to MSSQL server via DSN or connection string.  
 * - On Windows, you can use either ODBC or ADO; on Linux/Unix, only ODBC is available.  
 * - **Not intended for other database servers.**
 */
class OdbcConnection : public ConnectionCommon
{
public:
   using VariantType = tbs::DefaultVariantType;

   OdbcConnection();
   ~OdbcConnection();

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
    * \param sql        SQL command query.
    * \param parameters Collection of parameters to bind to the query.
   */
   int execute(const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection());

   /** 
    * \brief Executes a parameterized SQL query and returns the first column of the first row.
    * \details
    *  If the query executes successfully, returns the value as a string (may be empty).
    *  If execution fails or the connection is invalid, throws SqlException.
    * \param sql        SQL command query with placeholders.
    * \param parameters Collection of parameters to bind to the query.
   */
   std::string executeScalar(const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection());

   std::string versionString();

   std::string databaseName();

   BackendType backendType() const;

   std::string dbmsName();

   int64_t lastInsertRowid();

   // -------------------------------------------------------
   // Specific implementation functions
   // -------------------------------------------------------

   /// Allocate ODBC statement.
   SQLHSTMT allocateStatement();


   /// Get field data.
   VariantType getFieldData(SQLHSTMT pStmt, int col);


   /// Get columns.
   bool getColumns(std::vector<std::string>& columnNames, const std::string& table);


   // Is table or view exists.
   bool tableOrViewExists(const std::string& tableName, bool checkTable);


   bool isPrimaryKey(const std::string& columnName, const std::string& tableName);


   // Get tables or views.
   bool getTablesOrViews(std::vector<std::string>& objectNames, bool getTables);


   bool getPrimaryKeyColumns(std::vector<std::string>& primaryKeyCols, const std::string& tableName);

   /**
   * \brief Supply supply data-at-execution.
   * Note: https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/sending-long-data?view=sql-server-ver15
   */
   SQLRETURN sqlPutData(SQLHSTMT pStmt, const SqlParameterCollection& parameters);

private:

   bool environmentAndConnectionHandleAlive();

   void allocateEnvironmentAndConnectionHandle();

   std::string getInfo(short infoType);

   std::string _dbmsName;
   std::string _dbmsVersion;

   // Environment handle.
   SQLHENV _pEnv;

   // ConnectionHandle.
   SQLHDBC _pDbc;
};


} // namespace sql
} // namespace tbs