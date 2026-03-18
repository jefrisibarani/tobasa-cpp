#pragma once

#include "tobasasql/sql_connection_common.h"
#include "tobasasql/sql_parameter.h"
#include <libpq-fe.h>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief PostgreSQL SQL Connection class.
 * \details
 * Backend implementation using libpq (PGconn).  
 * - Connects to PostgreSQL server using connection string or individual parameters.
  */
class PgsqlConnection : public ConnectionCommon
{
public:
   PgsqlConnection();
   ~PgsqlConnection();

   std::string name() const;
   bool connect(const std::string& connString);
   bool disconnect();
   ConnectionStatus status();

   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details
    *  On successful execution of a data-modifying statement (INSERT, UPDATE, DELETE),
    *  returns the number of affected rows (≥ 0).  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns -1.  
    *
    *  If the SQL command produces a result set (e.g. SELECT), the result is ignored
    *  and the affected row count is reported as 0.
    *
    * \param sql        SQL command query
    * \param parameters Collection of parameters to bind to the query.
    */
   int execute(
      const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection() );

   /** 
    * \brief Executes a parameterized SQL query and returns the first column of the first row.
    * \details
    *  If the query executes successfully, returns the value as a string (may be empty).
    *  If execution fails or the connection is invalid, throws SqlException.
    * \param sql        SQL command query with placeholders.
    * \param parameters Collection of parameters to bind to the query.
    */
   std::string executeScalar(
      const std::string& sql,
      const SqlParameterCollection& parameters = SqlParameterCollection());

   std::string versionString();

   std::string databaseName();

   BackendType backendType() const;

   std::string dbmsName();

   /// EXPERIMENTAL. DO NOT USE THIS!
   int64_t lastInsertRowid();

   // -------------------------------------------------------
   // Specific implementation functions
   // -------------------------------------------------------

   /// Get last backend error.
   std::string lastBackendError();

   /// Get pointer to native driver's connection object.
   PGconn* nativeConn() const;

   int transactionStatus();

   /// Callback for libpq' PQsetNoticeProcessor, to process notice from PostgreSQL.
   //  use registerNoticeProcessor() to register this callback in libpq
   static void pgNoticeProcessor(void* arg, const char* message);

   PGresult* executeParams(const std::string& sql, const SqlParameterCollection& parameters);

private:

   void registerNoticeProcessor(PQnoticeProcessor proc, void* arg);

   /// Callback for libpq' PQsetNoticeProcessor.
   void processNotice(const char* msg);

   /// PostgreSQL native connection object.
   PGconn*  _pPGconn;

   long _lastSystemOID;
   long _dbOid;
   bool _needColumnQuoting;
};

} // namespace sql
} // namespace tbs