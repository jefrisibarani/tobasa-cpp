#include "tobasasql/db_api.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "tobasasql/database_connector.h"
#include "tobasasql/sql_connection_pool.h"
#include "tobasasql/sql_query.h"
#include "tobasasql/sql_table.h"

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   #include "tobasasql/adodb_util.h"
#endif

namespace tbs {
namespace sql {
namespace detail {

using Binding = DbParameter;

template <typename Variant>
DbValue normalizeValue(const Variant& variant, const std::string& fallback)
{
   return std::visit(
      [&fallback](const auto& value) -> DbValue
      {
         using ValueType = std::decay_t<decltype(value)>;
         if constexpr (std::is_constructible_v<DbValue, ValueType>)
            return DbValue(value);
         else
            return DbValue(fallback);
      },
      variant);
}

class TableApi;

class ResultApi
{
public:
   virtual ~ResultApi() = default;
   virtual bool isValid() const = 0;
   virtual bool isEof() = 0;
   virtual bool isBof() const = 0;
   virtual long totalRows() const = 0;
   virtual long totalColumns() const = 0;
   virtual long currentRowPosition() const = 0;
   virtual ResultStatus resultStatus() const = 0;
   virtual std::string sqlQuery() const = 0;
   virtual void moveFirst() = 0;
   virtual void moveNext() = 0;
   virtual void movePrevious() = 0;
   virtual void moveLast() = 0;
   virtual void locate(int row) = 0;
   virtual std::vector<std::string> columnNames() const = 0;
   virtual std::string columnName(int column) const = 0;
   virtual int columnNumber(const std::string& name) const = 0;
   virtual DataType columnDataType(int column) const = 0;
   virtual TypeClass columnTypeClass(int column) const = 0;
   virtual std::string columnNativeTypeStr(int column) const = 0;
   virtual std::string columnNativeFullTypeStr(int column) const = 0;
   virtual long columnNativeType(int column) const = 0;
   virtual long columnDefinedSize(int column) const = 0;
   virtual int affectedRows() const = 0;
   virtual bool isNull(int column) const = 0;
   virtual DbValue getValue(int column) const = 0;
   virtual std::string getString(int column, const std::string& valueIfNull) const = 0;
   virtual int32_t getInt32(int column) const = 0;
   virtual int64_t getInt64(int column) const = 0;
   virtual bool getBool(int column) const = 0;
   virtual float getFloat(int column) const = 0;
   virtual double getDouble(int column) const = 0;
   virtual DateTime getDateTime(int column) const = 0;
};

class CommandApi
{
public:
   virtual ~CommandApi() = default;
   virtual void bind(Binding binding) = 0;
   virtual void reset() = 0;
   virtual void close() = 0;
   virtual int execute() = 0;
   virtual bool executeVoid() = 0;
   virtual std::string executeScalar() = 0;
   virtual std::unique_ptr<ResultApi> executeQuery(bool cacheData, bool openTable) = 0;
   virtual int affectedRows() const = 0;
};

class TableApi
{
public:
   virtual ~TableApi() = default;
   virtual void init() = 0;
   virtual bool isValid() const = 0;
   virtual bool reloadData() = 0;
   virtual bool reloadData(const std::string& sql, bool readOnly) = 0;
   virtual bool save() = 0;
   virtual void moveFirst() = 0;
   virtual void moveNext() = 0;
   virtual void movePrevious() = 0;
   virtual void moveLast() = 0;
   virtual void locate(long row) = 0;
   virtual void locatePage(long page) = 0;
   virtual bool isBof() const = 0;
   virtual bool isEof() const = 0;
   virtual bool isEmpty() const = 0;
   virtual long currentRowPosition() const = 0;
   virtual long totalRows() const = 0;
   virtual int totalColumns() const = 0;
   virtual int initialRowsCount() const = 0;
   virtual bool appendRows(size_t count) = 0;
   virtual bool deleteRows(int position, int count) = 0;
   virtual int lastSuccessfullyDeletedRows() const = 0;
   virtual int lastModifiedRow() const = 0;
   virtual int lastInsertedRow() const = 0;
   virtual int rowsDeleted() const = 0;
   virtual int rowsAdded() const = 0;
   virtual int rowsCached() const = 0;
   virtual void setRowToEdit(long row) = 0;
   virtual std::string tableName() const = 0;
   virtual void setTableName(const std::string& name) = 0;
   virtual std::string sqlQuery() const = 0;
   virtual int columnPosition(const std::string& name) const = 0;
   virtual ColumnInfo columnInfo(int column) const = 0;
   virtual std::string columnLabel(int column) const = 0;
   virtual std::string rowLabel(int row) = 0;
   virtual bool isReadOnly() const = 0;
   virtual void setReadOnly(bool readOnly) = 0;
   virtual bool isRowReadOnly(int row) = 0;
   virtual bool isRowCached(int row) = 0;
   virtual bool isColumnText(int column) = 0;
   virtual void setColumnLabelWithType(bool show) = 0;
   virtual void setPageSize(long size) = 0;
   virtual DbValue getValue(int row, int column) = 0;
   virtual std::string getString(int row, int column) = 0;
   virtual void setValue(int row, int column, const DbValue& value) = 0;
   virtual bool setupLookup(const std::string& columnName, const std::string& sql, bool mapping) = 0;
   virtual DbLookup lookup(int column) const = 0;
};

class ConnectionApi
{
public:
   virtual ~ConnectionApi() = default;
   virtual std::string name() const = 0;
   virtual BackendType backendType() const = 0;
   virtual ConnectionStatus status() const = 0;
   virtual std::string versionString() const = 0;
   virtual std::string dbmsName() const = 0;
   virtual std::string databaseName() const = 0;
   virtual int64_t lastInsertRowid() const = 0;
   virtual int execute(const std::string& sql) = 0;
   virtual bool executeVoid(const std::string& sql) = 0;
   virtual std::string executeScalar(const std::string& sql) = 0;
   virtual bool startTransaction() = 0;
   virtual bool commitTransaction() = 0;
   virtual bool rollbackTransaction() = 0;
   virtual void setLogSqlQuery(bool enable) = 0;
   virtual bool logSqlQuery() const = 0;
   virtual void setLogExecuteStatus(bool enable) = 0;
   virtual bool logExecuteStatus() const = 0;
   virtual void setLogSqlQueryInternal(bool enable) = 0;
   virtual bool logSqlQueryInternal() const = 0;
   virtual void setLogId(const std::string& logId) = 0;
   virtual std::string logId() const = 0;
   virtual std::unique_ptr<CommandApi> createCommand(const std::string& sql, ParameterStyle style) = 0;
   virtual std::unique_ptr<TableApi> createTable(const std::string& tableName, const std::string& sql,bool readOnly) = 0;
};

struct ConnectionState
{
   std::shared_ptr<void> owner;
   std::shared_ptr<ConnectionApi> connection;
};

template <typename T>
struct DriverForConnection;

template <typename Driver>
struct DriverForConnection<SqlConnection<Driver>>
{
   using Type = Driver;
};

template <typename Driver>
class TypedResult final : public ResultApi
{
   using SqlResultType = SqlResult<Driver>;
   std::shared_ptr<SqlResultType> _result;

public:
   explicit TypedResult(std::shared_ptr<SqlResultType> result)
      : _result(std::move(result))
   {
   }

   bool isValid() const override
   {
      return _result && _result->isValid();
   }

   bool isEof() override
   {
      return !_result || _result->isEof();
   }

   bool isBof() const override
   {
      return !_result || _result->isBof();
   }

   long totalRows() const override
   {
      return _result ? _result->totalRows() : 0;
   }

   long totalColumns() const override
   {
      return _result ? _result->totalColumns() : 0;
   }

   long currentRowPosition() const override
   {
      return _result ? _result->currentRowPosition() : -1;
   }

   ResultStatus resultStatus() const override
   {
      return _result ? _result->resultStatus() : ResultStatus::unknown;
   }

   std::string sqlQuery() const override
   {
      return _result ? _result->getSqlQuery() : std::string{};
   }

   void moveFirst() override
   {
      if (_result)
         _result->moveFirst();
   }

   void moveNext() override
   {
      if (_result)
         _result->moveNext();
   }

   void movePrevious() override
   {
      if (_result)
         _result->movePrevious();
   }

   void moveLast() override
   {
      if (_result)
         _result->moveLast();
   }

   void locate(int row) override
   {
      if (_result)
         _result->locate(row);
   }

   std::vector<std::string> columnNames() const override
   {
      return _result ? _result->columnNames() : std::vector<std::string>{};
   }

   std::string columnName(int column) const override
   {
      return _result ? _result->columnName(column) : std::string{};
   }

   int columnNumber(const std::string& name) const override
   {
      return _result ? _result->columnNumber(name) : -1;
   }

   DataType columnDataType(int column) const override
   {
      return _result ? _result->columnDataType(column) : DataType::unknown;
   }

   TypeClass columnTypeClass(int column) const override
   {
      return _result ? _result->columnTypeClass(column) : TypeClass::unknown;
   }

   std::string columnNativeTypeStr(int column) const override
   {
      return _result ? _result->columnNativeTypeStr(column) : std::string{};
   }

   std::string columnNativeFullTypeStr(int column) const override
   {
      return _result ? _result->columnNativeFullTypeStr(column) : std::string{};
   }

   long columnNativeType(int column) const override
   {
      return _result ? _result->columnNativeType(column) : 0;
   }

   long columnDefinedSize(int column) const override
   {
      return _result ? _result->columnDefinedSize(column) : FIELD_SIZE_UNKNOWN;
   }

   int affectedRows() const override
   {
      return _result ? _result->affectedRows() : 0;
   }

   bool isNull(int column) const override
   {
      return !_result || _result->isNullField(column);
   }

   DbValue getValue(int column) const override
   {
      if (!_result)
         return {};

      return normalizeValue(_result->getVariantValue(column), _result->getStringValue(column));
   }

   std::string getString(int column, const std::string& valueIfNull) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");
#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
      if constexpr (std::is_same_v<Driver, AdodbDriver>)
      {
         if (_result->columnDataType(column) == DataType::time)
         {
            auto value = _result->getVariantValue(column);
            if (std::holds_alternative<std::vector<uint8_t>>(value))
               return adoDbTime2VariantToString(value);
         }
      }
#endif
      return _result->getStringValue(column, valueIfNull);
   }

   int32_t getInt32(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

      return static_cast<int32_t>(_result->getLongValue(column));
   }

   int64_t getInt64(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

      return static_cast<int64_t>(_result->getLongLongValue(column));
   }

   bool getBool(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

      return _result->getBoolValue(column);
   }

   float getFloat(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

         
      return _result->getFloatValue(column);
   }

   double getDouble(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

      return _result->getDoubleValue(column);
   }

   DateTime getDateTime(int column) const override
   {
      if (!_result)
         throw std::logic_error("SQL result is not available");

      return _result->getDateTimeValue(column);
   }
};

template <typename Driver>
class TypedCommand final : public CommandApi
{
   using ConnectionType = SqlConnection<Driver>;
   using ConnectionPtr = std::shared_ptr<ConnectionType>;

   ConnectionPtr _connection;
   std::unique_ptr<SqlQuery<Driver>> _query;

public:
   TypedCommand(ConnectionPtr connection, const std::string& sql,
      ParameterStyle style)
      : _connection(std::move(connection))
      , _query(std::make_unique<SqlQuery<Driver>>(*_connection, sql, style))
   {
   }

   void bind(Binding binding) override
   {
      std::visit(
         [&](const auto& value)
         {
            _query->addParam(binding.name, binding.type, value, binding.size,
               binding.decimalDigits, binding.isUnsigned, binding.direction);
         },
         binding.value);
   }

   void reset() override
   {
      _query->reset();
   }

   void close() override
   {
      _query->close();
   }

   int execute() override
   {
      return _query->execute();
   }

   bool executeVoid() override
   {
      return _query->executeVoid();
   }

   std::string executeScalar() override
   {
      return _query->executeScalar();
   }

   std::unique_ptr<ResultApi> executeQuery(bool cacheData,
      bool openTable) override
   {
      return std::make_unique<TypedResult<Driver>>(_query->executeResult(cacheData, openTable));
   }

   int affectedRows() const override
   {
      return const_cast<SqlQuery<Driver>&>(*_query).affectedRows();
   }
};

template <typename Driver>
class TypedTable final : public TableApi
{
   using ConnectionType = SqlConnection<Driver>;
   using TableType = SqlTable<Driver>;

   std::shared_ptr<ConnectionType> _connection;
   std::unique_ptr<TableType> _table;

public:
   TypedTable(std::shared_ptr<ConnectionType> connection,
      const std::string& tableName, const std::string& sql, bool readOnly)
      : _connection(std::move(connection))
      , _table(std::make_unique<TableType>(*_connection, tableName, sql, readOnly))
   {
   }

   void init() override { _table->init(); }
   bool isValid() const override { return const_cast<TableType&>(*_table).isValid(); }
   bool reloadData() override { return _table->reloadData(); }
   bool reloadData(const std::string& sql, bool readOnly) override
   {
      return _table->reloadData(sql, readOnly);
   }
   bool save() override { return _table->saveTable(); }
   void moveFirst() override { _table->moveFirst(); }
   void moveNext() override { _table->moveNext(); }
   void movePrevious() override { _table->movePrevious(); }
   void moveLast() override { _table->moveLast(); }
   void locate(long row) override { _table->locate(row); }
   void locatePage(long page) override { _table->locatePage(page); }
   bool isBof() const override { return _table->isBof(); }
   bool isEof() const override { return _table->isEof(); }
   bool isEmpty() const override { return _table->isEmpty(); }
   long currentRowPosition() const override { return _table->currentRowPosition(); }
   long totalRows() const override { return _table->totalRows(); }
   int totalColumns() const override { return _table->totalColumns(); }
   int initialRowsCount() const override { return _table->initialRowsCount(); }
   bool appendRows(size_t count) override { return _table->appendRows(count); }
   bool deleteRows(int position, int count) override
   {
      return _table->deleteRows(position, count);
   }
   int lastSuccessfullyDeletedRows() const override
   {
      return _table->lastSuccessfullyDeletedRows();
   }
   int lastModifiedRow() const override
   {
      return const_cast<TableType&>(*_table).lastModifiedRow();
   }
   int lastInsertedRow() const override
   {
      return const_cast<TableType&>(*_table).lastInsertedRow();
   }
   int rowsDeleted() const override
   {
      return const_cast<TableType&>(*_table).rowsDeleted();
   }
   int rowsAdded() const override
   {
      return const_cast<TableType&>(*_table).rowsAdded();
   }
   int rowsCached() const override
   {
      return const_cast<TableType&>(*_table).rowsCached();
   }
   void setRowToEdit(long row) override { _table->setRowToEdit(row); }
   std::string tableName() const override { return _table->tableName(); }
   void setTableName(const std::string& name) override { _table->setTableName(name); }
   std::string sqlQuery() const override { return _table->sqlQuery(); }
   int columnPosition(const std::string& name) const override
   {
      return const_cast<TableType&>(*_table).getColumnPosition(name);
   }
   ColumnInfo columnInfo(int column) const override
   {
      return _table->getColumnInfo(column);
   }
   std::string columnLabel(int column) const override
   {
      return const_cast<TableType&>(*_table).getColLabelValue(column);
   }
   std::string rowLabel(int row) override { return _table->getRowLabelValue(row); }
   bool isReadOnly() const override
   {
      return const_cast<TableType&>(*_table).isReadOnly();
   }
   void setReadOnly(bool readOnly) override { _table->setReadOnly(readOnly); }
   bool isRowReadOnly(int row) override { return _table->isRowReadOnly(row); }
   bool isRowCached(int row) override { return _table->isRowCached(row); }
   bool isColumnText(int column) override { return _table->isColText(column); }
   void setColumnLabelWithType(bool show) override
   {
      _table->setColumnLabelWithType(show);
   }
   void setPageSize(long size) override { _table->setOptionPageSize(size); }
   DbValue getValue(int row, int column) override
   {
      auto raw = _table->getVariantValue(row, column);
      return normalizeValue(raw, _table->getStringValue(row, column));
   }
   std::string getString(int row, int column) override
   {
      return _table->getStringValue(row, column);
   }
   void setValue(int row, int column, const DbValue& value) override
   {
      std::visit([&](const auto& typedValue)
      {
         _table->setValue(row, column, typedValue);
      }, value);
   }
   bool setupLookup(const std::string& columnName, const std::string& sql, bool mapping) override
   {
      return _table->setupColumnLookups(columnName, sql, mapping);
   }
   DbLookup lookup(int column) const override
   {
      DbLookup lookup;
      auto columnMap = const_cast<TableType&>(*_table).getColumnLookups(column);
      if (columnMap)
      {
         lookup.keys = columnMap->getKeys();
         lookup.values = columnMap->getValues();
         lookup.mappingEnabled = columnMap->needMapping();
      }
      return lookup;
   }
};

template <typename Driver>
class TypedConnection final : public ConnectionApi
{
   std::shared_ptr<SqlConnection<Driver>> _connection;

public:
   explicit TypedConnection(std::shared_ptr<SqlConnection<Driver>> connection)
      : _connection(std::move(connection))
   {
   }

   std::string name() const override
   {
      return _connection->connImpl().name();
   }
   BackendType backendType() const override { return _connection->backendType(); }
   ConnectionStatus status() const override { return _connection->status(); }
   std::string versionString() const override { return _connection->versionString(); }
   std::string dbmsName() const override { return _connection->dbmsName(); }
   std::string databaseName() const override { return _connection->databaseName(); }
   int64_t lastInsertRowid() const override { return _connection->lastInsertRowid(); }
   int execute(const std::string& sql) override { return _connection->execute(sql); }
   bool executeVoid(const std::string& sql) override
   {
      return _connection->executeVoid(sql);
   }
   std::string executeScalar(const std::string& sql) override
   {
      return _connection->executeScalar(sql);
   }
   bool startTransaction() override { return _connection->startTransaction(); }
   bool commitTransaction() override { return _connection->commitTransaction(); }
   bool rollbackTransaction() override { return _connection->rollbackTransaction(); }
   void setLogSqlQuery(bool enable) override { _connection->setLogSqlQuery(enable); }
   bool logSqlQuery() const override { return _connection->logSqlQuery(); }
   void setLogExecuteStatus(bool enable) override
   {
      _connection->setLogExecuteStatus(enable);
   }
   bool logExecuteStatus() const override { return _connection->logExecuteStatus(); }
   void setLogSqlQueryInternal(bool enable) override
   {
      _connection->setLogSqlQueryInternal(enable);
   }
   bool logSqlQueryInternal() const override
   {
      return _connection->logSqlQueryInternal();
   }
   void setLogId(const std::string& logId) override { _connection->setLogId(logId); }
   std::string logId() const override { return _connection->logId(); }

   std::unique_ptr<CommandApi> createCommand(const std::string& sql, ParameterStyle style) override
   {
      return std::make_unique<TypedCommand<Driver>>(_connection, sql, style);
   }

   std::unique_ptr<TableApi> createTable(const std::string& tableName, const std::string& sql, bool readOnly) override
   {
      return std::make_unique<TypedTable<Driver>>(_connection, tableName, sql, readOnly);
   }
};

std::shared_ptr<ConnectionApi> makeConnectionApi(const SqlConnectionPtrVariant& connectionVariant)
{
   return std::visit(
      [](const auto& connection) -> std::shared_ptr<ConnectionApi>
      {
         using ConnectionType = typename std::decay_t<decltype(connection)>::element_type;
         using Driver = typename DriverForConnection<ConnectionType>::Type;
         return std::make_shared<TypedConnection<Driver>>(connection);
      },
      connectionVariant);
}

std::shared_ptr<ConnectionState> makeConnectionState(std::shared_ptr<void> owner, const std::shared_ptr<DatabaseConnector>& connector)
{
   if (!connector)
      throw std::runtime_error("Database connector is not available");

   auto state = std::make_shared<ConnectionState>();
   state->owner = std::move(owner);
   state->connection = makeConnectionApi(connector->sqlConnPtrVariant());
   return state;
}

} // namespace detail

struct DbConnection::Impl
{
   std::shared_ptr<detail::ConnectionState> state;
};

struct DbCommand::Impl
{
   std::shared_ptr<detail::ConnectionState> state;
   std::unique_ptr<detail::CommandApi> command;
};

struct DbResult::Impl
{
   std::shared_ptr<detail::ConnectionState> state;
   std::unique_ptr<detail::ResultApi> result;
};

struct DbTable::Impl
{
   std::shared_ptr<detail::ConnectionState> state;
   std::unique_ptr<detail::TableApi> table;
};

struct DbConnectionPool::Impl
{
   std::shared_ptr<::tbs::sql::ConnectionPool> pool;
};

namespace {

conf::ConnectorOption toConnectorOption(const DbConnectionOptions& options)
{
   conf::ConnectorOption connector{};
   connector.environment                  = "production";
   connector.production.dbDriver          = options.backend;
   connector.production.connectionString  = options.connectionString;
   connector.production.password          = options.password;
   connector.development                  = connector.production;
   connector.logSqlQuery                  = options.logSqlQuery;
   connector.logInternalSqlQuery          = options.logInternalSqlQuery;
   connector.securitySalt                 = options.securitySalt;
   connector.connectionStringIsComplete   = options.connectionStringIsComplete && 
                                            options.password.empty() && 
                                            options.securitySalt.empty();
   return connector;
}

} // namespace

// ------------------------------------------------------------------------------
// DbConnection
// ------------------------------------------------------------------------------

DbConnection::DbConnection() noexcept = default;
DbConnection::DbConnection(std::unique_ptr<Impl> impl) 
   : _impl(std::move(impl)) 
{}

DbConnection::DbConnection(const DbConnectionOptions& options)
{
   auto connector = std::make_shared<DatabaseConnector>(toConnectorOption(options), "db_direct");
   connector->initSqlDriver();
   if (!connector->connect())
      throw std::runtime_error("Failed to connect to database");

   auto impl = std::make_unique<Impl>();
   impl->state = detail::makeConnectionState(connector, connector);
   _impl = std::move(impl);
}

DbConnection::DbConnection(DbConnection&& other) noexcept = default;
DbConnection& DbConnection::operator=(DbConnection&& other) noexcept = default;
DbConnection::~DbConnection() = default;

DbConnection::operator bool() const noexcept
{
   return _impl && _impl->state;
}

std::string DbConnection::name() const
{
   return *this ? _impl->state->connection->name() : std::string{};
}

BackendType DbConnection::backendType() const
{
   return *this ? _impl->state->connection->backendType() : BackendType::unknown;
}

ConnectionStatus DbConnection::status() const
{
   return *this ? _impl->state->connection->status() : ConnectionStatus::bad;
}

std::string DbConnection::versionString() const
{
   return *this ? _impl->state->connection->versionString() : std::string{};
}

std::string DbConnection::dbmsName() const
{
   return *this ? _impl->state->connection->dbmsName() : std::string{};
}

std::string DbConnection::databaseName() const
{
   return *this ? _impl->state->connection->databaseName() : std::string{};
}

int64_t DbConnection::lastInsertRowid() const
{
   return *this ? _impl->state->connection->lastInsertRowid() : 0;
}

int DbConnection::execute(const std::string& sql)
{
   if (!*this)
      return -1;

   return _impl->state->connection->execute(sql);
}

bool DbConnection::executeVoid(const std::string& sql)
{
   return *this && _impl->state->connection->executeVoid(sql);
}

std::string DbConnection::executeScalar(const std::string& sql)
{
   if (!*this)
      throw std::logic_error("Database connection is not acquired");

   return _impl->state->connection->executeScalar(sql);
}

bool DbConnection::startTransaction()
{
   return *this && _impl->state->connection->startTransaction();
}

bool DbConnection::commitTransaction()
{
   return *this && _impl->state->connection->commitTransaction();
}

bool DbConnection::rollbackTransaction()
{
   return *this && _impl->state->connection->rollbackTransaction();
}

void DbConnection::setLogSqlQuery(bool enable)
{
   if (*this)
      _impl->state->connection->setLogSqlQuery(enable);
}

bool DbConnection::logSqlQuery() const
{
   return *this && _impl->state->connection->logSqlQuery();
}

void DbConnection::setLogExecuteStatus(bool enable)
{
   if (*this)
      _impl->state->connection->setLogExecuteStatus(enable);
}

bool DbConnection::logExecuteStatus() const
{
   return *this && _impl->state->connection->logExecuteStatus();
}

void DbConnection::setLogSqlQueryInternal(bool enable)
{
   if (*this)
      _impl->state->connection->setLogSqlQueryInternal(enable);
}

bool DbConnection::logSqlQueryInternal() const
{
   return *this && _impl->state->connection->logSqlQueryInternal();
}

void DbConnection::setLogId(const std::string& logId)
{
   if (*this)
      _impl->state->connection->setLogId(logId);
}

std::string DbConnection::logId() const
{
   return *this ? _impl->state->connection->logId() : std::string{};
}

DbCommand DbConnection::createCommand(const std::string& sql, ParameterStyle style)
{
   if (!*this)
      throw std::logic_error("Database connection is not acquired");

   auto impl = std::make_unique<DbCommand::Impl>();
   impl->state = _impl->state;
   impl->command = _impl->state->connection->createCommand(sql, style);
   return DbCommand(std::move(impl));
}

DbTable DbConnection::createTable(const std::string& tableName,
   const std::string& sql, bool readOnly)
{
   if (!*this)
      throw std::logic_error("Database connection is not acquired");

   auto impl = std::make_unique<DbTable::Impl>();
   impl->state = _impl->state;
   impl->table = _impl->state->connection->createTable(tableName, sql, readOnly);
   return DbTable(std::move(impl));
}


// ------------------------------------------------------------------------------
// DbCommand
// ------------------------------------------------------------------------------

DbCommand::DbCommand() noexcept = default;
DbCommand::DbCommand(std::unique_ptr<Impl> impl) : _impl(std::move(impl)) {}
DbCommand::DbCommand(DbConnection& connection, const std::string& sql, ParameterStyle style)
   : DbCommand(connection.createCommand(sql, style)) {}
DbCommand::DbCommand(DbCommand&& other) noexcept = default;
DbCommand& DbCommand::operator=(DbCommand&& other) noexcept = default;
DbCommand::~DbCommand() = default;

DbCommand& DbCommand::addParam(const std::string& name, DataType type, DbValue value,
   uint64_t size, short decimalDigits, bool isUnsigned,
   ParameterDirection direction)
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   _impl->command->bind({name, type, std::move(value), size, decimalDigits, isUnsigned, direction});
   return *this;
}

DbCommand& DbCommand::parameter(DbParameter parameter)
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   _impl->command->bind(std::move(parameter));
   return *this;
}

void DbCommand::reset()
{
   if (_impl && _impl->command)
      _impl->command->reset();
}

void DbCommand::close()
{
   if (_impl && _impl->command)
      _impl->command->close();
}

int DbCommand::execute()
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   return _impl->command->execute();
}

bool DbCommand::executeVoid()
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   return _impl->command->executeVoid();
}

std::string DbCommand::executeScalar()
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   return _impl->command->executeScalar();
}

DbResult DbCommand::executeQuery(bool cacheData, bool openTable)
{
   if (!_impl || !_impl->command)
      throw std::logic_error("SQL command is not available");

   auto result = std::make_unique<DbResult::Impl>();
   result->state = _impl->state;
   result->result = _impl->command->executeQuery(cacheData, openTable);
   return DbResult(std::move(result));
}

int DbCommand::affectedRows() const
{
   return _impl && _impl->command ? _impl->command->affectedRows() : 0;
}


// ------------------------------------------------------------------------------
// DbResult
// ------------------------------------------------------------------------------

DbResult::DbResult() noexcept = default;
DbResult::DbResult(std::unique_ptr<Impl> impl) : _impl(std::move(impl)) {}
DbResult::DbResult(DbResult&& other) noexcept = default;
DbResult& DbResult::operator=(DbResult&& other) noexcept = default;
DbResult::~DbResult() = default;

bool DbResult::isValid() const
{
   return _impl && _impl->result && _impl->result->isValid();
}

bool DbResult::isEof() const
{
   return !_impl || !_impl->result || _impl->result->isEof();
}

long DbResult::totalRows() const
{
   return _impl && _impl->result ? _impl->result->totalRows() : 0;
}

ResultStatus DbResult::resultStatus() const
{
   return _impl && _impl->result ? _impl->result->resultStatus() : ResultStatus::unknown;
}

std::string DbResult::sqlQuery() const
{
   return _impl && _impl->result ? _impl->result->sqlQuery() : std::string{};
}

bool DbResult::isBof() const
{
   return !_impl || !_impl->result || _impl->result->isBof();
}

long DbResult::totalColumns() const
{
   return _impl && _impl->result ? _impl->result->totalColumns() : 0;
}

long DbResult::currentRowPosition() const
{
   return _impl && _impl->result ? _impl->result->currentRowPosition() : -1;
}

void DbResult::moveFirst()
{
   if (_impl && _impl->result)
      _impl->result->moveFirst();
}

void DbResult::moveNext()
{
   if (_impl && _impl->result)
      _impl->result->moveNext();
}

void DbResult::movePrevious()
{
   if (_impl && _impl->result)
      _impl->result->movePrevious();
}

void DbResult::moveLast()
{
   if (_impl && _impl->result)
      _impl->result->moveLast();
}

void DbResult::locate(int row)
{
   if (_impl && _impl->result)
      _impl->result->locate(row);
}

std::vector<std::string> DbResult::columnNames() const
{
   return _impl && _impl->result
      ? _impl->result->columnNames() : std::vector<std::string>{};
}

std::string DbResult::columnName(int column) const
{
   return _impl && _impl->result ? _impl->result->columnName(column) : std::string{};
}

int DbResult::columnNumber(const std::string& name) const
{
   return _impl && _impl->result ? _impl->result->columnNumber(name) : -1;
}

DataType DbResult::columnDataType(int column) const
{
   return _impl && _impl->result ? _impl->result->columnDataType(column) : DataType::unknown;
}

TypeClass DbResult::columnTypeClass(int column) const
{
   return _impl && _impl->result ? _impl->result->columnTypeClass(column) : TypeClass::unknown;
}

std::string DbResult::columnNativeTypeStr(int column) const
{
   return _impl && _impl->result ? _impl->result->columnNativeTypeStr(column) : std::string{};
}

std::string DbResult::columnNativeFullTypeStr(int column) const
{
   return _impl && _impl->result ? _impl->result->columnNativeFullTypeStr(column) : std::string{};
}

long DbResult::columnNativeType(int column) const
{
   return _impl && _impl->result ? _impl->result->columnNativeType(column) : 0;
}

long DbResult::columnDefinedSize(int column) const
{
   return _impl && _impl->result ? _impl->result->columnDefinedSize(column) : FIELD_SIZE_UNKNOWN;
}

int DbResult::affectedRows() const
{
   return _impl && _impl->result ? _impl->result->affectedRows() : 0;
}

bool DbResult::isNull(int column) const
{
   return !_impl || !_impl->result || _impl->result->isNull(column);
}

bool DbResult::isNull(const std::string& columnName) const
{
   return isNull(columnNumber(columnName));
}

DbValue DbResult::getValue(int column) const
{
   return _impl && _impl->result ? _impl->result->getValue(column) : DbValue{};
}

DbValue DbResult::getValue(const std::string& columnName) const
{
   return getValue(columnNumber(columnName));
}

std::string DbResult::getString(int column, const std::string& valueIfNull) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getString(column, valueIfNull);
}

int32_t DbResult::getInt32(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getInt32(columnNumber(columnName));
}

int32_t DbResult::getInt32(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getInt32(column);
}

int64_t DbResult::getInt64(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getInt64(column);
}

bool DbResult::getBool(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getBool(columnNumber(columnName));
}

bool DbResult::getBool(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getBool(column);
}

float DbResult::getFloat(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getFloat(columnNumber(columnName));
}

float DbResult::getFloat(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getFloat(column);
}

double DbResult::getDouble(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getDouble(columnNumber(columnName));
}

double DbResult::getDouble(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getDouble(column);
}

DateTime DbResult::getDateTime(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getDateTime(columnNumber(columnName));
}

DateTime DbResult::getDateTime(int column) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getDateTime(column);
}

std::string DbResult::getString(const std::string& columnName, const std::string& valueIfNull) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getString(_impl->result->columnNumber(columnName), valueIfNull);
}

int64_t DbResult::getInt64(const std::string& columnName) const
{
   if (!_impl || !_impl->result)
      throw std::logic_error("SQL result is not available");

   return _impl->result->getInt64(_impl->result->columnNumber(columnName));
}


// ------------------------------------------------------------------------------
// DbLookup
// ------------------------------------------------------------------------------

std::string DbLookup::keyAt(size_t index) const
{
   return index < keys.size() ? keys[index] : std::string{};
}

std::string DbLookup::valueAt(size_t index) const
{
   return index < values.size() ? values[index] : std::string{};
}

std::string DbLookup::keyForValue(const std::string& value) const
{
   auto found = std::find(values.begin(), values.end(), value);
   return found == values.end() ? std::string{} : keys[found - values.begin()];
}

std::string DbLookup::valueForKey(const std::string& key) const
{
   auto found = std::find(keys.begin(), keys.end(), key);
   return found == keys.end() ? std::string{} : values[found - keys.begin()];
}

int DbLookup::indexOfKey(const std::string& key) const
{
   auto found = std::find(keys.begin(), keys.end(), key);
   return found == keys.end() ? -1 : static_cast<int>(found - keys.begin());
}

int DbLookup::indexOfValue(const std::string& value) const
{
   auto found = std::find(values.begin(), values.end(), value);
   return found == values.end() ? -1 : static_cast<int>(found - values.begin());
}


// ------------------------------------------------------------------------------
// DbTable
// ------------------------------------------------------------------------------

DbTable::DbTable() noexcept = default;
DbTable::DbTable(std::unique_ptr<Impl> impl) : _impl(std::move(impl)) {}

DbTable::DbTable(DbConnection& connection, const std::string& tableName, const std::string& sql, bool readOnly)
   : DbTable(connection.createTable(tableName, sql, readOnly)) 
{}

DbTable::DbTable(DbTable&& other) noexcept = default;
DbTable& DbTable::operator=(DbTable&& other) noexcept = default;
DbTable::~DbTable() = default;

void DbTable::init()
{
   if (_impl && _impl->table)
      _impl->table->init();
}

bool DbTable::isValid() const
{
   return _impl && _impl->table && _impl->table->isValid();
}

bool DbTable::reloadData()
{
   return _impl && _impl->table && _impl->table->reloadData();
}

bool DbTable::reloadData(const std::string& sql, bool readOnly)
{
   return _impl && _impl->table && _impl->table->reloadData(sql, readOnly);
}

bool DbTable::save()
{
   return _impl && _impl->table && _impl->table->save();
}

void DbTable::moveFirst() { if (_impl && _impl->table) _impl->table->moveFirst(); }
void DbTable::moveNext() { if (_impl && _impl->table) _impl->table->moveNext(); }
void DbTable::movePrevious() { if (_impl && _impl->table) _impl->table->movePrevious(); }
void DbTable::moveLast() { if (_impl && _impl->table) _impl->table->moveLast(); }
void DbTable::locate(long row) { if (_impl && _impl->table) _impl->table->locate(row); }
void DbTable::locatePage(long page) { if (_impl && _impl->table) _impl->table->locatePage(page); }
bool DbTable::isBof() const { return !_impl || !_impl->table || _impl->table->isBof(); }
bool DbTable::isEof() const { return !_impl || !_impl->table || _impl->table->isEof(); }
bool DbTable::isEmpty() const { return !_impl || !_impl->table || _impl->table->isEmpty(); }
long DbTable::currentRowPosition() const { return _impl && _impl->table ? _impl->table->currentRowPosition() : -1; }
long DbTable::totalRows() const { return _impl && _impl->table ? _impl->table->totalRows() : 0; }
int DbTable::totalColumns() const { return _impl && _impl->table ? _impl->table->totalColumns() : 0; }
int DbTable::initialRowsCount() const { return _impl && _impl->table ? _impl->table->initialRowsCount() : 0; }
bool DbTable::appendRows(size_t count) { return _impl && _impl->table && _impl->table->appendRows(count); }
bool DbTable::deleteRows(int position, int count) { return _impl && _impl->table && _impl->table->deleteRows(position, count); }
int DbTable::lastSuccessfullyDeletedRows() const { return _impl && _impl->table ? _impl->table->lastSuccessfullyDeletedRows() : 0; }
int DbTable::lastModifiedRow() const { return _impl && _impl->table ? _impl->table->lastModifiedRow() : -1; }
int DbTable::lastInsertedRow() const { return _impl && _impl->table ? _impl->table->lastInsertedRow() : -1; }
int DbTable::rowsDeleted() const { return _impl && _impl->table ? _impl->table->rowsDeleted() : 0; }
int DbTable::rowsAdded() const { return _impl && _impl->table ? _impl->table->rowsAdded() : 0; }
int DbTable::rowsCached() const { return _impl && _impl->table ? _impl->table->rowsCached() : 0; }
void DbTable::setRowToEdit(long row) { if (_impl && _impl->table) _impl->table->setRowToEdit(row); }
std::string DbTable::tableName() const { return _impl && _impl->table ? _impl->table->tableName() : std::string{}; }
void DbTable::setTableName(const std::string& name) { if (_impl && _impl->table) _impl->table->setTableName(name); }
std::string DbTable::sqlQuery() const { return _impl && _impl->table ? _impl->table->sqlQuery() : std::string{}; }
int DbTable::columnPosition(const std::string& name) const { return _impl && _impl->table ? _impl->table->columnPosition(name) : -1; }
ColumnInfo DbTable::columnInfo(int column) const { return _impl && _impl->table ? _impl->table->columnInfo(column) : ColumnInfo{}; }
std::string DbTable::columnLabel(int column) const { return _impl && _impl->table ? _impl->table->columnLabel(column) : std::string{}; }
std::string DbTable::rowLabel(int row) const { return _impl && _impl->table ? _impl->table->rowLabel(row) : std::string{}; }
bool DbTable::isReadOnly() const { return !_impl || !_impl->table || _impl->table->isReadOnly(); }
void DbTable::setReadOnly(bool readOnly) { if (_impl && _impl->table) _impl->table->setReadOnly(readOnly); }
bool DbTable::isRowReadOnly(int row) const { return !_impl || !_impl->table || _impl->table->isRowReadOnly(row); }
bool DbTable::isRowCached(int row) const { return _impl && _impl->table && _impl->table->isRowCached(row); }
bool DbTable::isColumnText(int column) const { return !_impl || !_impl->table || _impl->table->isColumnText(column); }
void DbTable::setColumnLabelWithType(bool show) { if (_impl && _impl->table) _impl->table->setColumnLabelWithType(show); }
void DbTable::setPageSize(long size) { if (_impl && _impl->table) _impl->table->setPageSize(size); }

DbValue DbTable::getValue(int row, int column) const
{
   return _impl && _impl->table ? _impl->table->getValue(row, column) : DbValue{};
}

DbValue DbTable::getValue(const std::string& columnName) const
{
   return getValue(static_cast<int>(currentRowPosition()), columnPosition(columnName));
}

std::string DbTable::getString(int row, int column) const
{
   return _impl && _impl->table ? _impl->table->getString(row, column) : std::string{};
}

std::string DbTable::getString(const std::string& columnName) const
{
   return getString(static_cast<int>(currentRowPosition()), columnPosition(columnName));
}

void DbTable::setValue(int row, int column, DbValue value)
{
   if (!_impl || !_impl->table)
      throw std::logic_error("SQL table is not available");

   _impl->table->setValue(row, column, value);
}

void DbTable::setValue(const std::string& columnName, DbValue value)
{
   setValue(static_cast<int>(currentRowPosition()), columnPosition(columnName), std::move(value));
}

bool DbTable::setupLookup(const std::string& columnName, const std::string& sql, bool mapping)
{
   return _impl && _impl->table && _impl->table->setupLookup(columnName, sql, mapping);
}

DbLookup DbTable::lookup(int column) const
{
   return _impl && _impl->table ? _impl->table->lookup(column) : DbLookup{};
}

DbLookup DbTable::lookup(const std::string& columnName) const
{
   return lookup(columnPosition(columnName));
}


// ------------------------------------------------------------------------------
// DbConnectionPool
// ------------------------------------------------------------------------------

DbConnectionPool::DbConnectionPool(const std::string& name, const DbConnectionOptions& options, size_t maxConnections)
   : _impl(std::make_unique<Impl>())
{
   if (maxConnections == 0)
      throw std::invalid_argument("Database connection pool size must be greater than zero");

   _impl->pool = std::make_shared<::tbs::sql::ConnectionPool>( name, toConnectorOption(options), maxConnections);
}

DbConnectionPool::DbConnectionPool(DbConnectionPool&& other) noexcept = default;
DbConnectionPool& DbConnectionPool::operator=(DbConnectionPool&& other) noexcept = default;
DbConnectionPool::~DbConnectionPool() = default;

DbConnection DbConnectionPool::acquire()
{
   return acquire(PoolAcquireOptions{});
}

DbConnection DbConnectionPool::acquire(const PoolAcquireOptions& options)
{
   if (!_impl || !_impl->pool)
      throw std::logic_error("Database connection pool is not available");

   auto lease = std::make_shared<PooledConnection>(_impl->pool->acquire(options));
   auto connector = lease->connector();
   if (!connector || !connector->connected())
      throw std::runtime_error("Database connection unavailable");

   auto impl = std::make_unique<DbConnection::Impl>();
   impl->state = detail::makeConnectionState(lease, connector);
   return DbConnection(std::move(impl));
}

} // namespace sql
} // namespace tbs