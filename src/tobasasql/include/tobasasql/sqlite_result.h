#pragma once

#include <string>
#include <tobasa/navigator.h>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sqlite_connection.h"
#include "tobasasql/sqlite_command.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief SQLite SQL Result class.
 * \details
 * Backend implementation using SQLite C API.  
 * - `runQuery()` executes SQL commands or `SELECT * FROM <table/view>`.
 * - Results are stored in `RecordVariant` (vector of vectors).  
 * - `setOptionOpenTable()` works to fetch entire table/view.  
 * - `setOptionCacheData()` has no effect (data is always fully cached).
 *
 * Provides full navigation and value retrieval via SqlResult interface.
 */
class SqliteResult : public ResultCommon
{
public:
   using VariantType   = DefaultVariantType;
   using VariantHelper = VariantHelper<VariantType>;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

   SqliteResult(SqliteConnection* pconn = nullptr);

   SqliteResult(const SqliteResult&) = delete;
   SqliteResult& operator=(const SqliteResult&) = delete;

   SqliteResult(SqliteResult&& other) noexcept;
   SqliteResult& operator=(SqliteResult&& other) noexcept;

   ~SqliteResult();

   // -------------------------------------------------------
   // Specific implementation methods
   // -------------------------------------------------------

   /// Get implementation class name.
   std::string name() const;

   /**
    * \brief Execute query
    * \details
    * Executes the SQL query using SQLite API and caches rows in RecordVariant.  
    *
    * - setOptionOpenTable(true) → executes "SELECT * FROM <table/view>" internally.  
    * - setOptionCacheData() has no effect for SQLite (all data is materialized).
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
   
   bool runPreparedQuery(SqliteCommand& command);

   /// Set specific driver sql connection class implementation.
   void connection(SqliteConnection* conn);

   /// Get specific driver sql connection class implementation.
   SqliteConnection* connection() const;

   NavigatorBasic& navigator();

   // -------------------------------------------------------
   // Overridden methods from base class : ResultCommon
   // -------------------------------------------------------

   /** 
    * \brief Get Column Type Class.
    * Column index start at 0.
    * Throws TypeException, SqlException
    */
   virtual TypeClass columnTypeClass(const int columnIndex) const;

   /** 
    * \brief Get Variant value
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    */
   VariantType getVariantValue(const int columnIndex) const;

   
   /// Get Variant value.
   VariantType getVariantValue(const std::string& columnName) const;

   /** 
    * \brief Get string value
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    * */ 
   std::string getStringValue(const int columnIndex) const;

   /// Get string value.
   std::string getStringValue(const std::string& columnName) const;

   /** 
    * \brief Check for null field
    * Column index start at 0.
    * Throws SqlException.
    */
   virtual bool isNullField(const int columnIndex) const;

   bool getData(DataSetPtr dataSet, sqlite3_stmt* stmt);

private:

   /// Setup column informations.
   void setupColumnProperties(sqlite3_stmt* stmt);


   /// Setup column metadata.
   void setupColumnMetaData(sqlite3_stmt* stmt);

   /// SQLite column informations.
   struct ColumnMetadata
   {
      // Note: https://www.sqlite.org/c3ref/column_database_name.html

      long         colIndex     = 0;;
      std::string  colName      = "";
      std::string  tableName    = "";
      std::string  collSeqName  = "";
      std::string  dataType     = "";
   };

   /// Implemented Sqlite connection object.
   SqliteConnection* _pConn;

   /// Column metadata collection.
   std::vector<ColumnMetadata> _metadataCollection;

   /// Cached data from backend.
   DataSetPtr _pDataset;

   NavigatorBasic _navigator;
};

} // namespace sql
} // namespace tbs
