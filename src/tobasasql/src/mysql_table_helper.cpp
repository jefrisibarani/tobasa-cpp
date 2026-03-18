#include "tobasasql/column_info.h"
#include "tobasasql/mysql_table_helper.h"

namespace tbs {
namespace sql {

bool MysqlTableHelper::isView(MysqlConnection& connection)
{
   SqlApplyLogInternal applyLogRule(&connection);

   std::string sql = tbsfmt::format("SELECT table_name FROM information_schema.views WHERE table_name='{}'", _tableName);
   if (connection.executeScalar(sql) == _tableName) {
      return true;
   }

   return false;
}

bool MysqlTableHelper::setupColumnInfo(MysqlResult& tableResult, ColumnInfo* columnInfo)
{
   SqlApplyLogInternal applyLogRule(tableResult.connection());

   long tableColumnCount = tableResult.totalColumns();
   bool hasPrimaryKey{false};

   for (int i = 0; i < tableColumnCount; i++)
   {
      ColumnInfo& colInfo    = columnInfo[i];
      std::string columnName = tableResult.columnName(i);

      colInfo.setName(            columnName );
      colInfo.setAutoIncrement(   tableResult.columnIsAutoIncrement(i) );
      colInfo.setNativeType(      tableResult.columnNativeType(i) );
      colInfo.setTypeClass(       tableResult.columnTypeClass(i) );
      colInfo.setTypeLength(      tableResult.columnDefinedSize(i));
      colInfo.setNativeTypeStr(   tableResult.columnNativeTypeStr(i) );
      colInfo.setDisplayTypeName( tableResult.columnNativeTypeStr(i) );
      colInfo.setDataType(        tableResult.columnDataType(i) );
      colInfo.setPrecision(       tableResult.columnPrecision(i) );
      colInfo.setNumericScale(    tableResult.columnNumericScale(i) );

      TypeClass typeClass = tableResult.columnTypeClass(i);

      if (typeClass == TypeClass::numeric)
      {
         colInfo.setNumeric();
         colInfo.setReadOnly(false);
      }
      else if (typeClass == TypeClass::boolean)
      {
         colInfo.setNonNumeric();
         colInfo.setReadOnly(false);
      }
      else if (typeClass == TypeClass::blob)
      {
         colInfo.setNonNumeric();
         colInfo.setReadOnly(true);
      }
      else
      {
         colInfo.setNonNumeric();
         colInfo.setReadOnly(false);
         colInfo.setNeedResize(true);
      }

      // Setup Primary key information , for UPDATE AND INSERT AND DELETE
      if (tableResult.columnIsPrimaryKey(i))
      {
         colInfo.setPrimaryKey(true);
         colInfo.setReadOnly(false);

         std::string newNativeTypeStr = "[PK]";
         newNativeTypeStr += util::toLower( tableResult.columnNativeTypeStr(i) );

         colInfo.setDisplayTypeName(newNativeTypeStr);

         hasPrimaryKey = true;
      }

      // Auto increment field always read only
      if (colInfo.isAutoIncrement()) {
         colInfo.setReadOnly(true);
      }

      colInfo.setNeedResize(true);
   }

   // Finally, for table without Primary Key, make all columns read only
   for (int i = 0; i < tableColumnCount; i++)
   {
      ColumnInfo& colInfo = columnInfo[i];

      if (!hasPrimaryKey)
         colInfo.setReadOnly(true);
   }

   return true;;
}

void MysqlTableHelper::setTableName(const std::string& tableName)
{
   _tableName = tableName;
}

std::string MysqlTableHelper::tableName() const
{
   return _tableName;
}


} // namespace sql
} // namespace tbs