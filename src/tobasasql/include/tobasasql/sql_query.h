#pragma once

#include <tobasa/self_counter.h>
#include "tobasasql/sql_connection.h"
#include "tobasasql/util.h"

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
   using LoggerImpl        = typename SqlDriverType::Logger;
   using SqlResult         = sql::SqlResult<SqlDriverType>;
   using SqlConnection     = sql::SqlConnection<SqlDriverType>;
   using VariantType       = typename SqlDriverType::VariantType;
   using VectorVariant     = std::vector<VariantType>;
   using VariantHelper     = typename SqlDriverType::VariantHelper;

   /// Alias SqlParameter.
   using SqlParameter      = typename SqlDriverType::SqlParameter;
   /// Alias SqlParameterCollection.
   using SqlParameterCollection = typename SqlDriverType::SqlParameterCollection;

   /**
    * @brief Construct a new SqlQuery object.
    * @param Active database connection used for executing the query.
    */
   SqlQuery(SqlConnection& conn)
      : _conn(conn)
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
    *              - ParameterStyle::Native      : assume the query already uses
    *                DB-native placeholders and leave the SQL unchanged.
    *
    * By default, the constructor assumes ParameterStyle::named.
    */
   SqlQuery(SqlConnection& conn, const std::string& sql, ParameterStyle style=ParameterStyle::named)
      : _conn(conn)
      , _sqlQuery(sql)
      , _parameterStyle(style)
   {
   }

   ~SqlQuery() = default;

   void setSql(const std::string& sql)
   {
      _sqlQuery= sql;
   }

   void reset(const std::string& sql, ParameterStyle style=ParameterStyle::named)
   {
      _parameters = {};
      _sqlQuery   = sql;
      _parameterStyle = style;
   }

   // Add parameter in the order they appear in the query
   void addParam(
      const std::string& name,
      DataType           type,
      VariantType        value,
      long               size = 0,
      ParameterDirection direction = ParameterDirection::input,
      short              decimalDigits = 0)
   {
      _parameters.push_back(std::make_shared<SqlParameter>(name, type, value, size, direction, decimalDigits));
   }

   // template <typename T,
   //          typename = std::enable_if_t<std::is_constructible_v<VariantType, T>>>
   template <typename T>
   void addParam(
      const std::string& name,
      DataType           type,
      T&&                value,
      long               size = 0,
      ParameterDirection direction = ParameterDirection::input,
      short              decimalDigits = 0)
   {
      _parameters.push_back(std::make_shared<SqlParameter>(
         name, type, std::forward<T>(value), size, direction, decimalDigits));
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
      return _conn.execute(_sqlQuery, _parameters, _parameterStyle);
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
      return _conn.executeVoid(_sqlQuery, _parameters, _parameterStyle);
   }

   /** 
    * \brief Execute query, and retrieve single string(may empty) result.
    * \details On successfull execution, return string value
    * On error, SqlException thrown
    * On bad connection status, returns an empty string
    */
   std::string executeScalar()
   {
      return _conn.executeScalar(_sqlQuery, _parameters, _parameterStyle);
   }


   std::shared_ptr<SqlResult> executeResult(bool cacheData=true, bool openTable=false)
   {
      std::shared_ptr<SqlResult> result = std::make_shared<SqlResult>(_conn);
      result->setOptionCacheData(cacheData);
      result->setOptionOpenTable(openTable);
      result->runQuery(_sqlQuery, _parameters, _parameterStyle);

      return result;
   }

   SqlParameterCollection& parameters()
   {
      return _parameters;
   }

   // -------------------------------------------------------
   // TODO_JEFRI:
   void setBool(uint16_t paramPos, bool value) {}
   void setBool(uint16_t paramPos, uint8_t value) {}
   
   void setInt8(uint16_t paramPos, int8_t value) {}
   void setInt16(uint16_t paramPos, int16_t value) {}
   void setInt32(uint16_t paramPos, int32_t value) {}
   void setInt64(uint16_t paramPos, int64_t value) {}
   void setInt(uint16_t paramPos, int value) {}
   
   void setUInt8(uint16_t paramPos, uint8_t value) {}
   void setUInt16(uint16_t paramPos, uint16_t value) {}
   void setUInt32(uint16_t paramPos, uint32_t value) {}
   void setUInt64(uint16_t paramPos, uint64_t value) {}
   void setInt(uint16_t paramPos, unsigned int value) {}
   
   void setFloat(uint16_t paramPos, float value) {}
   void setDouble(uint16_t paramPos, double value) {}
   void setString(uint16_t paramPos, const std::string& value) {}
   void setWstring(uint16_t paramPos, const std::wstring& value) {}
   void setBytes(uint16_t paramPos, const std::vector<uint8_t>& value) {}
   void setChars(uint16_t paramPos, const std::vector<char>& value) {}
   // -------------------------------------------------------

protected:

   SqlConnection& _conn;
   std::string _sqlQuery;
   ParameterStyle _parameterStyle;
   SqlParameterCollection  _parameters;

   /*
   void expandNamedParams() 
   {
      std::string result;
      result.reserve(sql.size() + 16);

      auto isIdentChar = [](char c) noexcept {
         return (c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c == '_');
      };

      bool inSingleQuote = false;
      bool inDoubleQuote = false;
      bool inEscape = false;

      int position = 1;

      for (size_t i = 0; i < sql.size();) 
      {
         char c = sql[i];

         // Handle string literals
         if (c == '\'' && !inDoubleQuote) 
         {
            result.push_back(c);
            inSingleQuote = !inSingleQuote; // toggle
            ++i;
            continue;
         }

         if (c == '"' && !inSingleQuote) 
         {
            result.push_back(c);
            inDoubleQuote = !inDoubleQuote;
            ++i;
            continue;
         }

         if (inSingleQuote || inDoubleQuote) 
         {
            // inside literal → just copy
            result.push_back(c);
            ++i;
            continue;
         }

         // Handle "::" Postgres casts
         if (c == ':' && i + 1 < sql.size() && sql[i + 1] == ':') 
         {
            result.append("::");
            i += 2;
            continue;
         }

         // Handle named parameters ":param"
         if (c == ':' && i + 1 < sql.size() && isIdentChar(sql[i + 1])) 
         {
            size_t j = i + 1;
            while (j < sql.size() && isIdentChar(sql[j])) ++j;

            std::string_view paramName(sql.data() + i + 1, j - (i + 1));
            _paramOrder.emplace_back(paramName);

            if (dbmsName == "PGSQL")
               result.append("$").append(std::to_string(position));
            else
               result.push_back('?');

            ++position;
            i = j;
            continue;
         }

         // Default: copy char
         result.push_back(c);
         ++i;
      }

      return result;
   }
   */
};

} // namespace sql
} // namespace tbs