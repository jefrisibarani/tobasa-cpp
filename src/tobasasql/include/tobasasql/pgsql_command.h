#pragma once

#include <tobasa/notifier.h>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/pgsql_parameter.h"

#include <libpq-fe.h>

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
class DataSet;

class PgsqlConnection;
class PgsqlResult;

class PgsqlCommand : public Notifier
{
   using VariantType   = DefaultVariantType;
   using VariantHelper = VariantHelper<VariantType>;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

public:
   PgsqlCommand(PgsqlConnection* conn);
   ~PgsqlCommand();

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

   PgsqlResult executeSqlResult();

   PGresult* executePgResult();

   int affectedRows() { return _affectedRows; }

   static int getAffectedRows(PGresult* result);

   std::string sqlCommandText() const { return _sql; }

private:
   /// Implemented Sql connection object.
   PgsqlConnection* _pConn = nullptr;
   PGconn*          _pPGconn = nullptr;

   std::string      _sql;
   std::string      _statementName;
   int              _parameterCount = 0;
   int              _affectedRows;
   bool             _prepared = false;

   bool             _hasBackendError = false;

   PgsqlParameterContext _pgParam;

   std::string lastBackendError();

   bool doPrepare(const std::string& sql,int totalParameter);

   void removeStatement();

   // NOTE_JEFRI: unused
   PGresult* executeParams(const std::string& sql, const SqlParameterCollection& parameters);
};


} // namespace sql
} // namespace tbs