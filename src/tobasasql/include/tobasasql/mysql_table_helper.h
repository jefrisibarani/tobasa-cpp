#pragma once

#include "tobasasql/mysql_result.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

class ColumnInfo;

/** 
 * \brief MySql Table helper.
 * \note Experimental
 */
class MysqlTableHelper
{
public:
   bool isView(MysqlConnection& connection);
   bool setupColumnInfo(MysqlResult& tableResult, ColumnInfo* columnInfo);
   void setTableName(const std::string& tableName);
   std::string tableName() const;

private:
   std::string _tableName;
   bool _readOnly;
};

/** @}*/

} // namespace sql
} // namespace tbs