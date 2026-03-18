#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#import "c:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "EndOfFile")

#include "tobasasql/sql_connection_common.h"
#include "tobasasql/adodb_common.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL 
 * \brief ADO (MSSQL) SQL Connection class (Windows only).
 * \details
 * Backend implementation using `ADODB::_ConnectionPtr`.  
 * - Connects to SQL Server via OLE DB provider.
 * - Windows-only backend.
 * \note use DataTypeCompatibilyt=80 in connection string
 */ 
class AdodbConnection : public ConnectionCommon
{
public:
   AdodbConnection();
   ~AdodbConnection();

   std::string name() const;
   bool connect(const std::string& connString);
   bool disconnect();
   ConnectionStatus status();

   /** 
    * \brief Execute sql command or stored procedure that does not return rows.
    * \details On successfull execution, returns affected rows ( >= 0)  (INSERT/UPDATE/DELETE command)
    * On error, SqlException thrown
    * On bad connection, retuns -1
    * 
    * Result from sql command returning row(s) is ignored and affected rows is 0.
    * 
    * \param sql         Sql command query
    * \param parameters  AdoParameter collection.
    */
   int execute(
      const std::string& sql,
      const AdoParameterCollection& parameters = AdoParameterCollection());

   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details
    *  On successful execution of a data-modifying statement (INSERT, UPDATE, DELETE),
    *  returns the number of affected rows (≥ 0).  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns false.  
    *
    *  If the SQL command produces a result set (e.g. SELECT), the result is ignored
    *  and the affected row count is reported as 0.
    *
    * \param      sql           SQL command query
    * \param[out] affectedRows  Record(s) affected by executed sql command
    * \param[in]  parameters    Collection of AdoParameter.
    */
   bool execute(
      const std::string& sql,
      int& affectedRows,
      const AdoParameterCollection& parameters = AdoParameterCollection());

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
      const AdoParameterCollection& parameters = AdoParameterCollection());

   std::string versionString();

   std::string databaseName();

   BackendType backendType() const;

   std::string dbmsName();

   int64_t lastInsertRowid();

   // -------------------------------------------------------
   // Specific implementation functions
   // -------------------------------------------------------
   
   /// Get pointer to native driver's connection object.
   ADODB::_ConnectionPtr nativeConn() const;

   bool tableOrViewExists(const std::string& tableName, bool checkTable);

   bool isPrimaryKey(const std::string& columnName, const std::string& tableName);

   bool getTablesOrViews(std::vector<std::string>& objectNames, bool getTables);

   bool getPrimaryKeyColumns(std::vector<std::string>& primaryKeyCols, const std::string& tableName);

   bool getAutoIncrementColumns(std::vector<std::string>& autoIncrCols, const std::string& tableName);

private:

   /// Adodb native connection object.
   ADODB::_ConnectionPtr _pConn;
};


} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)