#pragma once

#include <tobasa/notifier.h>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/sqlite_type.h"

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
class DataSet;

class SqliteConnection;
class SqliteResult;

class SqliteCommand : public Notifier
{
   using VariantType   = DefaultVariantType;
   using VariantHelper = VariantHelper<VariantType>;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

public:
   SqliteCommand(SqliteConnection* conn);
   ~SqliteCommand();

   /// Prepare for one-shot query execution
   bool query(const std::string& sql, const SqlParameterCollection& parameters);

   bool prepare(const std::string& sql);
   bool bind(const SqlParameterCollection& parameters);
   void reset();
   void close();

   /**
    * @brief Executes the prepared query.
    * @return - The number of affected rows for INSERT, UPDATE, or DELETE
    *         - Zero for SELECT and other queries that do not affect rows.
    *         - -1 if invalid operation occured
    * Throwing Exception when error occured
    */
   int execute();

   /**
    * @brief Executes the prepared query.
    * @return - The first column of the first retrieved row, as std::string
    *         - Returned value may be empty string
    * Throwing Exception when error occured
    */
   std::string executeScalar();

   /**
    * @brief Executes the prepared query and returns its result set.
    * @return A shared dataset for row-producing queries such as SELECT;
    *         an empty shared pointer for INSERT, UPDATE, or DELETE.
    * Throwing Exception when error occured
    */
   DataSetPtr executeResult();

   SqliteResult executeSqlResult();

   int affectedRows();

   std::string sqlCommandText() const { return _sql; }

   sqlite3_stmt* statement() const { return _pStatement; }

private:
   SqliteConnection* _pConn;
   sqlite3*          _pSqliteDb;
   sqlite3_stmt*     _pStatement;
   int               _affectedRows;
   std::string       _sql;

   /// Get last backend error.
   std::string lastBackendError();
   std::string statementError();
};


} // namespace sql
} // namespace tbs