#pragma once

#include <tobasa/util_string.h>
#include <tobasa/self_counter.h>
#include "tobasasql/exception.h"
#include "tobasasql/sql_connection.h"
#include "tobasasql/sql_util.h"

namespace tbs {
namespace sql {


/** 
 * \ingroup SQL
 * \brief Sql Query class.
 * \tparam SqlDriverType
 */
template <typename SqlDriverType>
class SqlQuery
{
public:
   using LoggerImpl             = typename SqlDriverType::Logger;
   using SqlResult              = sql::SqlResult<SqlDriverType>;
   using SqlConnection          = sql::SqlConnection<SqlDriverType>;
   using CommandImpl            = typename SqlDriverType::CommandImpl;
   using VariantType            = typename SqlDriverType::VariantType;
   using VectorVariant          = std::vector<VariantType>;
   using VariantHelper          = typename SqlDriverType::VariantHelper;
   using SqlParameter           = typename SqlDriverType::SqlParameter;
   using SqlParameterCollection = typename SqlDriverType::SqlParameterCollection;

   /**
    * @brief Construct a new SqlQuery object.
    * @param Active database connection used for executing the query.
    */
   SqlQuery(SqlConnection& conn)
      : _conn(conn)
      , _cmdImpl(&conn.connImpl())
      , _parameterStyle(ParameterStyle::named)
   {
   }

   /**
    * @brief Construct a new SqlQuery object.
    *
    * This constructor prepares an SQL query for execution against the given
    * database connection. The query may contain either named parameters (colon
    * syntax, e.g. ":id") or DB-native parameter placeholders (e.g. "$1" for
    * PostgreSQL, "?" for SQLite/MySQL, "@p1" for MSSQL).
    *
    * @param conn  Active database connection used for executing the query.
    * @param sql   SQL query string. Depending on the parameter style, this may
    *              contain named placeholders (":name") or native DB placeholders.
    * @param style Defines how parameters are written in the SQL string:
    *              - ParameterStyle::named : parse and expand ":name"
    *                placeholders into DB-native form.
    *              - ParameterStyle::native : assume the query already uses
    *                DB-native placeholders and leave the SQL unchanged.
    *
    * By default, the constructor assumes ParameterStyle::named.
    */
   SqlQuery(SqlConnection& conn, const std::string& sql, ParameterStyle style=ParameterStyle::named)
      : _conn(conn)
      , _cmdImpl(&conn.connImpl())
      , _sqlQuery(sql)
      , _parameterStyle(style)
   {
      _cmdImpl.notificationHandler
         = std::bind(&SqlQuery::command_onNotification, this, std::placeholders::_1);
   }

   ~SqlQuery()
   {
      _cmdImpl.notificationHandler = nullptr;
   }

   /// Prepare for one-shot query execution
   bool query(const std::string& sql, const SqlParameterCollection& parameters={})
   {
      _sqlQuery          = sql;
      
      if ( !parameters.empty() )
         _parameters     = parameters;

      auto finalSqlQuery = expandNamedParams(_sqlQuery, _parameterStyle, _parameters, _conn.backendType());
      _prepared          = _cmdImpl.query(finalSqlQuery, _parameters);

      return _prepared;
   }

   bool prepare(const std::string& sql)
   {
      _sqlQuery          = sql;
      auto finalSqlQuery = expandNamedParams(_sqlQuery, _parameterStyle, _parameters, _conn.backendType());
      _prepared          = _cmdImpl.prepare(finalSqlQuery);

      return _prepared;
   }

   void reset()
   {
      _parameters = {};
      _cmdImpl.reset();
   }

   void close()
   {
      _cmdImpl.close();
      _prepared = false;
   }

   // Add parameter in the order they appear in the query
   void addParam(
      const std::string& name,
      DataType           type,
      VariantType        value,
      uint64_t           size = 0,
      short              decimalDigits = 0,
      bool               isUnsigned = false,
      ParameterDirection direction = ParameterDirection::input)
   {
      _parameters.push_back(std::make_shared<SqlParameter>(name, type, value, size, decimalDigits, isUnsigned, direction));
   }

   // Add parameter in the order they appear in the query
   template <typename T>
   void addParam(
      const std::string& name,
      DataType           type,
      T                  value,
      uint64_t           size = 0,
      short              decimalDigits = 0,
      bool               isUnsigned = false,
      ParameterDirection direction = ParameterDirection::input)
   {
      _parameters.push_back(std::make_shared<SqlParameter>(
         name, type, std::forward<T>(value), size, decimalDigits, isUnsigned, direction));
   }


   /** 
    * \brief Execute sql command or stored procedure that does not return rows.
    * \details 
    * On successfull execution, returns affected rows ( >= 0)  (INSERT/UPDATE/DELETE command)
    * On error, SqlException thrown
    * On bad connection, retuns -1
    * Result from sql command returning row(s) is ignored and affected rows is 0.
    */
   int execute()
   {
      std::string errorMessage;
      if (!ensurePreparedAndBound(errorMessage))
         throw SqlException(tbsfmt::format("SqlQuery failed. {}", errorMessage), "SqlQuery"); 

      return _cmdImpl.execute();
   }


   /** 
    * \brief Execute sql command or stored procedure that does not return rows.
    * \details 
    * On successfull execution, returns true
    * On error, SqlException thrown
    * On bad connection, retuns ?
    * Result from sql command returning row(s) is ignored and affected rows is 0.
    */
   bool executeVoid()
   {
      std::string errorMessage;
      if (!ensurePreparedAndBound(errorMessage))
         throw SqlException(tbsfmt::format("SqlQuery failed. {}", errorMessage), "SqlQuery"); 

      return _cmdImpl.execute() >= 0;
   }


   /** 
    * \brief Execute query, and retrieve single string(may empty) result.
    * \details On successfull execution, return string value
    * On error, SqlException thrown
    * On bad connection status, returns an empty string
    */
   std::string executeScalar()
   {
      std::string errorMessage;
      if (!ensurePreparedAndBound(errorMessage))
         throw SqlException(tbsfmt::format("SqlQuery failed. {}", errorMessage), "SqlQuery"); 

      return _cmdImpl.executeScalar();
   }

   /** 
    * \brief Execute query, and retrieve the sql result set.
    * \details On successfull execution, return std::shared_ptr<SqlResult>
    * On error, SqlException thrown
    */
   std::shared_ptr<SqlResult> executeResult(bool cacheData=true, bool openTable=false)
   {
      std::string errorMessage;
      if (!ensurePreparedAndBound(errorMessage))
         throw SqlException(tbsfmt::format("SqlQuery failed. {}", errorMessage), "SqlQuery"); 

      std::shared_ptr<SqlResult> result = std::make_shared<SqlResult>(_conn);
      result->setOptionCacheData(cacheData);
      result->setOptionOpenTable(openTable);

      const bool ok = result->runPreparedQuery(*this);
      if (!ok)
         return nullptr;

      return result;
   }


   SqlParameterCollection& parameters()
   {
      return _parameters;
   }

   int affectedRows() { return static_cast<int>(_cmdImpl.affectedRows()); }

   CommandImpl& queryImpl()
   {
      return _cmdImpl;
   }



protected:

   bool ensurePreparedAndBound(std::string& outErrMessage)
   {
      try
      {
         if (_sqlQuery.empty())
         {
            outErrMessage = "empty sql query";
            return false;
         }

         if (!_prepared)
         {
            auto finalSqlQuery = expandNamedParams(_sqlQuery, _parameterStyle, _parameters, _conn.backendType());
            _prepared = _cmdImpl.prepare(finalSqlQuery);
         }

         return _cmdImpl.bind(_parameters);
      }
      catch(const SqlException& ex)
      {
         outErrMessage = ex.appError.message;
         _logger.error("[sql] prepare failed. {}", ex.appError.message );
      }
      catch(const std::exception& ex)
      {
         outErrMessage = ex.what();
         _logger.error("[sql] prepare failed. {}", ex.what() );
      }
      catch(...)
      {
         _logger.error("[sql] prepare failed.");
      }

      return false;
   }


   /// Handler for notification from Implementation class.
   void command_onNotification(const NotifyEventArgs& arg)
   {
      if (arg.type == NotificationType::trace)
         _logger.trace(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::debug)
         _logger.debug(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::info)
         _logger.info(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::warning)
         _logger.warn(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));

      if (arg.type == NotificationType::error)
         _logger.error(tbsfmt::format("[sql] [{}] {}", arg.source, arg.message));
   }

   SqlConnection& _conn;
   CommandImpl    _cmdImpl;

   /// Implementation logger class.
   LoggerImpl     _logger;

   // original sql query
   std::string             _sqlQuery;
   ParameterStyle          _parameterStyle;
   SqlParameterCollection  _parameters;
   bool                    _prepared = false;
};

} // namespace sql
} // namespace tbs