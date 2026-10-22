
#pragma once

#include "tobasasql/exp_imp.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <tobasa/variant.h>
#include <tobasa/datetime.h>
#include "tobasasql/column_info.h"
#include "tobasasql/common_types.h"
#include "tobasasql/pool_acquire_options.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/// A value that can be passed to or returned from the database API.
using DbValue = DefaultVariantType;


// ------------------------------------------------------------------------------
/// A value and its metadata for one SQL parameter.
struct DbParameter
{
   /// Parameter name metadata; it does not control the binding order.
   std::string name;
    /// SQL data type.
   DataType    type = DataType::unknown;
   /// Value to send to the database.
   DbValue     value;
   /// Size metadata for the value.
   uint64_t    size = 0;
   /// Number of decimal digits.
   short       decimalDigits  = 0;
    /// Whether the value is unsigned.
   bool        isUnsigned = false;
    ///  Parameter direction.
   ParameterDirection direction = ParameterDirection::input;
};

// ------------------------------------------------------------------------------
/// Settings used to select and connect to a database backend.
struct DbConnectionOptions
{
   /// Database backend to use.
   BackendType backend = BackendType::sqlite; 
   /// Database connection string.
   std::string connectionString; 
   /// Password used to complete the connection string.
   std::string password; 
   /// Salt used when building a protected connection string.
   std::string securitySalt; 
   /// Whether the connection string is complete.
   bool connectionStringIsComplete = true; 
   /// Whether SQL statements are logged.
   bool logSqlQuery = false; 
   /// Whether internal SQL statements are logged.
   bool logInternalSqlQuery = false; 
};

class DbCommand;
class DbResult;
class DbTable;

// ------------------------------------------------------------------------------
/// A backend-neutral connection to a SQL database.
class TBS_SQL_API DbConnection
{
   struct Impl;
   std::unique_ptr<Impl> _impl;

   explicit DbConnection(std::unique_ptr<Impl> impl);
   friend class DbConnectionPool;

public:
   /// Create an empty connection handle.
   DbConnection() noexcept;
   /// Connect using the given backend and connection settings.
   explicit DbConnection(const DbConnectionOptions& options);
   /// Move a connection from another handle.
   DbConnection(DbConnection&& other) noexcept;
   /// Replace this handle with a connection moved from another handle.
   DbConnection& operator=(DbConnection&& other) noexcept;
   /// Release this connection handle.
   ~DbConnection();

   /// Copying connections is not supported.
   DbConnection(const DbConnection&) = delete;
   /// Copying connections is not supported.
   DbConnection& operator=(const DbConnection&) = delete;

   /// Return whether this handle has a connection.
   explicit operator bool() const noexcept;
   /// Get the connection implementation name.
   std::string name() const;
   /// Get the selected database backend.
   BackendType backendType() const;
   /// Get the current connection status.
   ConnectionStatus status() const;
   /// Get the database server version.
   std::string versionString() const;
   /// Get the database management system name.
   std::string dbmsName() const;
   /// Get the connected database name.
   std::string databaseName() const;
   /// Get the most recent inserted row ID.
   int64_t lastInsertRowid() const;
   /// Execute SQL and return the affected row count.
   int execute(const std::string& sql);
   /// Execute SQL and report whether it succeeded.
   bool executeVoid(const std::string& sql);
   /// Execute SQL and return the first value from the first row as text.
   std::string executeScalar(const std::string& sql);
   /// Start a transaction.
   bool startTransaction();
   /// Commit the current transaction.
   bool commitTransaction();
   /// Roll back the current transaction.
   bool rollbackTransaction();
   /// Enable or disable SQL statement logging.
   void setLogSqlQuery(bool enable = true);
   /// Return whether SQL statement logging is enabled.
   bool logSqlQuery() const;
   /// Enable or disable execution status logging.
   void setLogExecuteStatus(bool enable = true);
   /// Return whether execution status logging is enabled.
   bool logExecuteStatus() const;
   /// Enable or disable internal SQL logging.
   void setLogSqlQueryInternal(bool enable = true);
   /// Return whether internal SQL logging is enabled.
   bool logSqlQueryInternal() const;
   /// Set the identifier included in log messages.
   void setLogId(const std::string& logId);
   /// Get the identifier included in log messages.
   std::string logId() const;
   /// Create a command for the given SQL statement.
   DbCommand createCommand(const std::string& sql, ParameterStyle style = ParameterStyle::named);
   /// Create a table view for a table name or SQL query.
   DbTable createTable(const std::string& tableName, const std::string& sql = {}, bool readOnly = true);
};

// ------------------------------------------------------------------------------
/// A prepared SQL statement with parameter binding and execution methods.
class TBS_SQL_API DbCommand
{
   struct Impl;
   std::unique_ptr<Impl> _impl;

   explicit DbCommand(std::unique_ptr<Impl> impl);
   friend class DbConnection;

public:
   /// Create an empty command handle.
   DbCommand() noexcept;
   /// Create a command using an existing connection.
   explicit DbCommand(DbConnection& connection, const std::string& sql,
                      ParameterStyle style = ParameterStyle::named);
   /// Move a command from another handle.
   DbCommand(DbCommand&& other) noexcept;
   /// Replace this handle with a command moved from another handle.
   DbCommand& operator=(DbCommand&& other) noexcept;
   /// Release this command handle.
   ~DbCommand();

   /// Copying commands is not supported.
   DbCommand(const DbCommand&) = delete;
   /// Copying commands is not supported.
   DbCommand& operator=(const DbCommand&) = delete;

   /// Append a parameter to the binding list; its name does not reorder it.
   DbCommand& parameter(DbParameter parameter);

   /// Append a parameter in backend binding order.
   /// For named SQL placeholders, add values in placeholder appearance order; names do not match or reorder values.
   /// For native placeholders, add values in the order expected by the backend positions.
   DbCommand& addParam(const std::string& name,
                       DataType type, 
                       DbValue value,
                       uint64_t size = 0,
                       short decimalDigits = 0,
                       bool isUnsigned = false,
                       ParameterDirection direction = ParameterDirection::input);

   /// Clear the command's current execution state.
   void reset();
   /// Close the command and release its backend resources.
   void close();
   /// Execute the command and return the affected row count.
   int execute();
   /// Execute the command and report whether it succeeded.
   bool executeVoid();
   /// Execute the command and return the first value from the first row as text.
   std::string executeScalar();
   /// Execute the command and return its result rows.
   DbResult executeQuery(bool cacheData = true, bool openTable = false);
   /// Get the number of rows affected by the last execution.
   int affectedRows() const;
};

// ------------------------------------------------------------------------------
/// A navigable result set returned by a SQL command.
class TBS_SQL_API DbResult
{
   struct Impl;
   std::unique_ptr<Impl> _impl;

   explicit DbResult(std::unique_ptr<Impl> impl);
   friend class DbCommand;

public:
   /// Create an empty result handle.
   DbResult() noexcept;
   /// Move a result from another handle.
   DbResult(DbResult&& other) noexcept;
   /// Replace this handle with a result moved from another handle.
   DbResult& operator=(DbResult&& other) noexcept;
   /// Release this result handle.
   ~DbResult();

   DbResult(const DbResult&) = delete;
   DbResult& operator=(const DbResult&) = delete;

   /// Return whether this handle contains a result set.
   bool isValid() const;
   /// Get the status of the executed statement.
   ResultStatus resultStatus() const;
   /// Get the SQL statement that produced this result.
   std::string sqlQuery() const;
   /// Return whether the current position is after the last row.
   bool isEof() const;
   /// Return whether the current position is before the first row.
   bool isBof() const;
   /// Get the total number of rows.
   long totalRows() const;
   /// Get the total number of columns.
   long totalColumns() const;
   /// Get the current row position.
   long currentRowPosition() const;
   /// Move to the first row.
   void moveFirst();
   /// Move to the next row.
   void moveNext();
   /// Move to the previous row.
   void movePrevious();
   /// Move to the last row.
   void moveLast();
   /// Move to a row by its zero-based position.
   void locate(int row);
   /// Get all column names.
   std::vector<std::string> columnNames() const;
   /// Get a column name by its zero-based position.
   std::string columnName(int column) const;
   /// Get the zero-based position of a column by name.
   int columnNumber(const std::string& name) const;
   /// Get the SQL data type of a column.
   DataType columnDataType(int column) const;
   /// Get the general type category of a column.
   TypeClass columnTypeClass(int column) const;
   /// Get the backend type name of a column.
   std::string columnNativeTypeStr(int column) const;
   /// Get the full backend type description of a column.
   std::string columnNativeFullTypeStr(int column) const;
   /// Get the backend-specific type code of a column.
   long columnNativeType(int column) const;
   /// Get the declared size of a column.
   long columnDefinedSize(int column) const;
   /// Get the number of rows affected by the statement.
   int affectedRows() const;
   
   /// Return whether a column value is NULL.
   bool isNull(int column) const;
   /// Return whether a named column value is NULL.
   bool isNull(const std::string& columnName) const;
   
   /// Get a column value by its zero-based position.
   DbValue getValue(int column) const;
   /// Get a column value by name.
   DbValue getValue(const std::string& columnName) const;
   
   /// Get a column value as text, using the fallback for NULL values.
   std::string getString(const std::string& columnName, const std::string& valueIfNull = {}) const;
   /// Get a column value as text, using the fallback for NULL values.
   std::string getString(int column, const std::string& valueIfNull = {}) const;

   /// Get a column value as a 32-bit integer.
   int32_t getInt32(int column) const;
   /// Get a column value as a 32-bit integer.
   int32_t getInt32(const std::string& columnName) const;

   /// Get a column value as a 64-bit integer.
   int64_t getInt64(int column) const;
   /// Get a named column value as a 64-bit integer.
   int64_t getInt64(const std::string& columnName) const;
   
   /// Get a column value as a boolean.
   bool getBool(int column) const;
   /// Get a named column value as a boolean.
   bool getBool(const std::string& columnName) const;
   
   /// Get a column value as a float.
   float getFloat(int column) const;
   /// Get a named column value as a float.
   float getFloat(const std::string& columnName) const;
   
   /// Get a column value as a double.
   double getDouble(int column) const;
   /// Get a named column value as a double.
   double getDouble(const std::string& columnName) const;

   /// Get a column value as a date or time value.
   DateTime getDateTime(int column) const;
   /// Get a named column value as a date or time value.
   DateTime getDateTime(const std::string& columnName) const;
};

// ------------------------------------------------------------------------------
/// Stores the key and display-value pairs for a table lookup.
struct TBS_SQL_API DbLookup
{
   std::vector<std::string> keys;
   std::vector<std::string> values;
   bool mappingEnabled = true;

   /// Return whether the lookup has no entries.
   bool empty() const noexcept { return keys.empty(); }
   /// Get the number of lookup entries.
   size_t size() const noexcept { return keys.size(); }
   /// Get the key at the given position.
   std::string keyAt(size_t index) const;
   /// Get the display value at the given position.
   std::string valueAt(size_t index) const;
   /// Find the key for a display value.
   std::string keyForValue(const std::string& value) const;
   /// Find the display value for a key.
   std::string valueForKey(const std::string& key) const;
   /// Find the position of a key, or return -1 if it is missing.
   int indexOfKey(const std::string& key) const;
   /// Find the position of a display value, or return -1 if it is missing.
   int indexOfValue(const std::string& value) const;
};

// ------------------------------------------------------------------------------
/// A navigable table view that can load, edit, and save rows.
class TBS_SQL_API DbTable
{
   struct Impl;
   std::unique_ptr<Impl> _impl;

   explicit DbTable(std::unique_ptr<Impl> impl);
   friend class DbConnection;

public:
   /// Create an empty table handle.
   DbTable() noexcept;
   /// Create a table view using an existing connection.
   explicit DbTable(DbConnection& connection, const std::string& tableName,
                    const std::string& sql = {}, bool readOnly = true);
   /// Move a table from another handle.
   DbTable(DbTable&& other) noexcept;
   DbTable& operator=(DbTable&& other) noexcept;
   ~DbTable();
   DbTable(const DbTable&) = delete;
   DbTable& operator=(const DbTable&) = delete;

   /// Initialize the table and load its data.
   void init();
   /// Return whether the table is ready to use.
   bool isValid() const;
   /// Reload the current table data.
   bool reloadData();
   /// Load data using a new SQL query and read-only setting.
   bool reloadData(const std::string& sql, bool readOnly);
   /// Save pending row changes to the database.
   bool save();
   /// Move to the first row.
   void moveFirst();
   /// Move to the next row.
   void moveNext();
   /// Move to the previous row.
   void movePrevious();
   /// Move to the last row.
   void moveLast();
   /// Move to a row by its position.
   void locate(long row);
   /// Move to the first row on a page.
   void locatePage(long page);
   /// Return whether the current position is before the first row.
   bool isBof() const;
   /// Return whether the current position is after the last row.
   bool isEof() const;
   /// Return whether the table has no rows.
   bool isEmpty() const;
   /// Get the current row position.
   long currentRowPosition() const;
   /// Get the total number of rows.
   long totalRows() const;
   /// Get the total number of columns.
   int totalColumns() const;
   /// Get the number of rows loaded initially.
   int initialRowsCount() const;
   /// Add empty rows to the table.
   bool appendRows(size_t count);
   /// Delete rows starting at a position.
   bool deleteRows(int position, int count);
   /// Get the number of rows deleted successfully by the last save.
   int lastSuccessfullyDeletedRows() const;
   /// Get the position of the last modified row.
   int lastModifiedRow() const;
   /// Get the position of the last inserted row.
   int lastInsertedRow() const;
   /// Get the number of rows marked for deletion.
   int rowsDeleted() const;
   /// Get the number of rows added to the table.
   int rowsAdded() const;
   /// Get the number of rows currently cached.
   int rowsCached() const;
   /// Set the row to edit.
   void setRowToEdit(long row);
   /// Get the table name.
   std::string tableName() const;
   /// Set the table name.
   void setTableName(const std::string& name);
   /// Get the SQL query used to load the table.
   std::string sqlQuery() const;
   /// Get the zero-based position of a column by name.
   int columnPosition(const std::string& name) const;
   /// Get metadata for a column.
   ColumnInfo columnInfo(int column) const;
   /// Get the display label for a column.
   std::string columnLabel(int column) const;
   /// Get the display label for a row.
   std::string rowLabel(int row) const;
   /// Return whether the table is read-only.
   bool isReadOnly() const;
   /// Set whether the table is read-only.
   void setReadOnly(bool readOnly = true);
   /// Return whether a row is read-only.
   bool isRowReadOnly(int row) const;
   /// Return whether a row is cached.
   bool isRowCached(int row) const;
   /// Return whether a column contains text values.
   bool isColumnText(int column) const;
   /// Choose whether column labels include type names.
   void setColumnLabelWithType(bool show = true);
   /// Set the number of rows loaded on each page.
   void setPageSize(long size);
   /// Get a cell value by row and column position.
   DbValue getValue(int row, int column) const;
   /// Get a cell value by column name in the current row.
   DbValue getValue(const std::string& columnName) const;
   /// Get a cell value as text by row and column position.
   std::string getString(int row, int column) const;
   /// Get a cell value as text by column name in the current row.
   std::string getString(const std::string& columnName) const;
   /// Set a cell value by row and column position.
   void setValue(int row, int column, DbValue value);
   /// Set a cell value by column name in the current row.
   void setValue(const std::string& columnName, DbValue value);
   /// Set up lookup values for a column using a SQL query.
   bool setupLookup(const std::string& columnName, const std::string& sql, bool mapping = true);
   /// Get the lookup values for a column position.
   DbLookup lookup(int column) const;
   /// Get the lookup values for a named column.
   DbLookup lookup(const std::string& columnName) const;
};

// ------------------------------------------------------------------------------
/// Provides database connections from a shared connection pool.
class TBS_SQL_API DbConnectionPool
{
   struct Impl;
   std::unique_ptr<Impl> _impl;

public:
   /// Create a pool with the given name, settings, and maximum size.
   DbConnectionPool(const std::string& name,
                    const DbConnectionOptions& options,
                    size_t maxConnections = 4);

   /// Move a pool from another handle.
   DbConnectionPool(DbConnectionPool&& other) noexcept;
   /// Replace this pool with one moved from another handle.
   DbConnectionPool& operator=(DbConnectionPool&& other) noexcept;
   /// Release the pool handle.
   ~DbConnectionPool();

   /// Copying pools is not supported.
   DbConnectionPool(const DbConnectionPool&) = delete;
   /// Copying pools is not supported.
   DbConnectionPool& operator=(const DbConnectionPool&) = delete;

   /// Acquire a connection from the pool.
   DbConnection acquire();
   /// Acquire a connection with optional wait timeout or cancellation.
   DbConnection acquire(const PoolAcquireOptions& options);
};

/** @}*/

} // namespace sql
} // namespace tbs