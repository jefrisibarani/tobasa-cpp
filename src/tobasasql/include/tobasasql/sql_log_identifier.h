#pragma once

#include "tobasasql/sql_connection.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief SqlApplyLogId
 * \tparam SqlDriverType
 */
template < typename SqlDriverType >
class SqlApplyLogId
{
public:
   using SqlConnection = sql::SqlConnection<SqlDriverType>;

   SqlApplyLogId(SqlConnection &connection, const std::string& newIdentifier)
      : _conn(connection)
   {
      _currentId = _conn.logId();
      _conn.setLogId(newIdentifier);
   }

   ~SqlApplyLogId()
   {
      _conn.setLogId(_currentId);
   }

private:
   SqlConnection& _conn;
   std::string _currentId;
};

} // namespace sql
} // namespace tbs