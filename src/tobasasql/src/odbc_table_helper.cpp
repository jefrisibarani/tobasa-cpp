#include "tobasa/common.h"
#include "tobasa/util.h"
#include "tobasasql/odbc_table_helper.h"

namespace tbs {
namespace sql {

bool OdbcTableHelper::isView(OdbcConnection& connection)
{
   bool checkForTable = true;
   bool isTable       = connection.tableOrViewExists( tableName() , checkForTable);
   _readOnly          = ! isTable;

   return _readOnly;
}

bool OdbcTableHelper::setupColumnInfo(OdbcResult& tableResult, ColumnInfo* columnInfo)
{
   SqlApplyLogInternal applyLogRule(tableResult.connection());

   long tableColumnCount = tableResult.totalColumns();

   // get auto primary key columns
   std::vector<std::string> primaryKeyColumns;
   tableResult.connection()->getPrimaryKeyColumns(primaryKeyColumns, _tableName);

   bool hasPrimaryKey = primaryKeyColumns.size() > 0;

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
      if (util::findPositionInVector(primaryKeyColumns, columnName) != tbs::NOT_FOUND)
      {
         colInfo.setPrimaryKey(true);
         colInfo.setReadOnly(false);

         std::string newNativeTypeStr = "[PK]";
         newNativeTypeStr += util::toLower( tableResult.columnNativeTypeStr(i) );

         colInfo.setDisplayTypeName(newNativeTypeStr);
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

void OdbcTableHelper::setTableName(const std::string& tableName)
{
   _tableName = tableName;
}

std::string OdbcTableHelper::tableName() const
{
   return _tableName;
}


} // namespace sql
} // namespace tbs