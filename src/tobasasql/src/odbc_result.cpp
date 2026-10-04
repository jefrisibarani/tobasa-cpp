#include "tobasasql/odbc_util.h"
#include "tobasasql/odbc_result.h"

namespace tbs {
namespace sql {

OdbcResult::OdbcResult(OdbcConnection* pconn)
    : ResultCommon()
{
   _pConn         = pconn;
   _pDataset      = nullptr;
   notifierSource = "OdbcResult";
   _navigator.init(std::bind(&OdbcResult::totalRows, this));
}

OdbcResult::OdbcResult(OdbcResult&& other) noexcept
   : ResultCommon( std::move(other))
   , _pConn(       other._pConn)
   , _pDataset(    std::move(other._pDataset))
   , _navigator(   std::move(other._navigator))
{
   other._pConn         = nullptr;
   other._pDataset      = nullptr;
   other._nRows         = 0;
   other._nColumns      = 0;
   other._affectedRows  = 0;
   other._resultStatus  = ResultStatus::unknown;
   other._columnInfoCollection.clear();
   other._qryStr.clear();
}

OdbcResult& OdbcResult::operator=(OdbcResult&& other) noexcept
{
   if (this != &other)
   {
      ResultCommon::operator=(std::move(other));
      _pConn      = other._pConn;
      _pDataset   = std::move(other._pDataset);
      _navigator  = std::move(other._navigator);

      other._pConn         = nullptr;
      other._pDataset      = nullptr;
      other._nRows         = 0;
      other._nColumns      = 0;
      other._affectedRows  = 0;
      other._resultStatus  = ResultStatus::unknown;
      other._columnInfoCollection.clear();
      other._qryStr.clear();
   }
   return *this;
}

OdbcResult::~OdbcResult()
{
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string OdbcResult::name() const
{
   return "Odbc Result";
}

bool OdbcResult::runQuery(const std::string& sql,
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

   OdbcCommand command(_pConn);
   if (command.query(_qryStr, parameters))
   {
      auto dataSet  = command.executeResult();
      if (dataSet==nullptr)
         throw SqlException("runQuery, Invalid DataSet pointer", "SqliteResult"); 
      
      _affectedRows = command.affectedRows();
      return getData(std::move(dataSet), command.statement());
   }

   return false;
}

bool OdbcResult::runPreparedQuery(OdbcCommand& command)
{
   _qryStr = command.sqlCommandText();
   auto dataSet  = command.executeResult();
   if (dataSet == nullptr)
      throw SqlException("runPreparedQuery, Invalid DataSet pointer", "OdbcResult"); 

   _affectedRows = command.affectedRows();

   return getData(std::move(dataSet), command.statement());
}

void OdbcResult::connection(OdbcConnection* conn)
{
   _pConn = conn;
}

OdbcConnection* OdbcResult::connection() const { return _pConn; }

NavigatorBasic& OdbcResult::navigator() { return _navigator; }

// -------------------------------------------------------
// Override methods from base class : ResultCommon
// -------------------------------------------------------

TypeClass OdbcResult::columnTypeClass(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);

   TypeClass retVal;
   auto colType = columnNativeType(columnIndex);
   retVal = typeClassFromOdbcType(colType);
   return retVal;
}

OdbcResult::VariantType OdbcResult::getVariantValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   if (_pDataset == nullptr)
      throw std::runtime_error("getVariantValue, Invalid DataSet pointer");

   return _pDataset->data().at(row).at(columnIndex);
}

OdbcResult::VariantType OdbcResult::getVariantValue(const std::string& columnName) const
{
   return getVariantValue(columnNumber(columnName));
}

std::string OdbcResult::getStringValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   if (_pDataset == nullptr)
      throw std::runtime_error("getVariantValue, Invalid DataSet pointer");

   auto& value = _pDataset->data().at(row).at(columnIndex);
   return VariantHelper::toString(value);
}

std::string OdbcResult::getStringValue(const std::string& columnName) const
{
   return getStringValue(columnNumber(columnName));
}

bool OdbcResult::isNullField(const int columnIndex) const
{
   // TODO_JEFRI : do with better way
   return getStringValue(columnIndex) == sql::NULLSTR;
}

void OdbcResult::setupColumnProperties(SQLHSTMT stmt)
{
   if (_nColumns <= 0)
      return;

   // Note:
   // Make sure we allocate enough column name len
   // otherwise we got runtime exception: Stack around the variable 'colName' was corrupted.
   //const int TAB_LEN = SQL_MAX_TABLE_NAME_LEN + 100;
   const int COL_LEN = SQL_MAX_COLUMN_NAME_LEN + 100;

   _columnInfoCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _columnInfoCollection.emplace_back(ColumnInfo());
   }

   try
   {
      for (SQLUSMALLINT i = 0; i < _nColumns; i++)
      {
         SQLTCHAR    colName[COL_LEN] = { 0 };
         SQLSMALLINT colNameLength = sizeof(colName) / sizeof(SQLTCHAR);
         SQLSMALLINT colDataType;
         SQLULEN     colSize;
         SQLSMALLINT colDecimalDigits;
         SQLSMALLINT colNullable;
         SQLLEN      colIdentity = 0;

         // ODBC column indexing is 1-based, not 0-based.
         SQLRETURN rc;
         rc = SQLDescribeCol(
                  stmt,                // SQLHSTMT       StatementHandle
                  i + 1,               // SQLUSMALLINT   ColumnNumber
                  colName,             // SQLCHAR *      ColumnName
                  sizeof(colName),     // SQLSMALLINT    BufferLength
                  &colNameLength,      // SQLSMALLINT *  NameLengthPtr
                  &colDataType,        // SQLSMALLINT *  DataTypePtr
                  (SQLULEN*)&colSize,  // SQLULEN *      ColumnSizePtr
                  &colDecimalDigits,   // SQLSMALLINT *  DecimalDigitsPtr
                  &colNullable);       // SQLSMALLINT *  NullablePtr

         if (SQL_SUCCEEDED(rc))
         {
            SQLSMALLINT colDataTypeFinal = colDataType;

            // save column name
            std::string strField(tbs::util::odbcString_to_utf8(colName));
            util::strLower(strField);
            _columnInfoCollection[i].name = strField;

            // save column defined size
            _columnInfoCollection[i].definedSize = static_cast<long>(colSize);

            if (colDataType == SQL_FLOAT || colDataType == SQL_REAL || colDataType == SQL_DOUBLE)
            {
               if ( colSize>=1 && colSize <=24 ) {
                  colDataTypeFinal = SQL_FLOAT;
               }
               else if (colSize>=25 && colSize <=53)
               {
                  // double in SQL server is float 53
                  colDataTypeFinal = SQL_DOUBLE;
               }
            }

            // save column native type as string
            _columnInfoCollection[i].nativeTypeStr = sql::odbcDataTypeToString(colDataTypeFinal);

            // save column native full type as string
            _columnInfoCollection[i].nativeFullTypeStr = sql::odbcDataTypeToString(colDataTypeFinal);

            // save column native type
            _columnInfoCollection[i].nativeType = (long)colDataTypeFinal;

            // save column data type : sql::DataType
            _columnInfoCollection[i].dataType = sql::odbcTypeToDataType(colDataTypeFinal);
         }
         else
         {
            _columnInfoCollection[i].nativeTypeStr = "Unknown";
            tbs::statementDiagRecord(stmt, rc).throwOnNotSucceeded(this);
         }

         // Get auto increment field info
         rc = SQLColAttribute(
                  stmt,
                  i + 1,                        // ColumnNumber
                  SQL_DESC_AUTO_UNIQUE_VALUE,   // FieldIdentifier
                  NULL,                         // CharacterAttributePtr
                  0,                            // BufferLength
                  NULL,                         // StringLengthPtr
                  &colIdentity);                // NumericAttributePtr

         tbs::statementDiagRecord(stmt, rc).throwOnNotSucceeded(this);

         if (colIdentity == 1)
            _columnInfoCollection[i].autoIncrement = true;
      }
   }
   catch (const TypeException & ex)
   {
      onNotifyError(_pConn->logId() + ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "OdbcResult");
   }
}

bool OdbcResult::getData(DataSetPtr dataSet, SQLHSTMT stmt)
{
   if (dataSet == nullptr || stmt == SQL_NULL_HSTMT)
      throw tbs::SqlException("getData, invalid DataSet or statement object", "OdbcResult");

   _pDataset = std::move(dataSet);
   _nRows    = _pDataset->totalRows();
   _nColumns = _pDataset->totalColumns();

   if (_nColumns > 0)
   {
      // we have total columns, now set up columns info
      setupColumnProperties(stmt);
   }

   if (_nRows > 0)
      _resultStatus = ResultStatus::tuplesOk;
   else
      _resultStatus = ResultStatus::commandOk;

   _navigator.moveFirst();

   return true;
}

} // namespace sql
} // namespace tbs