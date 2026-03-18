#pragma once

#include <string>
#include <memory>
#include <tobasa/navigator.h>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/mysql_connection.h"
#include "tobasasql/mysql_variant_helper.h"

namespace tbs {
namespace sql {

class MysqlRowInfo;

/** 
 * \ingroup SQL
 * \brief MySQL SQL Result class.
 * \details
 * Backend implementation using MySQL C API and `RecordVariant`.  
 * - `runQuery()` executes SQL commands or `SELECT * FROM <table/view>`.  
 * - `setOptionOpenTable(true)` works to fetch entire table/view.  
 * - `setOptionCacheData()` has no effect (data is always fully cached).  
 *
 * Provides full navigation and value retrieval via SqlResult interface.
 */
class MysqlResult : public ResultCommon
{
public:

   using VariantType   = MysqlVariantType;
   using VectorVariant = std::vector<MysqlVariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   MysqlResult(MysqlConnection* pconn = nullptr);
   ~MysqlResult();

   // -------------------------------------------------------
   // Specific implementation methods
   // -------------------------------------------------------

   /// Get implementation class name
   std::string name() const;

   /**
    * \brief Execute query
    * \details
    * Executes the SQL query using LibMariaDB API and caches rows in DataSet<VariantType>.  
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
      const MysqlParameterCollection& parameters = {});

   /// Set specific driver sql connection class implementation.
   void connection(MysqlConnection* conn);

   /// Get specific driver sql connection class implementation.
   MysqlConnection* connection() const;

   NavigatorBasic& navigator();

   // -------------------------------------------------------
   // Override methods from base class : ResultCommon
   // -------------------------------------------------------

   /** 
    * Get Column Type Class.
    * Column index start at 0.
    * Throws TypeException, SqlException
   */
   TypeClass columnTypeClass(const int columnIndex) const;

   /** 
    * Get Variant value
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
   */
   VariantType getVariantValue(const int columnIndex) const;

   /// Get Variant value.
   VariantType getVariantValue(const std::string& columnName) const;

   /** 
    * Get string value.
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    */
   std::string getStringValue(const int columnIndex) const;

   /// Get string value.
   std::string getStringValue(const std::string& columnName) const;

   /**
    * Check for null field.
    * Column index start at 0.
    * Throws SqlException.
    */
   virtual bool isNullField(const int columnIndex) const;

private:

   /// Setup column informations.
   ///  Throws SqlException
   void setupColumnProperties(const std::vector<MYSQL_FIELD*>& fieldsInfo);

   /// Mysql extra column informations.
   struct ColumnMetadata
   {
      bool isBinary = false;
   };

   /// Column metadata collection.
   std::vector<ColumnMetadata> _metadataCollection;

   /// Cached data from backend.
   std::shared_ptr<sql::DataSet<VariantType>> _pDataset;
  
   /// Implemented Mysql connection object.
   MysqlConnection* _pConn;

   NavigatorBasic _navigator;
};

} // namespace sql
} // namespace tbs