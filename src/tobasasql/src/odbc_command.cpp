#include <tobasa/variant_helper.h>
#include <tobasa/datetime.h>
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sql_util.h"
#include "tobasasql/odbc_util.h"
#include "tobasasql/odbc_result.h"
#include "tobasasql/odbc_connection.h"
#include "tobasasql/odbc_command.h"

namespace tbs {
namespace sql {

OdbcCommand::OdbcCommand(OdbcConnection* conn)
   : _pConn(conn)
   , _pOdbcConn(nullptr)
   , _pStatement(SQL_NULL_HSTMT)
   , _affectedRows(-1)
   , _resultSetOpen(false)
{
   if (_pConn)
      _pOdbcConn = _pConn->nativeConnection();
   
   notifierSource = "OdbcCommand";
}

OdbcCommand::~OdbcCommand()
{
   close();
}

bool OdbcCommand::query(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (!prepare(sql))
      return false;

   return bind(parameters);
}

bool OdbcCommand::prepare(const std::string& sql)
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   close();

   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, _pOdbcConn, &_pStatement);
   if (!SQL_SUCCEEDED(rc))
      throw SqlException(connectionDiagRecord(_pOdbcConn, rc).message(), "OdbcCommand");

   _sql = sql;

   return true;
}

bool OdbcCommand::bind(const SqlParameterCollection& parameters)
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   if (_pStatement == SQL_NULL_HSTMT)
      throw SqlException("Invalid statement object", "OdbcCommand");

   onNotifyTrace(_pConn->logId() + "OdbcCommand, Binding paramaters" );

   SQLFreeStmt(_pStatement, SQL_CLOSE);
   _boundParameters = &parameters;

   try
   {
      if (parameters.empty())
      {
         auto sqlcmd = tbs::util::utf8_to_odbcString(_sql);
         //SQLRETURN rc = SQLExecDirect(pStmt, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);
         SQLRETURN rc = SQLPrepare(_pStatement, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);
         
         statementDiagRecord(_pStatement, rc).throwOnNotSucceeded();
      }
      else
      {
         _parameterCollection = std::make_unique<OdbcParameterCollection>(_pStatement, static_cast<short>(parameters.size()));
         _parameterCollection->prepare(_sql, parameters);
         _parameterCollection->bindParameter();
      }
   }
   catch (...)
   {
      //_boundParameters->clear();
      close();
      throw;
   }

   return true;
}

void OdbcCommand::reset()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   if (_pStatement == SQL_NULL_HSTMT)
      throw SqlException("Invalid statement object", "OdbcCommand");

   SQLFreeStmt(_pStatement, SQL_CLOSE);
   //_boundParameters->clear();
   _affectedRows = -1;
   _resultSetOpen = false;
}

void OdbcCommand::close()
{
   _parameterCollection.reset();

   _sql.clear();
   _resultSetOpen = false;

   if (_pStatement != SQL_NULL_HSTMT)
   {
      SQLFreeHandle(SQL_HANDLE_STMT, _pStatement);
      _pStatement = SQL_NULL_HSTMT;
   }

   _affectedRows = -1;
}

int OdbcCommand::execute()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   if (_pStatement == SQL_NULL_HSTMT)
      throw SqlException("Invalid statement object", "OdbcCommand");

   if (_resultSetOpen)
      throw SqlException("Previous result set is still active; call reset() or close() before execute() again", "OdbcCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("execute: {}", _sql));

   SQLRETURN rc = SQLExecute(_pStatement);
   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = sqlPutData(_pConn, _pStatement, *_boundParameters);
   }

   if (SQL_SUCCEEDED(rc) || rc == SQL_NO_DATA)
   {
      OdbcDiagRecord diag;
      int returnValue = 0;

      if (rc == SQL_SUCCESS_WITH_INFO)
         diag = statementDiagRecord(_pStatement, rc);

      // affected rows
      SQLLEN affectedRow;
      rc = SQLRowCount(_pStatement, (SQLLEN*)&affectedRow);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);
      _affectedRows = affectedRow < 0 ? 0 : affectedRow;


      std::string notitymsg("SQL command executed successfully");
      if (SQL_SUCCEEDED(rc))
      {
         returnValue = (int)affectedRow;
         if (diag.message().empty())
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRow);
         else
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}, code: {}, info: {}", affectedRow, diag.code(), diag.message());

         // Get columns count
         SQLSMALLINT ncol = 0;
         rc = SQLNumResultCols(_pStatement, &ncol);
         statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);
         _resultSetOpen = (ncol > 0);

         // clean statement
         //SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
         //pStmt = nullptr;
      }

      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + notitymsg);

      // affectedRow by successfull command other than UPDATE, INSERT, and DELETE is -1
      if (returnValue < 0) {
         returnValue = 0;
      }

      // Close the result cursor before returning, so the statement can be
      // reused after the caller decides to reset/close it. 
      SQLCloseCursor(_pStatement);
      _resultSetOpen = false;

      return returnValue;
   }
   else
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);

   return -1;
}

std::string OdbcCommand::executeScalar()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   if (_pStatement == SQL_NULL_HSTMT)
      throw SqlException("Invalid statement object", "OdbcCommand");

   if (_resultSetOpen)
      throw SqlException("Previous result set is still active; call reset() or close() before executeScalar() again", "OdbcCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeScalar: {}", _sql));

   SQLRETURN rc = SQLExecute(_pStatement);
   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = sqlPutData(_pConn, _pStatement, *_boundParameters);
   }

   if (SQL_SUCCEEDED(rc) || rc == SQL_NO_DATA)
   {
      std::string notitymsg("SQL command executed successfully");
      OdbcDiagRecord diag;

      if (rc == SQL_SUCCESS_WITH_INFO)
         diag = statementDiagRecord(_pStatement, rc);

      // affected rows
      SQLLEN affectedRow;
      rc = SQLRowCount(_pStatement, (SQLLEN*)&affectedRow);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);
      _affectedRows = affectedRow < 0 ? 0 : affectedRow;


      if (SQL_SUCCEEDED(rc))
      {
         if (diag.message().empty())
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRow);
         else
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}, code: {}, info: {}",
                              affectedRow, diag.code(), diag.message());
      }

      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + notitymsg);

      // Get columns count
      SQLSMALLINT ncol = 0;
      rc = SQLNumResultCols(_pStatement, &ncol);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);
      _resultSetOpen = (ncol > 0);

      // get first data
      rc = SQLFetch(_pStatement);
      statementDiagRecord(_pStatement, rc).throwOnError(_pConn);

      if (SQL_SUCCEEDED(rc))
      {
         // column index start from 1
         VariantType vdata = getFieldData(_pConn,_pStatement, 1);

         // Close the result cursor before returning, so the statement can be
         // reused after the caller decides to reset/close it. 
         SQLCloseCursor(_pStatement);
         _resultSetOpen = false;

         return VariantHelper::toString(vdata);
      }
   }
   else
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);

   return "";
}

OdbcCommand::DataSetPtr OdbcCommand::executeResult()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   if (_pStatement == SQL_NULL_HSTMT)
      throw SqlException("Invalid statement object", "OdbcCommand");

   if (_resultSetOpen)
      throw SqlException("Previous result set is still active; call reset() or close() before executeResult() again", "OdbcCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeResult: {}", _sql));

   SQLRETURN rc = SQLExecute(_pStatement);
   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = sqlPutData(_pConn, _pStatement, *_boundParameters);
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
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);

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
         onNotifyDebug(_pConn->logId() + notitymsg);

      auto dataSet = std::make_shared<DataSet<VariantType>>();

      // Get columns count
      SQLSMALLINT columns = 0;
      rc = SQLNumResultCols(_pStatement, &columns);
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);

      dataSet->totalColumns(columns);
      _resultSetOpen = (columns > 0);

      // SQL command returning no result, has 0 column
      // we got "Invalid cursor state" error if we do SQLFetch(_pStatement) on zero column
      if (columns <= 0)
      {
         _affectedRows = 0;
         _resultSetOpen = false;
         return dataSet;
      }

      while (true)
      {
         rc = SQLFetch(_pStatement);
         statementDiagRecord(_pStatement, rc).throwOnError(_pConn);

         if (rc == SQL_NO_DATA)
            break;

         if (!SQL_SUCCEEDED(rc))
            break;

         VectorVariant row;
         row.reserve(columns);
         // Loop through the columns
         for (SQLUSMALLINT i = 1; i <= columns; i++)
         {
            VariantType vdata = getFieldData(_pConn,_pStatement, i);
            row.emplace_back(vdata);
         }

         dataSet->data().emplace_back(std::move(row));
      }

      SQLCloseCursor(_pStatement);
      _resultSetOpen = false;

      return dataSet;
   }
   else
      statementDiagRecord(_pStatement, rc).throwOnNotSucceeded(_pConn);

   return nullptr;
}

OdbcResult OdbcCommand::executeSqlResult()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
      throw SqlException("Invalid connection object", "OdbcCommand");

   OdbcResult result;
   result.connection(_pConn);
   result.runPreparedQuery(*this);

   return std::move(result);
}

std::string OdbcCommand::lastBackendError()
{
   if (_pOdbcConn == SQL_NULL_HDBC)
   {
      onNotifyError("lastBackendError, invalid connection object", "OdbcCommand");
      return "invalid connection object";
   }

   return connectionDiagRecord(_pOdbcConn, SQL_ERROR).message();
}

std::string OdbcCommand::statementError()
{
   if (_pStatement == SQL_NULL_HSTMT)
      return lastBackendError();

   return statementDiagRecord(_pStatement, SQL_ERROR).message();
}

int OdbcCommand::affectedRows() 
{ 
   return static_cast<int>(_affectedRows); 
}

// --------------------------------------------------------------------------
SQLRETURN OdbcCommand::sqlPutData(OdbcConnection* conn, SQLHSTMT pStmt, const SqlParameterCollection& parameters)
{
   // Supply data-at-execution
   // Note: https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/sending-long-data?view=sql-server-ver15

   SQLRETURN rc;

   // find parameter
   SQLPOINTER pParamPos;
   rc = SQLParamData(pStmt, &pParamPos);

   // for each parameters that need to send data in segments
   while (rc == SQL_NEED_DATA)
   {
#if defined(_MSC_VER)
      unsigned long paramPos = ::PtrToUlong(pParamPos);
#else
      unsigned long paramPos = (unsigned long)(unsigned long*) pParamPos;
#endif
      // we need SqlParameter object not OdbcParameter, because SqlParameter holds actual data
      auto& param = parameters[paramPos-1];

      if (conn->logExecuteStatus())
         conn->onNotifyTrace(conn->logId() + tbsfmt::format("sqlPutData: Processing data-at-execution parameter no: {} name: {}", paramPos, param->name() ));

      // the raw binary data pointer
      SQLLEN   lbytes = (SQLLEN) param->size();
      uint8_t* pBlob  = *(param->valueBinaryPtr());
      constexpr SQLLEN PUTDATA_BUFFER = 512;

      if (lbytes < PUTDATA_BUFFER) {
         rc = SQLPutData(pStmt, (SQLPOINTER)pBlob, lbytes);
      }
      else
      {
         // Send data in segment
         while (lbytes > PUTDATA_BUFFER)
         {
            rc = SQLPutData(pStmt, (SQLPOINTER)pBlob, PUTDATA_BUFFER);
            
            if (conn->logExecuteStatus())
               conn->onNotifyTrace(conn->logId() + tbsfmt::format("sqlPutData: parameter no: {} name: {}, bytes remaining {}:", paramPos, param->name(), lbytes ));

            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
               statementDiagRecord(pStmt, rc).throwOnNotSucceeded(conn);
            }

            pBlob  += PUTDATA_BUFFER;
            lbytes -= PUTDATA_BUFFER;
         }

         // Put final segment
         rc = SQLPutData(pStmt, (SQLPOINTER)pBlob, lbytes);
         
         if (conn->logExecuteStatus())
            conn->onNotifyTrace(conn->logId() + tbsfmt::format("sqlPutData: parameter no: {} name: {}, final bytes {}:", paramPos, param->name(),lbytes ));
      }

      if ( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
         statementDiagRecord(pStmt, rc).throwOnNotSucceeded(conn);
      }

      // ask for next parameter
      rc = SQLParamData(pStmt, &pParamPos);
   }

   return rc;
}


OdbcCommand::VariantType OdbcCommand::getFieldData(OdbcConnection* conn, SQLHSTMT pStmt, int col)
{
   // NOTE: ODBC column indexing is 1-based, not 0-based.

   if (col < 0)
      throw tbs::SqlException("Invalid column index in getFieldData", "OdbcCommand");

   SQLRETURN   rc;
   SQLLEN      dataTypeFinal = -1;

   // {
   //    SQLLEN      colType = -1;     // Data type returned by ODBC
   //    SQLSMALLINT buflen;
   //    rc = SQLColAttribute(
   //          pStmt,               // SQLHSTMT        StatementHandle,
   //          col,                 // SQLUSMALLINT    ColumnNumber
   //          SQL_DESC_TYPE,       // SQLUSMALLINT    FieldIdentifier
   //          NULL,                // SQLPOINTER      CharacterAttributePtr
   //          0,                   // SQLSMALLINT     BufferLength
   //          &buflen,             // SQLSMALLINT *   StringLengthPtr
   //          (SQLLEN*)&colType);  // SQLLEN *        NumericAttributePtr

   //    statementDiagRecord(pStmt, rc).throwOnNotSucceeded(conn);
   //    dataTypeFinal = colType;
   // }

   {
      SQLSMALLINT dataType;
      SQLULEN     columnSize;
      SQLSMALLINT decimalDigits;
      SQLRETURN   rc;
      rc =  SQLDescribeCol(
               pStmt,            // SQLHSTMT       StatementHandle
               col,              // SQLUSMALLINT   ColumnNumber
               nullptr,          // SQLCHAR *      ColumnName
               0,                // SQLSMALLINT    BufferLength
               nullptr,          // SQLSMALLINT *  NameLengthPtr
               &dataType,        // SQLSMALLINT *  DataTypePtr
               &columnSize,      // SQLULEN *      ColumnSizePtr
               &decimalDigits,   // SQLSMALLINT *  DecimalDigitsPtr
               nullptr           // SQLSMALLINT *  NullablePtr
            );
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(conn);

      dataTypeFinal = dataType;
      if (dataType == SQL_FLOAT || dataType == SQL_REAL || dataType == SQL_DOUBLE)
      {
         if ( columnSize >= 1 && columnSize <=24 ) {
            dataTypeFinal = SQL_FLOAT;
         }
         else if ( columnSize >=25 && columnSize <=53 )
         {
            // double in SQL server is float 53
            dataTypeFinal = SQL_DOUBLE;
         }
      }
   }
   
   if (dataTypeFinal == SQL_SMALLINT || dataTypeFinal == SQL_TINYINT) 
   {
      int32_t ret;
      SQLLEN sqlPtr;
      rc = SQLGetData(pStmt, col, SQL_C_SHORT, &ret, 0, (SQLLEN*)&sqlPtr);
      statementDiagRecord(pStmt, rc).throwOnError(conn);
      if (sqlPtr == SQL_NULL_DATA)
         return std::monostate{};
      else
         return ret;
   }
   else if (dataTypeFinal == SQL_INTEGER) 
   {
      int32_t ret;
      SQLLEN sqlPtr;
      rc = SQLGetData(pStmt, col, SQL_C_LONG, &ret, 0, (SQLLEN*)&sqlPtr);
      statementDiagRecord(pStmt, rc).throwOnError(conn);
      if (sqlPtr == SQL_NULL_DATA)
         return std::monostate{};
      else
         return ret;
   }
   else if (dataTypeFinal == SQL_BIGINT) 
   {
      int64_t ret;
      SQLLEN sqlPtr;
      rc = SQLGetData(pStmt, col, SQL_C_SBIGINT, &ret, 0, (SQLLEN*)&sqlPtr);
      statementDiagRecord(pStmt, rc).throwOnError(conn);
      if (sqlPtr == SQL_NULL_DATA)
         return std::monostate{};
      else
         return ret;
   }
   else if (dataTypeFinal == SQL_FLOAT || dataTypeFinal == SQL_REAL) 
   {
      float  ret;
      SQLLEN sqlPtr;
      rc = SQLGetData(pStmt, col, SQL_C_FLOAT, &ret, 0, (SQLLEN*)&sqlPtr);
      statementDiagRecord(pStmt, rc).throwOnError(conn);
      if (sqlPtr == SQL_NULL_DATA)
         return std::monostate{};
      else
         return ret;
   }
   else if (dataTypeFinal == SQL_DOUBLE) 
   {
      double ret;
      SQLLEN sqlPtr;
      rc = SQLGetData(pStmt, col, SQL_C_DOUBLE, &ret, 0, (SQLLEN*)&sqlPtr);
      statementDiagRecord(pStmt, rc).throwOnError(conn);
      if (sqlPtr == SQL_NULL_DATA)
         return std::monostate{};
      else
         return ret;
   }
   else
   {
      // Getting long/large size data
      // https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/getting-long-data?view=sql-server-ver15

      std::string strValue;
      SQLLEN valueLenOrInd;
      SQLLEN dataRetrieved = 0;

      // allocate 512 void* pointer, on 64bit machine, sizeof pointer is 8 byte
      // buffer will have 4096 bytes
      SQLPOINTER  buffer[512] = { 0 };
      SQLLEN      bufferLen = sizeof(buffer);

      while (true)
      {
         rc = SQLGetData(
               pStmt,                    // SQLHSTMT       StatementHandle
               col,                      // SQLUSMALLINT   Col_or_Param_Num
               SQL_C_TCHAR,              // SQLSMALLINT    TargetType
               buffer,                   // SQLPOINTER     TargetValuePtr
               bufferLen,                // SQLLEN         BufferLength ( in bytes)
               (SQLLEN*)&valueLenOrInd); // SQLLEN *       StrLen_or_IndPtr

         statementDiagRecord(pStmt, rc).throwOnError(conn);

         if (SQL_SUCCEEDED(rc))
         {
            /*
            if (rc == SQL_SUCCESS_WITH_INFO)
            {
               // we may got "[Microsoft][ODBC Driver 17 for SQL Server]String data, right truncation"  here
               //OdbcDiagRecord diag(pStmt, SQL_HANDLE_STMT, rc);
               onNotifyTrace(logId() + tbsfmt::format("[{}] {}", diag.state(), diag.message()));
            }
            */
            dataRetrieved = (valueLenOrInd > bufferLen) || (valueLenOrInd == SQL_NO_TOTAL) ? bufferLen : valueLenOrInd;

            if (dataRetrieved > 0)
            {
               std::string tmp = tbs::util::odbcString_to_utf8((const SQLTCHAR*)buffer);
               strValue += tmp;
            }
            else if (valueLenOrInd == SQL_NULL_DATA)
               return std::monostate{};
         }
         else if (rc == SQL_SUCCESS || rc == SQL_NO_DATA) {
            break;
         }
         else 
         {
            // TODO_JEFRI: SQL_STILL_EXECUTING
            //statementDiagRecord(pStmt, rc).throwException(this);
            break;
         }
      }

      if (SQL_NO_DATA)
      {
         // TODO_JEFRI
      }

      switch (dataTypeFinal)
      {
         case SQL_CHAR:
         case SQL_VARCHAR:
         case SQL_LONGVARCHAR:
         case SQL_WCHAR:
         case SQL_WVARCHAR:
         case SQL_WLONGVARCHAR:
            return strValue;
         // case SQL_SMALLINT:
         // case SQL_TINYINT:
         //    return std::stoi(strValue);
         // case SQL_INTEGER:
         //   return std::stol(strValue);
         // case SQL_BIGINT:
         //   return static_cast<int64_t>(std::stoll(strValue));
         // case SQL_FLOAT:
         //    return std::stof(strValue);
         // case SQL_REAL:
         //    return std::stof(strValue);
         // case SQL_DOUBLE:
         //    return std::stod(strValue);
         case SQL_DATETIME:
            return strValue;
         case SQL_DECIMAL:
            return strValue;
         case SQL_NUMERIC:
            return strValue;
         case SQL_TYPE_DATE:
            // Note; https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-date-time/data-type-support-for-odbc-date-and-time-improvements?view=sql-server-ver15
         case -154:    // SQL_SS_TIME2	-154 (SQLNCLI.h)
         case SQL_TYPE_TIME:
            return strValue;
         case -155:   // SQL_SS_TIMESTAMPOFFSET - 155 (SQLNCLI.h)
         case SQL_TYPE_TIMESTAMP:
            return strValue;
         case SQL_BIT:
            return util::strToBool(strValue);
         case SQL_VARBINARY:
         case SQL_LONGVARBINARY:
            return strValue;
         default:
            return strValue;
      }

      return strValue;
   }
}

} // namespace sql
} // namespace tbs