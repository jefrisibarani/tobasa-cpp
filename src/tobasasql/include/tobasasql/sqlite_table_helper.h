#pragma once

#include <string>
#include "tobasasql/sqlite_result.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/// Forward declaration
class ColumnInfo;

/**
 * \brief SQLite Table helper class.
 */
class SqliteTableHelper
{
public:
   bool isView(SqliteConnection& connection);
   bool setupColumnInfo(SqliteResult& tableResult, ColumnInfo* columnInfo);
   void setTableName(const std::string& tableName);
   std::string tableName() const;

private:
   std::string _tableName;
   bool _readOnly;
};

/** @}*/

} // namespace sql
} // namespace tbs