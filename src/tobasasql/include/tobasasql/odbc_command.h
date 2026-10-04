#pragma once

#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <tobasa/notifier.h>
#include "tobasasql/odbc_parameter.h"
#include <memory>

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
class DataSet;

class OdbcConnection;
class OdbcResult;

class OdbcCommand : public Notifier
{
   using VariantType   = DefaultVariantType;
   using VariantHelper = VariantHelper<VariantType>;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

public:
   OdbcCommand(OdbcConnection* conn);
   ~OdbcCommand();

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

   OdbcResult executeSqlResult();

   int affectedRows();

   std::string sqlCommandText() const { return _sql; }

   SQLHSTMT statement() const { return _pStatement; }

   /**
   * \brief Supply supply data-at-execution.
   * Note: https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/sending-long-data?view=sql-server-ver15
   */
   static SQLRETURN sqlPutData(OdbcConnection* conn, SQLHSTMT pStmt, const SqlParameterCollection& parameters);   

   // Note: ODBC column indexing is 1-based, not 0-based.
   static VariantType getFieldData(OdbcConnection* conn, SQLHSTMT pStmt, int col);

private:

   OdbcConnection* _pConn;
   SQLHDBC         _pOdbcConn;
   SQLHSTMT        _pStatement;
   int64_t         _affectedRows;
   bool            _resultSetOpen = false;
   std::string     _sql;

   const SqlParameterCollection* _boundParameters;

   std::unique_ptr<OdbcParameterCollection> _parameterCollection;

   /// Get last backend error.
   std::string lastBackendError();
   std::string statementError();
};


} // namespace sql
} // namespace tbs