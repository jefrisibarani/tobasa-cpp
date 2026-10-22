#pragma once

#include "tobasasql/exp_imp.h"

#include <string>
#include "tobasasql/column_info.h"
#include "tobasasql/odbc_result.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ODBC Table helper.
 * \note Tested only with MS SQL Server   
*/
class TBS_SQL_API OdbcTableHelper
{
public:
   bool isView(OdbcConnection& connection);
   bool setupColumnInfo(OdbcResult& tableResult, ColumnInfo* columnInfo);
   void setTableName(const std::string& tableName);
   std::string tableName() const;

private:
   std::string _tableName;
   bool _readOnly;
};

} // namespace sql
} // namespace tbs