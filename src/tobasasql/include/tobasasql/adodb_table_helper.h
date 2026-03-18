#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include "tobasasql/column_info.h"
#include "tobasasql/adodb_result.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ADO Table helper class.
 */
class AdodbTableHelper
{
public:
   bool isView(AdodbConnection& connection);
   bool setupColumnInfo(AdodbResult& tableResult, ColumnInfo* columnInfo);
   void setTableName(const std::string& tableName);
   std::string tableName() const;

private:
   std::string _tableName;
   bool _readOnly;
};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)