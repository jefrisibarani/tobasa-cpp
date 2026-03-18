#pragma once

#include <cassert>
#include <string>
#include <memory>

#include <tobasa/navigator.h>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/pgsql_connection.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief PostgreSQL SQL Result class.
 * \details
 * Backend implementation using `PGresult` (libpq).  
 * - `runQuery()` executes SQL commands or `SELECT * FROM <table/view>`.
 * - All rows are fully materialized in memory.
 * - `setOptionOpenTable()` works to fetch entire table/view.
 * - `setOptionCacheData()` has no effect (data is always fully cached).
 *
 * Provides full navigation and value retrieval via SqlResult interface.
 */
class PgsqlResult : public ResultCommon
{
public:
   using VariantType   = tbs::DefaultVariantType;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   /// Constructor.
   PgsqlResult(PgsqlConnection* pconn = nullptr);

   /// Destructor.
   ~PgsqlResult();

   // -------------------------------------------------------
   // Specific implementation methods
   // -------------------------------------------------------

   /// Get implementation class name.
   virtual std::string name() const;

   /** 
    * \brief Execute query (PostgreSQL).
    * \details
    * Executes the SQL query using libpq and stores results in PGresult.  
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
      const SqlParameterCollection& parameters = SqlParameterCollection());

   /// Set specific driver sql connection class implementation.
   void connection(PgsqlConnection* conn);

   /// Get specific driver sql connection class implementation.
   PgsqlConnection* connection() const;

   NavigatorBasic& navigator();

   Oid tableOid();

   // -------------------------------------------------------
   // Override methods from base class : ResultCommon
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
    */
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

   void throwIfPgResultInvalid() const;

   /// Setup column informations.
   ///  Throws SqlException
   void setupColumnProperties();

   /// PostgreSql native result object.
   PGresult* _pPGresult;

   /// Implemented Sql connection object.
   PgsqlConnection* _pConn;

   NavigatorBasic _navigator;
};

} // namespace sql
} // namespace tbs