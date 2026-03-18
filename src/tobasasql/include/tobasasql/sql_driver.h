#pragma once

#include "tobasasql/sql_defines.h"

#include <tobasa/variant.h>
#include <tobasa/navigator.h>
#include <tobasa/logger_null.h>
#include <tobasa/logger_tobasa.h>

#include "tobasasql/sql_parameter.h"

#if defined(TOBASA_SQL_USE_PGSQL)
   #include "tobasasql/pgsql_helper.h"
   #include "tobasasql/pgsql_table_helper.h"
#endif

#if defined(TOBASA_SQL_USE_SQLITE)
   #include "tobasasql/sqlite_helper.h"
   #include "tobasasql/sqlite_table_helper.h"
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   #include "tobasasql/adodb_helper.h"
   #include "tobasasql/adodb_table_helper.h"
   #include "tobasasql/com_variant_helper.h"
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   #include "tobasasql/odbc_helper.h"
   #include "tobasasql/odbc_table_helper.h"
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   #include "tobasasql/mysql_helper.h"
   #include "tobasasql/mysql_table_helper.h"
#endif

namespace tbs {
namespace sql {

/** \defgroup SQL SQL driver module for Tobasa
 * @{
 */

/**
 * \brief SQL Driver template class.
 */
template <
   typename ConnectionImplemented,
   typename ResultImplemented,
   typename HelperImplemented,
   typename TableHelperImplemented,
   typename LoggerImplemented,
   typename VariantTypeImplemented = tbs::DefaultVariantType >
struct SqlDriver
{
   using ConnectionImpl             = ConnectionImplemented;
   using ResultImpl                 = ResultImplemented;
   using HelperImpl                 = HelperImplemented;
   using TableHelperImpl            = TableHelperImplemented;
   using Logger                     = LoggerImplemented;

   using VariantType                = VariantTypeImplemented;
   using VariantHelper              = tbs::VariantHelper<VariantType>;

   using SqlParameter               = Parameter<VariantType>;
   using SqlParameterCollection     = std::vector<std::shared_ptr<SqlParameter> >;
   using SqlParameterCollectionPtr  = std::shared_ptr< SqlParameterCollection >;

   using TableNavigator             = tbs::NavigatorBasic;
   using ResultNavigator            = tbs::NavigatorBasic;
};

#if defined(TOBASA_SQL_USE_PGSQL)
   using PgsqlDriverDefault   = SqlDriver<PgsqlConnection,  PgsqlResult,  PgsqlHelper,  PgsqlTableHelper,  log::NullLogger>;
   /// Postgresql driver with TobasaLogger.
   using PgsqlDriver = SqlDriver<PgsqlConnection, PgsqlResult, PgsqlHelper, PgsqlTableHelper, log::TobasaLogger>;
#endif

#if defined(TOBASA_SQL_USE_SQLITE)
   using SqliteDriverDefault  = SqlDriver<SqliteConnection, SqliteResult, SqliteHelper, SqliteTableHelper, log::NullLogger>;
   /// SQLite driver with TobasaLogger.
   using SqliteDriver = SqlDriver<SqliteConnection, SqliteResult, SqliteHelper, SqliteTableHelper, log::TobasaLogger>;
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   using AdodbDriverDefault   = SqlDriver<AdodbConnection,  AdodbResult,  AdodbHelper,  AdodbTableHelper,  log::NullLogger>;

   /// ADODB driver with TobasaLogger.
   class AdodbDriver
      : public SqlDriver<AdodbConnection, AdodbResult, AdodbHelper, AdodbTableHelper, log::TobasaLogger, ComVariantType>
   {
   public:
      using VariantHelper = ComVariantHelper;
      using ResultNavigator = AdoResultNavigator<AdodbResult>;
   };
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   using OdbcDriverDefault    = SqlDriver<OdbcConnection,   OdbcResult,   OdbcHelper,   OdbcTableHelper,   log::NullLogger>;
   /// ODBC driver with TobasaLogger.
   using OdbcDriver = SqlDriver<OdbcConnection, OdbcResult, OdbcHelper, OdbcTableHelper, log::TobasaLogger>;
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   using MysqlDriverDefault   = SqlDriver<MysqlConnection,  MysqlResult,  MysqlHelper,  MysqlTableHelper,  log::NullLogger>;

   /// MySql driver with TobasaLogger.
   class MysqlDriver
      : public SqlDriver<MysqlConnection, MysqlResult, MysqlHelper, MysqlTableHelper, log::TobasaLogger, MysqlVariantType>
   {
   public:
      using VariantHelper = MysqlVariantHelper;
   };

#endif


/**
 * \brief SqlDriverVariant
 */
#if defined(TOBASA_SQL_USE_ONLY_SQLITE)
      using SqlDriverVariant = Variant<SqliteDriver>;
#else
      using SqlDriverVariant = Variant<
   #if defined(TOBASA_SQL_USE_PGSQL)
            PgsqlDriver ,
   #endif
   #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
            AdodbDriver ,
   #endif
   #if defined(TOBASA_SQL_USE_ODBC)
            OdbcDriver ,
   #endif
   #if defined(TOBASA_SQL_USE_MYSQL)
            MysqlDriver ,
   #endif
            SqliteDriver
         >;
#endif // defined(TOBASA_SQL_USE_ONLY_SQLITE)

/** @}*/

} // namespace sql
} // namespace tbs