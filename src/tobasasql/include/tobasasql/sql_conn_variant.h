#pragma once

#include "tobasasql/sql_defines.h"
#include "tobasasql/sql_connection.h"
#include "tobasasql/sql_driver.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

      /// SQL Connection variant and SQL Connection shared pointer variant
#if defined(TOBASA_SQL_USE_ONLY_SQLITE)
      using SqlConnectionVariant = Variant<SqlConnection<SqliteDriver>>;
      using SqlConnectionPtrVariant = Variant<SqlConnectionPtr<SqliteDriver>>;
#else
      using SqlConnectionVariant = Variant<
   #if defined(TOBASA_SQL_USE_PGSQL)
            SqlConnection<PgsqlDriver>,
   #endif
   #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
            SqlConnection<AdodbDriver>,
   #endif
   #if defined(TOBASA_SQL_USE_ODBC)
            SqlConnection<OdbcDriver>,
   #endif
   #if defined(TOBASA_SQL_USE_MYSQL)
            SqlConnection<MysqlDriver> ,
   #endif
            SqlConnection<SqliteDriver>
         >;

      using SqlConnectionPtrVariant = Variant<
   #if defined(TOBASA_SQL_USE_PGSQL)
            SqlConnectionPtr<PgsqlDriver> ,
   #endif
   #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
            SqlConnectionPtr<AdodbDriver> ,
   #endif
   #if defined(TOBASA_SQL_USE_ODBC)
            SqlConnectionPtr<OdbcDriver> ,
   #endif
   #if defined(TOBASA_SQL_USE_MYSQL)
            SqlConnectionPtr<MysqlDriver> ,
   #endif
            SqlConnectionPtr<SqliteDriver>
         >;

#endif // defined(TOBASA_SQL_USE_ONLY_SQLITE)

/** @}*/

} // namespace sql
} // namespace tbs