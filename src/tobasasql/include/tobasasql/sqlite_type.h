#pragma once

#ifdef TOBASA_SQL_USE_SQLITE3_MC
#include <sqlite3mc.h>
#else
#include <sqlite3.h>
#endif

namespace tbs {
namespace sql {

/**
 * Sqlite data types
 */
enum class SqliteType
{
   null     = SQLITE_NULL,
   integer  = SQLITE_INTEGER,
   real     = SQLITE_FLOAT,
   text     = SQLITE3_TEXT,
   blob     = SQLITE_BLOB,
   unknown  = -1
};

} // namespace sql
} // namepsace tbs