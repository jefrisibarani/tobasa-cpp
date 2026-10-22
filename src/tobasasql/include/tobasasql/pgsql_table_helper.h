#pragma once

#include "tobasasql/exp_imp.h"

#include <string>
#include "tobasasql/pgsql_result.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

//! Forward declaration
class ColumnInfo;

/** 
 * \ingroup SQL
 * \brief PostgreSQL Table helper.
 */
class TBS_SQL_API PgsqlTableHelper
{
public:
   bool isView(PgsqlConnection& connection );
   bool setupColumnInfo(PgsqlResult& tableResult, ColumnInfo* columnInfo);
   void setTableName(const std::string& tableName);
   std::string tableName() const;

private:
   std::string _tableName;
   bool _readOnly;
};

/** @}*/

} // namespace sql
} // namespace tbs
