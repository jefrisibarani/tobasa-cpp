#include "tobasasql/odbc_util.h"
#include "tobasasql/odbc_result.h"

namespace tbs {
namespace sql {

OdbcResult::OdbcResult(OdbcConnection* pconn)
    : ResultCommon()
{
   _pStatement    = nullptr;
   _pConn         = pconn;
   notifierSource = "OdbcResult";
   _navigator.init(std::bind(&OdbcResult::totalRows, this));
}

OdbcResult::~OdbcResult()
{
   if (_pStatement != nullptr)
      _pStatement = nullptr;
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string OdbcResult::name() const
{
   return "Odbc Result";
}

bool OdbcResult::runQuery(
   const std::string& sql,
   const SqlParameterCollection& parameters)
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

   SQLRETURN rc;
   _pStatement = _pConn->allocateStatement();

   // Convert SqlParameter to OdbcParameter
   OdbcParameterCollection odbcParams(_pStatement, static_cast<short>(parameters.size()));
   if (parameters.size() > 0)
   {
      odbcParams.prepare(_qryStr, parameters);
      odbcParams.bindParameter();
      rc = SQLExecute(_pStatement);
   }
   else
   {
      auto sqlcmd = tbs::util::utf8_to_odbcString(_qryStr);
      rc = SQLExecDirect(_pStatement, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);
   }


   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = _pConn->sqlPutData(_pStatement, parameters);
   }

   if (SQL_SUCCEEDED(rc) || rc == SQL_NO_DATA)
   {
      std::string notitymsg("SQL command executed successfully");
      OdbcDiagRecord diag;

      if (rc == SQL_SUCCESS_WITH_INFO)
         diag = statementDiagRecord(_pStatement, rc);

      // get affected rows
      SQLLEN affectedRow;
      rc = SQLRowCount(_pStatement, (SQLLEN*)&affectedRow);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(this);

      if (SQL_SUCCEEDED(rc))
      {
         _affectedRows = static_cast<int>( (affectedRow < 0) ? 0 : affectedRow );

         if (diag.message().empty())
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows);
         else {
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}, code: {}, info: {}",
                           _affectedRows, diag.code(), diag.message());
         }
      }

      if (_pConn->logExecuteStatus()) 
         onNotifyTrace(_pConn->logId() + notitymsg);

      // Get columns count
      SQLSMALLINT ncol = 0;
      rc = SQLNumResultCols(_pStatement, &ncol);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(this);

      if (SQL_SUCCEEDED(rc))
      {
         _nColumns = (int)ncol;
         setupColumnProperties();
      }

      // retrieve all rows
      int rowsRetrieved = 0;

      // SQL command returning no result, has 0 column
      // we got "Invalid cursor state" error if we do SQLFetch(_pStatement) on zero column
      if (_nColumns > 0)
      {
         // Loop through the rows in the result-set
         while (true)
         {
            rc = SQLFetch(_pStatement);
            statementDiagRecord(_pStatement, rc).throwOnError(this);

            if (SQL_SUCCEEDED(rc))
            {
               VectorVariant recordVariant;
               recordVariant.reserve(_nColumns);
               // Loop through the columns
               for (SQLUSMALLINT i = 1; i <= _nColumns; i++)
               {
                  VariantType vdata = _pConn->getFieldData(_pStatement, i);
                  recordVariant.emplace_back(vdata);
               }

               _dataVariant.emplace_back(recordVariant);
               rowsRetrieved++;
            }
            else
               break;
         }
      }

      _nRows = rowsRetrieved;
      if (_nRows > 0)
         _resultStatus = ResultStatus::tuplesOk;
      else
         _resultStatus = ResultStatus::commandOk;

      _navigator.moveFirst();

      // clean statement
      SQLFreeHandle(SQL_HANDLE_STMT, _pStatement);
      _pStatement = nullptr;

      return true;
   }
   else
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(this);

   return false;
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

   VariantType value = _dataVariant[row][columnIndex];
   return value;
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

   auto& value = _dataVariant[row][columnIndex];
   return VariantHelper<>::toString(value);
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

void OdbcResult::setupColumnProperties()
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

         // TODO_JEFRI : check col_decimal_digits

         SQLRETURN rc;
         rc = SQLDescribeCol(
                  _pStatement,         // SQLHSTMT       StatementHandle
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

            if (colDataType == SQL_FLOAT) // what about SQL_REAL ?
            {
               if ( colSize>=1 && colSize <=24 )
                  colDataTypeFinal = SQL_FLOAT;
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
            tbs::statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(this);
         }

         // Get auto increment field info
         rc = SQLColAttribute(
                  _pStatement,
                  i + 1,                        // ColumnNumber
                  SQL_DESC_AUTO_UNIQUE_VALUE,   // FieldIdentifier
                  NULL,                         // CharacterAttributePtr
                  0,                            // BufferLength
                  NULL,                         // StringLengthPtr
                  &colIdentity);                // NumericAttributePtr

         tbs::statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(this);

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

} // namespace sql
} // namespace tbs