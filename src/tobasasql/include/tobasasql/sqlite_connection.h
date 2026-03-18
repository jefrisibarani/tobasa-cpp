#pragma once

#include "tobasasql/sql_connection_common.h"
#include "tobasasql/sql_parameter.h"
#include "tobasasql/sqlite_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */
template <typename VariantTypeImplemented>
struct DataSet;

/**
 * \brief SQLite SQL Connection class.
 * \details
 * Backend implementation using SQLite C API.  
 */
class SqliteConnection : public ConnectionCommon
{
public:
   SqliteConnection();
   ~SqliteConnection();

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
    * \param sql        SQL command or stored procedure text.
    * \param parameters Collection of parameters to bind to the query.
    */
   int execute(
      const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection());


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
      const SqlParameterCollection& parameters = SqlParameterCollection());

   std::string versionString() const;

   std::string databaseName();

   BackendType backendType() const;

   std::string dbmsName();

   int64_t lastInsertRowid();

   // -------------------------------------------------------
   // Specific implementation functions
   // -------------------------------------------------------

   /// Get pointer to native driver's connection object.
   sqlite3* nativeConn() const;

   // Open database with encryption key.
   bool keyDatabase(const std::string& key);

   // Sets new database encryption key.
   bool rekeyDatabase(const std::string& newKey);

   /// Get last backend error.
   std::string lastBackendError() const;

   sqlite3_stmt* createStatement(const std::string& sql, const SqlParameterCollection& parameters);


   /** 
    * \brief Execute query, and retrieve simple result set.
    * \param sql         Sql command or table name
    * \param parameters  SqlParameter collection
    * 
    * On error, SqlException thrown
    * On bad connection status, nullptr
    */
   std::shared_ptr<DataSet<DefaultVariantType>> executeResult(
      const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection());

   // Is table or view exists.
   bool tableOrViewExists(const std::string& tableName, bool checkTable);

   // Get tables or views.
   bool getTablesOrViews(std::vector<std::string>& objectNames, bool getTables);

private:
   /// Sqlite native connection object.
   sqlite3*    _pDatabase;
   std::string _databaseName;
   bool        _isEncrypted;

};

/** @}*/

} // namespace sql
} // namespace tbs