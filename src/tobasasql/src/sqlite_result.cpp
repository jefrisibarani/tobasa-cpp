#include <string>
#include "tobasasql/sqlite_util.h"
#include "tobasasql/sqlite_result.h"

namespace tbs {
namespace sql {

SqliteResult::SqliteResult(SqliteConnection* pconn)
   : ResultCommon()
{
   _pConn          = pconn;
   notifierSource  = "SqliteResult";
   _navigator.init( std::bind(&SqliteResult::totalRows, this) );
}

SqliteResult::SqliteResult(SqliteResult&& other) noexcept
   : ResultCommon(        std::move(other))
   , _pConn(              other._pConn)
   , _metadataCollection( std::move(other._metadataCollection))
   , _pDataset          ( std::move(other._pDataset))
   , _navigator(          std::move(other._navigator))
{
   other._pConn         = nullptr;
   other._nRows         = 0;
   other._nColumns      = 0;
   other._affectedRows  = 0;
   other._resultStatus  = ResultStatus::unknown;
   other._columnInfoCollection.clear();
   other._qryStr.clear();
}

SqliteResult& SqliteResult::operator=(SqliteResult&& other) noexcept
{
   if (this != &other)
   {
      ResultCommon::operator=(std::move(other));
      _pConn              = other._pConn;
      _metadataCollection = std::move(other._metadataCollection);
      _pDataset           = std::move(other._pDataset);
      _navigator          = std::move(other._navigator);

      other._pConn        = nullptr;
      other._nRows        = 0;
      other._nColumns     = 0;
      other._affectedRows = 0;
      other._resultStatus = ResultStatus::unknown;
      other._columnInfoCollection.clear();
      other._qryStr.clear();
   }
   return *this;
}

SqliteResult::~SqliteResult()
{
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string SqliteResult::name() const
{
   return "Sqlite Result";
}

bool SqliteResult::runQuery(const std::string& sql, 
                           const SqlParameterCollection& parameters,
                           ParameterStyle paramStyle)
{
   if (_pConn == nullptr)
      return false;

   if (_pConn->status() != ConnectionStatus::ok)
      return false;

   if (_optionOpenTable)
      _qryStr = "SELECT * FROM " + sql;
   else 
      _qryStr = sql;

   SqliteCommand command(_pConn);
   if (command.query(_qryStr, parameters))
   {
      auto dataSet  = command.executeResult();
      if (dataSet == nullptr)
         throw SqlException("runQuery, Invalid DataSet pointer", "SqliteResult"); 
      
      _affectedRows = command.affectedRows();
      return getData(std::move(dataSet), command.statement());
   }

   return false;
}

bool SqliteResult::runPreparedQuery(SqliteCommand& command)
{
   _qryStr = command.sqlCommandText();
   auto dataSet  = command.executeResult();
   if (dataSet == nullptr)
      throw SqlException("runPreparedQuery, Invalid DataSet pointer", "SqliteResult"); 

   _affectedRows = command.affectedRows();
   return getData(std::move(dataSet), command.statement());
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

   if (_pDataset == nullptr)
      throw std::runtime_error("getVariantValue, Invalid DataSet pointer");

   return _pDataset->data().at(row).at(columnIndex);
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

   if (_pDataset == nullptr)
      throw std::runtime_error("getStringValue, Invalid DataSet pointer");

   auto& value = _pDataset->data().at(row).at(columnIndex);
   return VariantHelper::toString(value);
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

bool SqliteResult::getData(DataSetPtr dataSet, sqlite3_stmt* stmt)
{
   if (dataSet == nullptr || stmt == nullptr)
      throw tbs::SqlException("getData, invalid DataSet or statement object", "SqliteResult");

   _pDataset =  std::move(dataSet);
   _nRows    = _pDataset->totalRows();
   _nColumns = _pDataset->totalColumns();

   if (_nColumns > 0)
   {
      // we have total columns, now set up columns info
      setupColumnProperties(stmt);
      // OK, we have valid result set, now init metadatas
      setupColumnMetaData(stmt);
   }

   if (_nRows > 0)
      _resultStatus = ResultStatus::tuplesOk;
   else
      _resultStatus = ResultStatus::commandOk;

   _navigator.moveFirst();

   return true;
}

// -------------------------------------------------------
// Specific to SqliteResult
// -------------------------------------------------------

void SqliteResult::setupColumnProperties(sqlite3_stmt* stmt)
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
         const char* colname = sqlite3_column_name(stmt, i);
         _columnInfoCollection[i].name = std::string(colname);

         // save column defined size
         _columnInfoCollection[i].definedSize = FIELD_SIZE_UNKNOWN;

         std::string declaredTypeStr = sqliteColumnDeclaredType(stmt, i);
         if (declaredTypeStr == "DECLTYPE_UNKNOWN")
            declaredTypeStr = "Text";

         SqliteType sqliteType = sqliteTypeFromDeclaredType(declaredTypeStr);

         // save column native type as string
         _columnInfoCollection[i].nativeTypeStr = sqliteTypeToString(sqliteType);

         // save column native declared type as string
         _columnInfoCollection[i].nativeFullTypeStr = declaredTypeStr;

         // save column native type
         _columnInfoCollection[i].nativeType = (long)sqliteType;

         // save column data type : DataType
         _columnInfoCollection[i].dataType = sqliteTypeToDataType(sqliteType);
      }
   }
   catch (const std::exception& ex /*TypeException& ex*/)
   {
      onNotifyError(_pConn->logId() + ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "SqliteResult");
   }
}


void SqliteResult::setupColumnMetaData(sqlite3_stmt* stmt)
{
   // Note: https://www.sqlite.org/c3ref/column_database_name.html
   // The names returned are the original un-aliased names of the database, table, and column.
   // If the Nth column returned by the statement is an expression or subquery and is not a column value,
   // then all of these functions return NULL

   // Check first column, make sure that we are doing this on a table
   const char* tableName = sqlite3_column_table_name(stmt, 0);
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
      const char* tableName = sqlite3_column_table_name(stmt, i);
      if (tableName)
         _metadataCollection[i].tableName = std::string(tableName);
      else { 
         return ; 
      }  

      // retrieve column name
      const char* columnName = sqlite3_column_name(stmt, i);
      if (!columnName) { // Not a column, break now
         break;
      }

      // retrieve column origin name
      const char* columnOriginName = sqlite3_column_origin_name(stmt, i);
      if (!columnOriginName) { // Not a column, break now
         break;
      }

      // retrive metadata
      int rc = sqlite3_table_column_metadata(
                  _pConn->nativeConnection(),
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
         if (!dataType) {
            throw tbs::SqlException(tbsfmt::format("setupColumnMetaData, invalid column data type for column {}", columnName), "SqliteResult");
         }

         _metadataCollection[i].colIndex      = i;
         _metadataCollection[i].collSeqName   = std::string(collSeq);
         _metadataCollection[i].colName       = std::string(columnName);
         _metadataCollection[i].dataType      = std::string(dataType);

         _columnInfoCollection[i].autoIncrement = util::numToBool(isAutoInc);
         _columnInfoCollection[i].allowNull   = ! util::numToBool(isNotNull);
         _columnInfoCollection[i].primaryKey  = util::numToBool(isPrimaryKey);
      }
      else
      {
         onNotifyError(_pConn->logId() + _pConn->lastBackendError());
         throw tbs::SqlException(tbsfmt::format("setupColumnMetaData, {}", _pConn->lastBackendError()), "SqliteResult");
      }
   }
}


} // namespace sql
} // namespace tbs