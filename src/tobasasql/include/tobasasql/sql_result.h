#pragma once

#include <string>
#include <tobasa/variant.h>
#include <tobasa/format.h>
#include <tobasa/self_counter.h>
#include <tobasa/datetime.h>
#include <tobasa/util_string.h>
#include <tobasa/common.h>
#include "tobasasql/common_types.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/// SqlConnection forward declaration
template < typename SqlDriverType >
class SqlConnection;

/**
 * \brief Generic SQL result wrapper.
 * \details 
 * This template class provides a driver-agnostic wrapper around the underlying 
 * SQL result implementation. It manages the lifecycle of query results and 
 * exposes navigation, data access, and metadata retrieval methods.
 *
 * Typical usage:
 * - Execute an SQL command or open a table using `runQuery()`.
 * - Check validity with `isValid()` and inspect row count via `totalRows()`.
 * - Navigate rows using `move*()` functions (e.g., `moveNext()`).
 * - Retrieve field values with `get*Value()` methods.
 * - Access schema/column details using `column*()` methods.
 *
 * \tparam SqlDriverType The driver-specific implementation type.
 */
template < typename SqlDriverType >
class SqlResult
{
public:
   using ResultImpl                = typename SqlDriverType::ResultImpl;
   using LoggerImpl                = typename SqlDriverType::Logger;
   using SqlConnection             = sql::SqlConnection<SqlDriverType>;
   using VariantType               = typename SqlDriverType::VariantType;

   /// Alias for sql parameter implemented.
   using SqlParameter              = typename SqlDriverType::SqlParameter;

   /// Alias for SqlParameter Collection.
   using SqlParameterCollection    = typename SqlDriverType::SqlParameterCollection;

   /// Alias for SqlParameterImpl shared_ptr.
   using SqlParameterCollectionPtr = typename SqlDriverType::SqlParameterCollectionPtr;

   using ResultNavigator           = typename SqlDriverType::ResultNavigator;

   SqlResult(SqlConnection& conn)
      : _conn(conn)
   {
      _resultImpl.connection( &(_conn.connImpl()) );
      _resultImpl.notificationHandler =
         std::bind(&SqlResult::result_onNotification, this, std::placeholders::_1);
   }

   ~SqlResult()
   {
      _resultImpl.notificationHandler = nullptr;
   }

   /// Get implementation class name.
   std::string name() const
   {
      return _resultImpl.name();
   }

   /**
    * \brief Configure the result set to represent a table or view instead of a SQL query.
    * \details
    * When enabled, `runQuery()` treats the input string as a table or view name instead of
    * an arbitrary SQL command. This affects how the backend fetches data and which cursor type is used.
    *
    * **Behavior per backend:**
    * 
    * | Backend       | Effect of setOptionOpenTable(true)                                                 |
    * |---------------|------------------------------------------------------------------------------------|
    * | AdodbResult   | Opens the table/view using `adOpenKeyset` cursor and `adCmdTable`.                 | 
    * | OdbcResult    | Executes `SELECT * FROM <table/view>`. All rows are materialized in RecordVariant. |
    * | PgsqlResult   | Executes `SELECT * FROM <table/view>`. All rows are materialized in PGresult.      |
    * | SqliteResult  | Executes `SELECT * FROM <table/view>`. All rows are materialized in RecordVariant. |
    * | MySqlResult   | Executes `SELECT * FROM <table/view>`. All rows are materialized in RecordVariant. |
    * 
    * **Notes:**
    * - Works with or without `setOptionCacheData()`.  
    * - Always enables full navigation
    * - This option does not execute arbitrary SQL commands; only table/view access is supported.
    *
    * \param openTable  true to treat input as table/view name, false for normal SQL command
    */
   void setOptionOpenTable(bool openTable = true)
   {
      return _resultImpl.setOptionOpenTable(openTable);
   }


   /**
    * \brief Enable or disable caching of result data from the database.
    * \details
    * This option controls whether the SQL result rows are stored in memory (cached) 
    * or accessed directly from the backend cursor.
    * 
    * **Behavior per backend:**
    * 
    * | Backend       | Effect of setOptionCacheData(true)                                                       |
    * |---------------|------------------------------------------------------------------------------------------|
    * | AdodbResult   | Uses forward-only cursor (adOpenForwardOnly) and caches all rows into `RecordVariant`.   | 
    *                 | After caching, `_pResult` is closed                                                      |
    * | OdbcResult    | No effect; all rows are already materialized in RecordVariant                            |
    * | PgsqlResult   | No effect; all rows are already materialized in PGresult.                                |
    * | SqliteResult  | No effect; all rows are already materialized in RecordVariant.                           |
    * | MySqlResult   | No effect; all rows are already materialized in RecordVariant.                           |
    * 
    * **Notes:**
    * - Only meaningful for ADO/ODBC backends where forward-only cursors are used.
    * - Cached results allow proper navigation (`moveFirst`, `moveNext`, etc.) and row counting.
    * - Combining with `setOptionOpenTable(true)` causes `setOptionOpenTable` behavior to take precedence.
    *
    * \param cache  true to enable caching, false to access live backend cursor only
    */
   void setOptionCacheData(bool cache = true)
   {
      return _resultImpl.setOptionCacheData(cache);
   }

   /**
    * \brief Execute a SQL query or open a table/view.
    * \details
    * Generic interface for executing SQL statements.
    *
    * - If setOptionOpenTable(true) is used, the input is treated as a table/view 
    *   name instead of a free-form SQL statement.  
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
    * \param style       Parameter style used in SQL.  
    */
   bool runQuery(const std::string& sql,
               const SqlParameterCollection& parameters,
               ParameterStyle style = ParameterStyle::named)
   {
      if (sql.empty()) throw SqlException("SQL query empty");
      
      auto qry = _conn.expandNamedParams(sql, style, parameters);
      return _resultImpl.runQuery(qry, parameters);
   }

   /**
    * \brief Execute a SQL command or open a table (no parameters).
    * \details See the overload with parameters for details on behavior and options.  
    *
    * \param sql SQL command or table name (depending on option mode).  
    */
   bool runQuery(const std::string& sql)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      return _resultImpl.runQuery(sql);
   }


   ResultStatus resultStatus() const
   {
      return _resultImpl.resultStatus();
   }

   std::string columnTypeClassToString(TypeClass typeClass) const
   {
      return _resultImpl.columnTypeClassToString(typeClass);
   }

   std::string getSqlQuery() const
   {
      return _resultImpl.getSqlQuery();
   }

   bool isValid() const
   {
      return  _resultImpl.isValid();
   }

   // -------------------------------------------------------
   // Navigations
   // -------------------------------------------------------
   void moveNext()                  { _resultImpl.navigator().moveNext(); }
   void movePrevious()              { _resultImpl.navigator().movePrevious(); }
   void moveFirst()                 { _resultImpl.navigator().moveFirst(); }
   void moveLast()                  { _resultImpl.navigator().moveLast(); }
   void locate(int newRow)          { _resultImpl.navigator().locate(newRow); }
   long currentRowPosition() const  { return _resultImpl.navigator().position(); }
   bool isBof() const               { return _resultImpl.navigator().isBof(); }
   bool isEof() const               { return _resultImpl.navigator().isEof(); }

   // -------------------------------------------------------
   // Result summary
   // -------------------------------------------------------
   long totalRows() const
   {
      return _resultImpl.totalRows();
   }

   long totalColumns() const
   {
      return _resultImpl.totalColumns();
   }

   std::vector<std::string> columnNames() const
   {
      return _resultImpl.columnNames();
   }

   std::string columnName(const int columnIndex) const
   {
      return _resultImpl.columnName(columnIndex);
   }

   int columnNumber(const std::string& name) const
   {
      return _resultImpl.columnNumber(name);
   }

   std::string columnNativeTypeStr(const int columnIndex) const
   {
      return _resultImpl.columnNativeTypeStr(columnIndex);
   }

   std::string columnNativeFullTypeStr(const int columnIndex) const
   {
      return _resultImpl.columnNativeFullTypeStr(columnIndex);
   }

   TypeClass columnTypeClass(const int columnIndex) const
   {
      return _resultImpl.columnTypeClass(columnIndex);
   }

   long columnNativeType(const int columnIndex) const
   {
      return _resultImpl.columnNativeType(columnIndex);
   }

   /// Get Defined column size/length.
   long columnDefinedSize(const int columnIndex) const
   {
      return _resultImpl.columnDefinedSize(columnIndex);
   }

   /// Get Sql Data type.
   virtual DataType columnDataType(const int columnIndex)const
   {
      return _resultImpl.columnDataType(columnIndex);
   }

   // -------------------------------------------------------
   // Field value getters
   // -------------------------------------------------------

   /** 
    * \brief Get Variant value
    * Get a single field value for specified column name/ position on current row.
    * Throw RangeException for invalid column index. Column index start at 0.
    */
   VariantType getVariantValue(const int columnIndex) const
   {
      return _resultImpl.getVariantValue(columnIndex);
   }

   /// Get Variant value.
   VariantType getVariantValue(const std::string& columnName) const
   {
      return getVariantValue(columnNumber(columnName));
   }

   template< class T>
   constexpr T get(const int columnIndex) const
   {
      return std::get<T>( _resultImpl.getVariantValue(columnIndex) );
   }

   template< class T>
   constexpr T get(const std::string& columnName) const
   {
      return std::get<T>( _resultImpl.getVariantValue(columnName) );
   }

   /** 
    * \brief Get string value
    * Get a single field value for specified column name/ position on current row.
    * Throw RangeException for invalid column index. Column index start at 0.
    */
   std::string getStringValue(const int columnIndex, const std::string& valueIfNull="") const
   {
      auto res = _resultImpl.getStringValue(columnIndex);
      
      if (res == tbs::NULLSTR)
         return valueIfNull;
      else
         return res;
   }
   
   /// Get string value.
   virtual std::string getStringValue(const std::string& columnName, const std::string& valueIfNull="") const
   {
      return getStringValue(columnNumber(columnName));
   }

   long getLongValue(const int columnIndex) const
   {
      return std::stol(getStringValue(columnIndex));
   }
   long getLongValue(const std::string& columnName) const
   {
      return getLongValue(columnNumber(columnName));
   }

   bool getBoolValue(const int columnIndex) const
   {
      return util::strToBool(getStringValue(columnIndex));
   }

   bool getBoolValue(const std::string& columnName) const
   {
      return getBoolValue(columnNumber(columnName));
   }

   double getDoubleValue(const int columnIndex) const
   {
      return std::stod(getStringValue(columnIndex));
   }

   double getDoubleValue(const std::string& columnName) const
   {
      return getDoubleValue(columnNumber(columnName));
   }

   long long getLongLongValue(const int columnIndex) const
   {
      return std::stoll(getStringValue(columnIndex));
   }

   long long getLongLongValue(const std::string& columnName) const
   {
      return getLongLongValue(columnNumber(columnName));
   }

   /// @brief Get DateTime value
   /// @param columnIndex 
   /// @return DateTime with correct value if success, otherwise null date
   /// Use DateTime's isNullDateTime() to check if the returned date is a null date
   /// Note null DateTime value is returned as string: 'null'
   /// @throw AppException if invalid data type for datetime value
   DateTime getDateTimeValue(const int columnIndex) const
   {
      DataType dataType = columnDataType(columnIndex);
      DateTime datetime;
      std::string dateTimeStr = getStringValue(columnIndex);

      if (dataType == DataType::date)
      {
         if (datetime.parse(dateTimeStr, "%Y-%m-%d"))
            return datetime;
         else if (datetime.parse(dateTimeStr, "%m/%d/%Y"))
            return datetime;
      }
      else if (dataType == DataType::timestamp)
      {
         if (datetime.parse(dateTimeStr, "%Y-%m-%d %H:%M:%S"))
            return datetime;
         else if (datetime.parse(dateTimeStr, "%Y-%m-%d"))
            return datetime;
         else if (datetime.parse(dateTimeStr, "%d/%m/%Y %H:%M:%S"))
         {
            // date time string from sql server (ADO driver, come in format dd/mm/yyy hh:mm:ss)
            return datetime;
         }
         else if (datetime.parse(dateTimeStr, "%d/%m/%Y"))
            return datetime;
      }
      else if (dataType == DataType::varchar)
      {
         // Note: 
         // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
         // Sql server 2008 up "time" and "date" data type with latest MSOLEDBSQL and SQLNCLI,
         // with SQLNCLI w/DataTypeCompatibilyt=80 recognized as adVarWChar
         if (datetime.parse(dateTimeStr, "%Y-%m-%d %H:%M:%S"))
            return datetime;
         else if (datetime.parse(dateTimeStr, "%Y-%m-%d"))
            return datetime;            
         else if (datetime.parse(dateTimeStr, "%d/%m/%Y %H:%M:%S"))
         {
            // date time string from sql server (ADO driver, come in format dd/mm/yyy hh:mm:ss)
            return datetime;
         }
         else if (datetime.parse(dateTimeStr, "%d/%m/%Y"))
         {
            // date time string from sql server (ADO driver, come in format dd/mm/yyy)
            return datetime;
         }
      }
      else {
         throw AppException("invalid data type for datetime value", "SqlResult");
      }

      // return null date from DB with DateTime value as string: 'null
      datetime.setToNullDateTime();
      return datetime;
   }

   DateTime getDateTimeValue(const std::string& columnName) const
   {
      return getDateTimeValue(columnNumber(columnName));
   }

   std::string operator[](const int columnIndex) const
   {
      return getStringValue(columnIndex);
   }
   std::string operator[](const std::string& columnName) const
   {
      return getStringValue(columnName);
   }

   /// Check for null field.
   bool isNullField(const int columnIndex) const
   {
      return _resultImpl.isNullField(columnIndex);
   }
   bool isNullField(const std::string& columnName) const
   {
      return isNullField(columnNumber(columnName));
   }   

   /// Get affected rows.
   int affectedRows() const
   {
      return _resultImpl.affectedRows();
   }

   /// Get Implementation class object reference.
   ResultImpl& resultImpl()
   {
      return _resultImpl;
   }

private:

   /// Handler for notification from Implementation class.
   void result_onNotification(const NotifyEventArgs& arg)
   {
      if (arg.type == NotificationType::trace)
         _logger.info(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::debug)
         _logger.debug(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::info)
         _logger.info(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::warning)
         _logger.warn(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::error)
         _logger.error(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));
   }

   /// Implementation result object.
   ResultImpl _resultImpl;

   /// SqlConnection class.
   SqlConnection& _conn;

   /// Implementation logger class.
   LoggerImpl _logger;
};

/** @}*/

} // namespace sql
} // namespace tbs