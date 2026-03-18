#pragma once

#include <string>
#include <tobasa/navigator.h>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/sqlite_connection.h"

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
   using VariantType   = tbs::DefaultVariantType;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   SqliteResult(SqliteConnection* pconn = nullptr);
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
      const SqlParameterCollection& parameters = SqlParameterCollection());

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

private:

   /// Setup column informations.
   void setupColumnProperties();

   /// Get column declared type.
   std::string columnDeclaredType(sqlite3_stmt* stmt, int pos);

   /// Setup column metadata.
   void setupColumnMetaData();

   /// SQLite column informations.
   struct ColumnMetadata
   {
      // Note: https://www.sqlite.org/c3ref/column_database_name.html

      long         colIndex     = 0;;
      std::string  colName      = "";
      std::string  tableName    = "";
      std::string  collSeqName  = "";
      std::string  dataType     = "";
      //bool         isNotNull    = true;
      //bool         isPrimaryKey = false;
      //bool         isAutoInc    = false;
   };

   /// Column metadata collection.
   std::vector<ColumnMetadata> _metadataCollection;

   /// Cached data from backend.
   RecordVariant _dataVariant;

   /// Sqlite driver native statement pointer.
   sqlite3_stmt* _pStatement;

   /// Implemented Sqlite connection object.
   SqliteConnection* _pConn;

   NavigatorBasic _navigator;
};

} // namespace sql
} // namespace tbs