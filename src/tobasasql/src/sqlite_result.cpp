#include <string>
#include "tobasasql/sqlite_util.h"
#include "tobasasql/sqlite_result.h"

namespace tbs {
namespace sql {

SqliteResult::SqliteResult(SqliteConnection* pconn)
   : ResultCommon()
{
   _pStatement     = nullptr;
   _pConn          = pconn;
   notifierSource  = "SqliteResult";
   _navigator.init( std::bind(&SqliteResult::totalRows, this) );
}

SqliteResult::~SqliteResult()
{
   if (_pStatement != nullptr)
      _pStatement = nullptr;
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string SqliteResult::name() const
{
   return "Sqlite Result";
}

bool SqliteResult::runQuery(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (_pConn == nullptr)
      return false;

   if (_pConn->status() != ConnectionStatus::ok)
      return false;

   if (_optionOpenTable)
      _qryStr = "SELECT * FROM " + sql;
   else
      _qryStr = sql;

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("runQuery: {}", _qryStr));

   _pStatement = _pConn->createStatement(_qryStr, parameters);
   _nColumns   = sqlite3_column_count(_pStatement);

   if (_nColumns > 0)
   {
      // we have total columns, now set up columns info
      setupColumnProperties();
      // OK, we have valid result set, now init metadatas
      setupColumnMetaData();
   }

   // retrieve all rows
   int retCode       = 0;
   int rowsRetrieved = 0;

   while (true)
   {
      retCode = sqlite3_step(_pStatement);
      _affectedRows += sqlite3_changes(_pConn->nativeConn());

      if (retCode == SQLITE_DONE)
         break;
      else if (retCode == SQLITE_ROW)
      {
         VectorVariant recordVariant;

         if (_nColumns > 0) {
            recordVariant.reserve(_nColumns);
         }

         for (int i = 0; i < _nColumns; i++)
         {
            int colType = sqlite3_column_type(_pStatement, i);
            SqliteType sqliteType = (SqliteType)colType;
            switch (sqliteType)
            {
               case SqliteType::null:
               {
                  recordVariant.emplace_back( std::monostate{} );
                  break;
               }
               case SqliteType::integer:
               {
                  int64_t value = static_cast<int64_t>(sqlite3_column_int64(_pStatement, i));
                  recordVariant.emplace_back(value);
                  break;
               }
               case SqliteType::real:
               {
                  double value = sqlite3_column_double(_pStatement, i);
                  recordVariant.emplace_back(value);
                  break;
               }
               case SqliteType::text:
               {
                  std::string value = (const char*)sqlite3_column_text(_pStatement, i);
                  recordVariant.emplace_back(value);
                  break;
               }
               case SqliteType::blob:
               {
                  // BLOB. The value is a blob of data, stored exactly as it was input.
                  // SQLite store blob as binary data, so we need to convert first to hex string
                  int blobSize = sqlite3_column_bytes(_pStatement, i);
                  crypt::byte_t* raw = (crypt::byte_t*)sqlite3_column_blob(_pStatement, i);
                  std::string result;
                  for (int i = 0; i < blobSize; ++i)
                  {
                     crypt::byte_t b = raw[i];
                     result += crypt::decToHex(b);
                  }
                  recordVariant.emplace_back(result);
                  break;
               }
               default:
               {
                  std::string value = (const char*)sqlite3_column_text(_pStatement, i);
                  recordVariant.emplace_back(value);
                  break;
               }
            }
         }

         _dataVariant.emplace_back(recordVariant);
         rowsRetrieved++;
      }
      else
      {
         onNotifyError(_pConn->logId() + _pConn->lastBackendError());
         sqlite3_finalize(_pStatement);
         _pStatement = nullptr;
         throw tbs::SqlException(tbsfmt::format("runQuery, {}",_pConn->lastBackendError()), "SqliteResult");
      }
   }

   _nRows = rowsRetrieved;

   if (retCode == SQLITE_DONE)
   {
      sqlite3_finalize(_pStatement);
      _pStatement = nullptr;

      if (_pConn->logExecuteStatus())
         onNotifyTrace(_pConn->logId() + tbsfmt::format("SQL command executed successfully, row: {}, columns: {} ", _nRows, _nColumns));

      if (_nRows > 0)
         _resultStatus = ResultStatus::tuplesOk;
      else
         _resultStatus = ResultStatus::commandOk;

      _navigator.moveFirst();
      return true;
   }

   return false;
}

void SqliteResult::connection(SqliteConnection* conn)
{
   _pConn = conn;
}

SqliteConnection* SqliteResult::connection() const { return _pConn; }

NavigatorBasic& SqliteResult::navigator() { return _navigator; }

// -------------------------------------------------------
// Overridden methods from base class : ResultCommon
// -------------------------------------------------------

TypeClass SqliteResult::columnTypeClass(const int columnIndex) const
{
   std::string colDeclaredType = columnNativeFullTypeStr(columnIndex);
   return typeClassFromSqliteDeclaredType(colDeclaredType);
}

SqliteResult::VariantType SqliteResult::getVariantValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   VariantType value = _dataVariant[row][columnIndex];
   return value;
}

SqliteResult::VariantType SqliteResult::getVariantValue(const std::string& columnName) const
{
   return getVariantValue(columnNumber(columnName));
}

std::string SqliteResult::getStringValue(const int columnIndex) const
{
   long row = _navigator.position();

   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   auto& value = _dataVariant[row][columnIndex];
   return VariantHelper<>::toString(value);
}

std::string SqliteResult::getStringValue(const std::string& columnName) const
{
   return getStringValue(columnNumber(columnName));
}

bool SqliteResult::isNullField(const int columnIndex) const
{
   // TODO_JEFRI : do with better way
   return getStringValue(columnIndex) == sql::NULLSTR;
}

// -------------------------------------------------------
// Specific to SqliteResult
// -------------------------------------------------------

void SqliteResult::setupColumnProperties()
{
   if (_nColumns <= 0)
      return;

   _columnInfoCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _columnInfoCollection.emplace_back(ColumnInfo());
   }

   //SqlApplyLogInternal applyLogRule(_pConn);

   try
   {
      for (int i = 0; i < _nColumns; i++)
      {
         // save column name
         const char* colname = sqlite3_column_name(_pStatement, i);
         _columnInfoCollection[i].name = std::string(colname);

         // save column defined size
         _columnInfoCollection[i].definedSize = FIELD_SIZE_UNKNOWN;

         std::string declaredTypeStr = columnDeclaredType(_pStatement, i);
         SqliteType sqliteType = sql::sqliteTypeFromDeclaredType(declaredTypeStr);

         // save column native type as string
         _columnInfoCollection[i].nativeTypeStr = sql::sqliteTypeToString(sqliteType);

         // save column native declared type as string
         _columnInfoCollection[i].nativeFullTypeStr = declaredTypeStr;

         // save column native type
         _columnInfoCollection[i].nativeType = (long)sqliteType;

         // save column data type : sql::DataType
         _columnInfoCollection[i].dataType = sql::sqliteTypeToDataType(sqliteType);
      }
   }
   catch (const TypeException & ex)
   {
      onNotifyError(_pConn->logId() + ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "SqliteResult");
   }
}

std::string SqliteResult::columnDeclaredType(sqlite3_stmt* stmt, int pos)
{
   // Note: https://www.sqlite.org/c3ref/column_decltype.html
   // The first parameter is a prepared statement. If this statement is a SELECT statement 
   // and the Nth column of the returned result set of that SELECT is a table column 
   // (not an expression or subquery) then the declared type of the table column is returned. 
   // If the Nth column of the result set is an expression or subquery, 
   // then a NULL pointer is returned. The returned string is always UTF-8 encoded.

   const char* colTypStr = sqlite3_column_decltype(stmt, pos);
   if (!colTypStr)
   {
      onNotifyWarning(_pConn->logId() + "Could not get column declared type for column " + std::to_string(pos));
      return "Text";
   }

   std::string declaredtype(colTypStr);
   return sql::sqliteColumnDeclaredType(declaredtype);
}

void SqliteResult::setupColumnMetaData()
{
   // Note: https://www.sqlite.org/c3ref/column_database_name.html
   // The names returned are the original un-aliased names of the database, table, and column.
   // If the Nth column returned by the statement is an expression or subquery and is not a column value,
   // then all of these functions return NULL

   // Check first column, make sure that we are doing this on a table
   const char* tableName = sqlite3_column_table_name(_pStatement, 0);
   if (!tableName) // Not a table, return now
      return;

   // Initialize _metadataCollection 
   _metadataCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _metadataCollection.emplace_back(ColumnMetadata());
   }

   const char* dataType;
   char const** pzDataType = &dataType;
   const char* collSeq;
   char const** pzCollSeq = &collSeq;
   int isNotNull, isPrimaryKey, isAutoInc;

   for (int i = 0; i < _nColumns; i++)
   {
      // check current column table origin
      const char* tableName = sqlite3_column_table_name(_pStatement, i);
      if (tableName)
         _metadataCollection[i].tableName = std::string(tableName);
      else { 
         return ; 
      }  

      // retrieve column name
      const char* columnName = sqlite3_column_name(_pStatement, i);
      if (!columnName) { // Not a column, break now
         break;
      }

      // retrieve column origin name
      const char* columnOriginName = sqlite3_column_origin_name(_pStatement, i);
      if (!columnOriginName) { // Not a column, break now
         break;
      }

      // retrive metadata
      int rc = sqlite3_table_column_metadata(
                  _pConn->nativeConn(),
                  0,
                  tableName,
                  columnOriginName,
                  pzDataType,
                  pzCollSeq,
                  &isNotNull,
                  &isPrimaryKey,
                  &isAutoInc);

      if (rc == SQLITE_OK)
      {
         if (!dataType)
         {
            sqlite3_finalize(_pStatement);
            _pStatement = nullptr;
            throw tbs::SqlException(tbsfmt::format("setupColumnMetaData, invalid column data type for column {}", columnName), "SqliteResult");
         }

         _metadataCollection[i].colIndex      = i;
         _metadataCollection[i].collSeqName   = std::string(collSeq);
         _metadataCollection[i].colName       = std::string(columnName);
         _metadataCollection[i].dataType      = std::string(dataType);
         
         //_metadataCollection[i].isAutoInc     = util::numToBool(isAutoInc);
         //_metadataCollection[i].isNotNull     = util::numToBool(isNotNull);
         //_metadataCollection[i].isPrimaryKey  = util::numToBool(isPrimaryKey);

         _columnInfoCollection[i].autoIncrement = util::numToBool(isAutoInc);
         _columnInfoCollection[i].allowNull   = ! util::numToBool(isNotNull);
         _columnInfoCollection[i].primaryKey  = util::numToBool(isPrimaryKey);
      }
      else
      {
         onNotifyError(_pConn->logId() + _pConn->lastBackendError());
         sqlite3_finalize(_pStatement);
         _pStatement = nullptr;
         throw tbs::SqlException(tbsfmt::format("setupColumnMetaData, {}", _pConn->lastBackendError()), "SqliteResult");
      }
   }
}


} // namespace sql
} // namespace tbs