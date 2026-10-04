#pragma once

#include <string>
#include <memory>
#include <tobasa/navigator.h>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/sql_dataset.h"
#include "tobasasql/odbc_connection.h"
#include "tobasasql/odbc_command.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ODBC (MSSQL) SQL Result class.
 * \details
 * Backend implementation using ODBC API and `RecordVariant`.  
 * - `runQuery()` executes SQL commands or opens tables/views.  
 * - `setOptionOpenTable(true)` → fetch entire table/view.  
 * - `setOptionCacheData()` has no effect (data is always fully cached).
 * - `_affectedRows` tracks modified rows for action queries.
 *
 * Provides full navigation and value retrieval via SqlResult interface.
 */
class OdbcResult : public ResultCommon
{
public:

   using VariantType   = DefaultVariantType;
   using VariantHelper = VariantHelper<VariantType>;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

   OdbcResult(OdbcConnection* pconn = nullptr);

   OdbcResult(const OdbcResult&) = delete;
   OdbcResult& operator=(const OdbcResult&) = delete;

   OdbcResult(OdbcResult&& other) noexcept;
   OdbcResult& operator=(OdbcResult&& other) noexcept;

   ~OdbcResult();

   // -------------------------------------------------------
   // Specific implementation methods
   // -------------------------------------------------------

   /// Get implementation class name.
   std::string name() const;

   /**
    * \brief Execute query (MSSQL via ODBC).
    * \details
    * Executes the SQL query using ODBC API and caches rows in RecordVariant.  
    *
    * - setOptionOpenTable(true) → executes "SELECT * FROM <table/view>" internally.  
    * - setOptionCacheData() has no effect (all data is materialized).
    *
    * On success, returns true; on bad connection, returns false.  
    * On error, throws \c SqlException.
    *
    * For SQL commands that do not return rows (e.g. INSERT, UPDATE, DELETE),  
    * \c resultStatus() is \c ResultStatus::commandOk.  
    * For queries that return rows, \c resultStatus() is \c ResultStatus::tuplesOk.  
    *
    * \param sql         SQL command or table name (depending on option mode).  
    * \param parameters  SqlParameter collection. 
    */
   virtual bool runQuery(
      const std::string& sql,
      const SqlParameterCollection& parameters = {},
      ParameterStyle paramStyle = ParameterStyle::named );

   bool runPreparedQuery(OdbcCommand& command);

   /// Set specific driver sql connection class implementation.
   virtual void connection(OdbcConnection* conn);

   /// Get specific driver sql connection class implementation.
   OdbcConnection* connection() const;

   /// Get navigator reference.
   NavigatorBasic& navigator();

   // -------------------------------------------------------
   // Override methods from base class : ResultCommon
   // -------------------------------------------------------

   TypeClass columnTypeClass(const int columnIndex) const;

   VariantType getVariantValue(const int columnIndex) const;

   VariantType getVariantValue(const std::string& columnName) const;

   std::string getStringValue(const int columnIndex) const;

   std::string getStringValue(const std::string& columnName) const;

   /// Check if the field is a null field.
   // TODO_JEFRI : Fix this
   virtual bool isNullField(const int columnIndex) const;

   bool getData(DataSetPtr dataSet, SQLHSTMT stmt);

private:

   /// Setup column informations.
   void setupColumnProperties(SQLHSTMT stmt);

   /// Cached data from backend.
   DataSetPtr _pDataset;

   /// Implemented Odbc connection object.
   OdbcConnection* _pConn;

   NavigatorBasic _navigator;
};

} // namespace sql
} // namespace tbs