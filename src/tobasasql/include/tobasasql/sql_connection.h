#pragma once

#include <string>
#include <memory>
#include <tobasa/notifier.h>
#include <tobasa/self_counter.h>
#include "tobasasql/exception.h"
#include "tobasasql/sql_result.h"
#include "tobasasql/sql_parameter_rewriter.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/**
 * \brief Generic SQL connection wrapper.
 * \details 
 * This template class provides a driver-agnostic wrapper around the underlying 
 * SQL connection implementation. It manages connection state, command execution, 
 * and query handling in a unified interface.
 *
 * Typical usage:
 * - Establish or close a database connection.
 * - Execute parameterized or non-parameterized SQL commands with `execute*()` methods.
 *
 * \tparam SqlDriverType The driver-specific connection implementation type.
 */
template < typename SqlDriverType >
class SqlConnection
{
public:
   /// Alias for implementation sql connection class.
   using ConnectionImpl    = typename SqlDriverType::ConnectionImpl;

   /// Alias for implementation sql connection class's shared_ptr.
   using ConnectionImplPtr = std::shared_ptr<ConnectionImpl>;

   /// Alias for implementation sql result class.
   using ResultImpl        = typename SqlDriverType::ResultImpl;

   /// Alias for implementation sql result class's shared_ptr.
   using ResultImplPtr     = std::shared_ptr<ResultImpl>;

   /// Alias for implementation logger class.
   using LoggerImpl        = typename SqlDriverType::Logger;

   /// Alias for sql result template class.
   using SqlResult         = sql::SqlResult<SqlDriverType>;

   /// Alias for sql result template class's shared_ptr.
   using SqlResultPtr      = std::shared_ptr< SqlResult >;

   /// Alias for sql parameter implemented.
   using SqlParameter      = typename SqlDriverType::SqlParameter;

   /// Alias for SqlParameter Collection.
   using SqlParameterCollection = typename SqlDriverType::SqlParameterCollection;

   /// Alias for SqlParameterImpl shared_ptr.
   using SqlParameterCollectionPtr = typename SqlDriverType::SqlParameterCollectionPtr;

   using VariantType    = typename SqlDriverType::VariantType;
   using VariantHelper  = typename SqlDriverType::VariantHelper;

   /// Constructor.
   SqlConnection()
   {
      _connImpl.notificationHandler
         = std::bind(&SqlConnection::conn_onNotification, this, std::placeholders::_1);
   }

   /// Destructor.
   ~SqlConnection()
   {
      if (status() == ConnectionStatus::ok) {
         disconnect();
      }

      _connImpl.notificationHandler = nullptr;
      _logger.trace("[sql] SqlConnection destroyed");
   }

   /// Get implementation class name.
   std::string name() const
   {
      return _connImpl.name();
   }

   /// Connect to backend database.
   bool connect(const std::string& connString)
   {
      _logger.debug("[sql] Connecting to database");
      return _connImpl.connect(connString);
   }

   /// Disconnect from backend database.
   bool disconnect()
   {
      _logger.debug("[sql] Disconnecting database");
      return _connImpl.disconnect();
   }

   /// Get connection status.
   ConnectionStatus status()
   {
      return _connImpl.status();
   }

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
    * \param style      Parameter style used in the SQL (default: ParameterStyle::Named).
    */
   int execute(const std::string& sql,
               const SqlParameterCollection& parameters,
               ParameterStyle style = ParameterStyle::named)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      auto qry = expandNamedParams(sql, style, parameters);
      return _connImpl.execute(qry, parameters);
   }

   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details See the overload with parameters for details on behavior and options.
    *  On successful execution of a data-modifying statement (INSERT, UPDATE, DELETE),
    *  returns the number of affected rows (≥ 0).  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns -1.  
    *
    *  If the SQL command produces a result set (e.g. SELECT), the result is ignored
    *  and the affected row count is reported as 0.
    *
    * \param sql SQL command query.
    */
   int execute(const std::string& sql)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      return _connImpl.execute(sql);
   }


   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details
    *  If execution succeeds, returns true.  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns false.  
    *  Any result set produced by the command is ignored; only the affected row count is considered.
    * \param sql        SQL command or stored procedure text.
    * \param parameters Collection of parameters to bind to the query.
    * \param style      Parameter style used in the SQL (default: ParameterStyle::Named).
    */
   bool executeVoid(const std::string& sql,
      const SqlParameterCollection& parameters,
      ParameterStyle style = ParameterStyle::named)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      auto qry = expandNamedParams(sql, style, parameters);
      int rc = _connImpl.execute(qry, parameters);
      return rc >= 0;
   }

   /** 
    * \brief Executes a SQL command or stored procedure that does not return rows.
    * \details
    *  If execution succeeds, returns true.  
    *  If execution fails, throws SqlException.  
    *  If the connection is invalid, returns false.  
    *  Any result set produced by the command is ignored; only the affected row count is considered.
    * \param sql SQL command or stored procedure text.
    */
   bool executeVoid( const std::string& sql)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      int rc = _connImpl.execute(sql);
      return rc >= 0;
   }

   /** 
    * \brief Executes a parameterized SQL query and returns the first column of the first row.
    * \details
    *  If the query executes successfully, returns the value as a string (may be empty).
    *  If execution fails or the connection is invalid, throws SqlException.
    * \param sql        SQL command query with placeholders.
    * \param parameters Collection of parameters to bind to the query.
    * \param style      Parameter style used in the SQL (default: ParameterStyle::Named).
    */
   std::string executeScalar(const std::string& sql,
      const SqlParameterCollection& parameters,
      ParameterStyle style = ParameterStyle::named)
   {
      if (sql.empty()) throw SqlException("SQL query empty");

      auto qry = expandNamedParams(sql, style, parameters);
      return _connImpl.executeScalar(qry, parameters);
   }

   /** 
    * \brief Executes a SQL query and returns the first column of the first row.
    * \details
    *  If the query executes successfully, returns the value as a string (may be empty).
    *  If execution fails or the connection is invalid, throws SqlException.
    * \param sql  SQL command query.
    */
   std::string executeScalar(const std::string& sql)
   {
      if (sql.empty()) throw SqlException("SQL query empty");
      
      return _connImpl.executeScalar(sql);
   }

   /// Get backend version information.
   std::string versionString()
   {
      return _connImpl.versionString();
   }

   /// Get reference to internal implementation connection object.
   ConnectionImpl& connImpl()
   {
      return _connImpl;
   }

   BackendType backendType() const
   {
      return _connImpl.backendType();
   }

   /// Get last inserted row id.
   int64_t lastInsertRowid()
   {
      return _connImpl.lastInsertRowid();
   }

   void setLogSqlQuery(bool enable=true)
   {
      _connImpl.setLogSqlQuery(enable);
   }

   bool logSqlQuery()
   {
      return _connImpl.logSqlQuery();
   }

   void setLogExecuteStatus(bool enable=true)
   {
      _connImpl.setLogExecuteStatus(enable);
   }

   bool logExecuteStatus()
   {
      return _connImpl.logExecuteStatus();
   }   

   void setLogSqlQueryInternal(bool enable = true)
   {
      _connImpl.setLogSqlQueryInternal(enable);
   }

   bool logSqlQueryInternal()
   {
      return  _connImpl.logSqlQueryInternal();
   }

   void setLogId(const std::string& logId)
   {
      _connImpl.setLogId(logId);
   }

   std::string logId()
   {
      return  _connImpl.logId();
   }


   /**
    * \brief Factory method to create a SQL parameter.
    * \details 
    * Constructs and returns a new \c SqlParameter object wrapped in a 
    * \c std::shared_ptr. Parameters can represent input, output, or 
    * input/output values for a SQL command or stored procedure.
    *
    * \param name           Logical parameter name (used for named parameters).
    * \param type           SQL data type of the parameter.
    * \param value          Initial parameter value.
    * \param size           Maximum size of the parameter (for strings/binary). Default is 0.
    * \param direction      Parameter direction (input, output, input/output). Default is input.
    * \param decimalDigits  Number of decimal digits (for numeric/decimal types). Default is 0.
    * \return A shared pointer to the created \c SqlParameter instance.
    */
   std::shared_ptr<SqlParameter> createParameter(
         const std::string&  name,
         DataType            type,
         VariantType         value,
         long                size = 0,
         ParameterDirection  direction = ParameterDirection::input,
         short               decimalDigits = 0)
   {
      return std::make_shared<SqlParameter>(name, type, value, size, direction, decimalDigits);
   }

   /** 
    * \brief Get real backend name.
    * For ODBC connection, this is the driver name. Non odbc, same as name()
    * \return std::string
    */
   std::string dbmsName()
   {
      if (_connImpl.backendType() != BackendType::odbc)
         return _connImpl.name();
      else
         return _connImpl.dbmsName();
   }

   /** 
    * \brief Get database name of this connection
    */
   std::string databaseName()
   {
      return _connImpl.databaseName();
   }

   std::string expandNamedParams(const std::string& sql, ParameterStyle style, const SqlParameterCollection& parameters ) 
   {
      if ( style == ParameterStyle::named && parameters.size()>0)
      {
         SqlParameterRewriter writer( backendType() );
         return writer.rewrite(sql);
      }
      return sql;
   }

private:

   /// Handler for notification from Implementation class.
   void conn_onNotification(const NotifyEventArgs& arg)
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

   /// Implementation connection object.
   ConnectionImpl _connImpl;

   /// Implementation logger class.
   LoggerImpl _logger;
};

/**
 * \brief SqlConnection shared pointer
 */
template < typename SqlDriverType >
using SqlConnectionPtr = std::shared_ptr< SqlConnection<SqlDriverType> >;

/** @}*/

} // namespace sql
} // namespace tbs