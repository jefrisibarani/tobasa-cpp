#include "tobasasql/column_info.h"
#include "tobasasql/sqlite_table_helper.h"

namespace tbs {
namespace sql {

bool SqliteTableHelper::isView(SqliteConnection& connection)
{
   bool checkForTable = true;
   bool isTable       = connection.tableOrViewExists( tableName() , checkForTable);
   _readOnly          = ! isTable;
   
   return _readOnly;
}

bool SqliteTableHelper::setupColumnInfo(SqliteResult& tableResult, ColumnInfo* columnInfo)
{
   SqlApplyLogInternal applyLogRule(tableResult.connection());

   long tableColumnCount = tableResult.totalColumns();
   bool hasPrimaryKey{false};

   for (int i = 0; i < tableColumnCount; i++)
   {
      ColumnInfo& colInfo     = columnInfo[i];
      std::string columnName  = tableResult.columnName(i);

      colInfo.setName(            columnName );
      colInfo.setAutoIncrement(   tableResult.columnIsAutoIncrement(i) );
      colInfo.setNativeType(      tableResult.columnNativeType(i) );
      colInfo.setTypeClass(       tableResult.columnTypeClass(i) );
      colInfo.setTypeLength(      tableResult.columnDefinedSize(i));
      colInfo.setNativeTypeStr(   tableResult.columnNativeTypeStr(i) );
      colInfo.setDisplayTypeName( tableResult.columnNativeTypeStr(i) );
      colInfo.setDataType(        tableResult.columnDataType(i) );

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

   return true;
}

void SqliteTableHelper::setTableName(const std::string& tableName)
{
   _tableName = tableName;
}

std::string SqliteTableHelper::tableName() const
{
   return _tableName;
}


} // namespace sql
} // namespace tbs