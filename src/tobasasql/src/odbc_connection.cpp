#include <tobasa/variant_helper.h>
#include "tobasasql/odbc_util.h"
#include "tobasasql/odbc_connection.h"

namespace tbs {
namespace sql {

OdbcConnection::OdbcConnection()
   : ConnectionCommon()
{
   _pDbc          = nullptr;
   _pEnv          = nullptr;
   notifierSource = "OdbcConnection";
}

OdbcConnection::~OdbcConnection()
{
   // odbc handles cleanup done in disconnect()
   disconnect();

   if (_pDbc != nullptr)
      _pDbc = nullptr;

   if (_pEnv != nullptr)
      _pEnv = nullptr;
}

std::string OdbcConnection::name()
{
   return "Odbc Connection";
}

bool OdbcConnection::connect(const std::string& connString)
{
   if (status() == ConnectionStatus::ok)
      return true;

   if (!environmentAndConnectionHandleAlive())
      allocateEnvironmentAndConnectionHandle();

   if (connString.empty())
   {
      onNotifyDebug(logId() + "Empty connection string not allowed");
      throw tbs::SqlException("Empty connection string not allowed", "OdbcConnection");
   }

   // Note: 
   // SQLTCHAR is SQLWCHAR in windows(on UNICODE build resolve to wchar_t) in windows, SQLCHAR in linux
   // https://developer.rhino3d.com/guides/cpp/using-sizeof-with-tchar-wchar-t/
   // To get correct total characters: sizeof(buffer)/sizeof(SQLTCHAR)

   SQLRETURN   rc;
   SQLSMALLINT outConnStrLen;
   SQLTCHAR    outConnStrBuff[8192] = {0};
   SQLSMALLINT outConnStrBuffLen    = sizeof(outConnStrBuff) / sizeof(SQLTCHAR);

   auto inConnStr = tbs::util::utf8_to_odbcString(connString);

   rc = SQLDriverConnect(
         (SQLHDBC)_pDbc,                  // SQLHDBC      ConnectionHandle
         NULL,                            // SQLHWND      WindowHandle
         (SQLTCHAR*)inConnStr.c_str(),    // SQLCHAR*     InConnectionString
         SQL_NTS,                         // SQLSMALLINT  StringLength1
         (SQLTCHAR*)outConnStrBuff,       // SQLCHAR*     OutConnectionString
         outConnStrBuffLen,               // SQLSMALLINT  BufferLength, in characters
         &outConnStrLen,                  // SQLSMALLINT* StringLength2Ptr
         SQL_DRIVER_NOPROMPT);            // SQLUSMALLINT DriverCompletion

   connectionDiagRecord(_pDbc, rc).throwOnNotSucceeded(this);

   if (SQL_SUCCEEDED(rc))
   {
      _connStatus  = ConnectionStatus::ok;
      _dbmsName    = getInfo(SQL_DBMS_NAME);
      _dbmsVersion = getInfo(SQL_DBMS_VER);

      onNotifyDebug(logId() + tbsfmt::format("Connected to : {} {}", _dbmsName, _dbmsVersion));
      return true;
   }

   return false;
}

bool OdbcConnection::disconnect()
{
   if (status() == ConnectionStatus::ok)
   {
      if (_pDbc != SQL_NULL_HDBC)
      {
         SQLRETURN rc = SQLDisconnect((SQLHDBC)_pDbc);
         if (!SQL_SUCCEEDED(rc))
         {
            OdbcDiagRecord diag(_pDbc, SQL_HANDLE_DBC, rc);
            onNotifyError(logId() + diag.message());
         }

         rc = SQLFreeHandle(SQL_HANDLE_DBC, (SQLHDBC)_pDbc);
         if (!SQL_SUCCEEDED(rc))
         {
            OdbcDiagRecord diag(_pDbc, SQL_HANDLE_DBC, rc);
            onNotifyError(logId() + diag.message());
         }
      }

      if (_pEnv != SQL_NULL_HENV)
      {
         SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, (SQLHENV)_pEnv);
         if (!SQL_SUCCEEDED(rc))
         {
            OdbcDiagRecord diag(_pEnv, SQL_HANDLE_ENV, rc);
            onNotifyError(logId() + diag.message());
         }
      }

      _pDbc       = nullptr;
      _pEnv       = nullptr;
      _connStatus = ConnectionStatus::bad;
   }

   return true;
}

ConnectionStatus OdbcConnection::status()
{
   if (!environmentAndConnectionHandleAlive())
   {
      _connStatus = ConnectionStatus::bad;
      return _connStatus;
   }

   if ( checkStatus() )
      return _connStatus;
   else 
   {
      _connStatus = ConnectionStatus::bad;
      throw tbs::SqlException("invalid connection object", "OdbcConnection");
   }
}

int OdbcConnection::execute(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "OdbcConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

   SQLRETURN rc;
   SQLHSTMT pStmt = allocateStatement();

   // Convert SqlParameter to OdbcParameter
   OdbcParameterCollection odbcParams(pStmt, static_cast<short>(parameters.size()));
   if (parameters.size() > 0)
   {
      odbcParams.prepare(sql, parameters);
      odbcParams.bindParameter();
      rc = SQLExecute(pStmt);
   }
   else
   {
      auto sqlcmd = tbs::util::utf8_to_odbcString(sql);
      rc = SQLExecDirect(pStmt, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);
   }

   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = sqlPutData(pStmt, parameters);
   }

   if (SQL_SUCCEEDED(rc) || rc == SQL_NO_DATA)
   {
      std::string notitymsg("SQL command executed successfully");
      OdbcDiagRecord diag;
      int returnValue = 0;

      if (rc == SQL_SUCCESS_WITH_INFO)
         diag = statementDiagRecord(pStmt, rc);

      SQLLEN affectedRow;
      rc = SQLRowCount(pStmt, (SQLLEN*)&affectedRow);
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

      if (SQL_SUCCEEDED(rc))
      {
         returnValue = (int)affectedRow;
         if (diag.message().empty())
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRow);
         else
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}, code: {}, info: {}", affectedRow, diag.code(), diag.message());
      }

      if (logExecuteStatus()) 
         onNotifyTrace(logId() + notitymsg);

      //clean statement
      SQLFreeHandle(SQL_HANDLE_STMT, pStmt);

      // affectedRow by successfull command other than UPDATE, INSERT, and DELETE is -1
      if (returnValue < 0) {
         returnValue = 0;
      }

      return returnValue;
   }
   else
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   return -1;
}

std::string OdbcConnection::executeScalar(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "OdbcConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("executeScalar: {}", sql));

   SQLRETURN rc;
   SQLHSTMT pStmt = allocateStatement();

   // Convert SqlParameter to OdbcParameter
   OdbcParameterCollection odbcParams(pStmt, static_cast<short>(parameters.size()));
   if (parameters.size() > 0)
   {
      odbcParams.prepare(sql, parameters);
      odbcParams.bindParameter();
      rc = SQLExecute(pStmt);
   }
   else
   {
      auto sqlcmd = tbs::util::utf8_to_odbcString(sql);
      rc = SQLExecDirect(pStmt, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);
   }

   if (rc == SQL_NEED_DATA)
   {
      // process data-at-execution parameters
      rc = sqlPutData(pStmt, parameters);
   }

   if (SQL_SUCCEEDED(rc) || rc == SQL_NO_DATA)
   {
      std::string notitymsg("SQL command executed successfully");
      OdbcDiagRecord diag;

      if (rc == SQL_SUCCESS_WITH_INFO)
         diag = statementDiagRecord(pStmt, rc);

      SQLLEN affectedRow;
      rc = SQLRowCount(pStmt, (SQLLEN*)&affectedRow);
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

      if (SQL_SUCCEEDED(rc))
      {
         if (diag.message().empty())
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRow);
         else
            notitymsg = tbsfmt::format("SQL command executed successfully, affectedRows: {}, code: {}, info: {}",
                              affectedRow, diag.code(), diag.message());
      }

      if (logExecuteStatus()) 
         onNotifyTrace(logId() + notitymsg);

      // Get columns count
      SQLSMALLINT ncol = 0;
      rc = SQLNumResultCols(pStmt, &ncol);
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

      // get first data
      rc = SQLFetch(pStmt);
      statementDiagRecord(pStmt, rc).throwOnError(this);

      if (SQL_SUCCEEDED(rc))
      {
         // column index start from 1
         VariantType vdata = getFieldData(pStmt, 1);

         // clean statement
         SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
         pStmt = nullptr;

         return VariantHelper<>::toString(vdata);
      }
   }
   else
      statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   return "";
}

std::string OdbcConnection::versionString()
{
   if (status() != ConnectionStatus::ok)
      return "";

   std::string libVersion = getInfo(SQL_DRIVER_VER);
   return tbsfmt::format("{} {}, ODBC Library version: {}", _dbmsName, _dbmsVersion, libVersion);
}

std::string OdbcConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("SELECT DB_NAME() AS database_name");
}

BackendType OdbcConnection::backendType() const { return BackendType::odbc; }

std::string OdbcConnection::dbmsName() { return _dbmsName; }

int64_t OdbcConnection::lastInsertRowid()
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "OdbcConnection");

   SqlApplyLogInternal applyLogRule(this);

   // Note: this is for MS SQL only. Fixes need for other sql backend 
   auto rowId = executeScalar("SELECT @@IDENTITY");
   if (rowId.empty())
      throw tbs::SqlException("Invalid last inserted row id", "OdbcConnection");

   if (!util::isNumber(rowId))
      throw tbs::SqlException("Invalid last inserted row id", "OdbcConnection");
      
   return std::stoll( rowId );
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

SQLHSTMT OdbcConnection::allocateStatement()
{
   SQLRETURN rc;
   SQLHSTMT  pStmt = nullptr;

   rc = SQLAllocHandle(SQL_HANDLE_STMT, _pDbc, &pStmt);
   connectionDiagRecord(_pDbc, rc).throwOnNotSucceeded(this);
   return pStmt;
}

OdbcConnection::VariantType OdbcConnection::getFieldData(SQLHSTMT pStmt, int col)
{
   if (col < 0)
      throw tbs::SqlException("Invalid column index in getFieldData", "OdbcConnection");

   SQLRETURN   rc;
   SQLSMALLINT buflen;
   SQLLEN      colType = 0;

   rc = SQLColAttribute(
         pStmt,               // SQLHSTMT        StatementHandle,
         col,                 // SQLUSMALLINT    ColumnNumber
         SQL_DESC_TYPE,       // SQLUSMALLINT    FieldIdentifier
         NULL,                // SQLPOINTER      CharacterAttributePtr
         0,                   // SQLSMALLINT     BufferLength
         &buflen,             // SQLSMALLINT *   StringLengthPtr
         (SQLLEN*)&colType);  // SQLLEN *        NumericAttributePtr

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   switch (colType)
   {
      case SQL_FLOAT:
      case SQL_REAL:
      {
         float  ret;
         SQLLEN sqlPtr;

         rc = SQLGetData(
            pStmt,               // SQLHSTMT       StatementHandle
            col,                 // SQLUSMALLINT   Col_or_Param_Num
            SQL_C_FLOAT,         // SQLSMALLINT    TargetType
            &ret,                // SQLPOINTER     TargetValuePtr
            0,                   // SQLLEN         BufferLength
            (SQLLEN*)&sqlPtr);   // SQLLEN *       StrLen_or_IndPtr

         statementDiagRecord(pStmt, rc).throwOnError(this);

         if (sqlPtr == SQL_NULL_DATA)
            return std::monostate{};
         else
            return ret;
      }
      break;

      case SQL_DOUBLE:
      {
         double ret;
         SQLLEN sqlPtr;

         rc = SQLGetData(
            pStmt,               // SQLHSTMT       StatementHandle
            col,                 // SQLUSMALLINT   Col_or_Param_Num
            SQL_C_DOUBLE,        // SQLSMALLINT    TargetType
            &ret,                // SQLPOINTER     TargetValuePtr
            0,                   // SQLLEN         BufferLength
            (SQLLEN*)&sqlPtr);   // SQLLEN *       StrLen_or_IndPtr

         statementDiagRecord(pStmt, rc).throwOnError(this);

         if (sqlPtr == SQL_NULL_DATA)
            return std::monostate{};
         else
            return ret;
      }
      break;

#if 0
      case SQL_DATETIME:
      {
         TIMESTAMP_STRUCT  ret;
         SQLLEN            sqlPtr;

         rc = SQLGetData(
            pStmt,
            col,
            SQL_C_TIMESTAMP,
            &ret,
            sizeof(ret),
            &sqlPtr);

         statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

         // Mark this field as retrieved
         // Record whether this field is NULL
         if (sqlPtr == SQL_NULL_DATA) {
            return std::monostate{};
         }
         else
         {
            DateTime dt(ret.day, DateTime::Month(ret.month - 1), ret.year, ret.hour, ret.minute, ret.second, ret.fraction);
            return dt;
         }
      }
      break;
#endif // if 0

      default:
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

            statementDiagRecord(pStmt, rc).throwOnError(this);

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
               // TODO_JEFRI:
               // SQL_STILL_EXECUTING
               //statementDiagRecord(pStmt, rc).throwException(this);
               break;
            }
         }

         if (SQL_NO_DATA)
         {
            // TODO_JEFRI
         }

         // note colType=-3 (MySql VARBINARY)

         switch (colType)
         {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
               return strValue;
            case SQL_SMALLINT:
            case SQL_TINYINT:
               return std::stoi(strValue);
            case SQL_INTEGER:
               return std::stol(strValue);
            case SQL_BIGINT:
               return static_cast<int64_t>(std::stoll(strValue));
            case SQL_FLOAT:
            case SQL_REAL:
               return std::stof(strValue);
            case SQL_DOUBLE:
               return std::stod(strValue);
            case SQL_DATETIME:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_TYPE_DATE:
               // Note; https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-date-time/data-type-support-for-odbc-date-and-time-improvements?view=sql-server-ver15
            case -154:    // SQL_SS_TIME2	-154 (SQLNCLI.h)
            case SQL_TYPE_TIME:
            case -155:   // SQL_SS_TIMESTAMPOFFSET	-155 (SQLNCLI.h)
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
      break;
   }
}

bool OdbcConnection::getColumns(std::vector<std::string>& columnNames, const std::string& table)
{
   SQLRETURN rc;
   SQLHSTMT  pStmt = allocateStatement();

   auto tableStr  = tbs::util::utf8_to_odbcString(table);

   rc = SQLColumns(
         pStmt,                                // StatementHandle
         NULL, 0,                              // CatalogName
         NULL, 0,                              // SchemaName
         (SQLTCHAR*)tableStr.c_str(), SQL_NTS, // TableName
         NULL, 0);                             // ColumnName

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   // Loop through the rows in the result-set
   while (true)
   {
      rc = SQLFetch(pStmt);
      statementDiagRecord(pStmt, rc).throwOnError(this);

      if (SQL_SUCCEEDED(rc))
      {
         SQLPOINTER buff[8192] = {0};
         SQLINTEGER colSize    = sizeof(buff) / (sizeof(SQLTCHAR));
         SQLINTEGER realSize   = 0;
         int        nField     = 4;

         SQLRETURN retCode =
            SQLGetData(
                  pStmt,               // SQLHSTMT       StatementHandle
                  nField,              // SQLUSMALLINT   Col_or_Param_Num
                  SQL_C_TCHAR,         // SQLSMALLINT    TargetType,
                  buff,                // SQLPOINTER     TargetValuePtr
                  colSize,             // SQLLEN         BufferLength
                  (SQLLEN*)&realSize); // SQLLEN *       StrLen_or_IndPtr

         statementDiagRecord(pStmt, retCode).throwOnNotSucceeded(this);

         columnNames.emplace_back((const char*) buff);
      }
      else
         break;
   }

   SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
   return true;
}

bool OdbcConnection::tableOrViewExists(const std::string& tableName, bool checkTable)
{
   bool output           = false;
   std::string tableType = checkTable ? "TABLE" : "VIEW";
   auto tableNameStr     = tbs::util::utf8_to_odbcString(tableName);
   auto tableTypeStr     = tbs::util::utf8_to_odbcString(tableType);

   SQLRETURN  rc;
   SQLHSTMT pStmt = allocateStatement();

   rc = SQLTables(
         pStmt,                                      // StatementHandle
         NULL, 0,                                    // CatalogName
         NULL, 0,                                    // SchemaName
         (SQLTCHAR*)tableNameStr.c_str(), SQL_NTS,   // TableName
         (SQLTCHAR*)tableTypeStr.c_str(), SQL_NTS);  // TableType

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   rc = SQLFetch(pStmt);
   statementDiagRecord(pStmt, rc).throwOnError(this);

   if (SQL_SUCCEEDED(rc))
      output = true;

   SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
   return output;
}

bool OdbcConnection::isPrimaryKey(const std::string& columnName, const std::string& tableName)
{
   SQLRETURN      rc;
   SQLHSTMT pStmt = allocateStatement();
   auto tableStr  = tbs::util::utf8_to_odbcString(tableName);

// Note:
// Make sure we allocate enough column name len
// otherwise we got runtime exception: Stack around the variable 'szPkCol' was corrupted.
//const int TAB_LEN = SQL_MAX_TABLE_NAME_LEN + 100;
const int COL_LEN = SQL_MAX_COLUMN_NAME_LEN + 100;

   SQLTCHAR szPkCol[COL_LEN] = {0};  // Primary key column
   SQLLEN   cbPkCol;

   rc = SQLBindCol(pStmt,
         4,
         SQL_C_CHAR,
         szPkCol,
         COL_LEN,
         &cbPkCol);

   rc = SQLPrimaryKeys(
         pStmt,                                  // StatementHandle
         NULL, 0,                                // Catalog name
         NULL, 0,                                // Schema name
         (SQLTCHAR*)tableStr.c_str(), SQL_NTS);  // Table name

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   bool isPrimaryKey = false;

   // Loop through the rows in the result-set
   while (true)
   {
      rc = SQLFetch(pStmt);
      statementDiagRecord(pStmt, rc).throwOnError(this);

      if (SQL_SUCCEEDED(rc))
      {
         std::string pkColName = tbs::util::odbcString_to_utf8((const SQLTCHAR*)szPkCol);

         if (columnName == pkColName)
         {
            isPrimaryKey = true;
            break;
         }
      }
      else
         break;
   }

   SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
   pStmt = nullptr;

   return isPrimaryKey;
}

bool OdbcConnection::getTablesOrViews(std::vector<std::string>& objectNames, bool getTables)
{
   SQLRETURN rc;
   SQLHSTMT pStmt = allocateStatement();

   // ok do the job
   std::string tableType = getTables ? "TABLE" : "VIEW";
   onNotifyDebug(logId() + tbsfmt::format("getTablesOrViews: type is: {}", tableType));

   auto tableTypeStr = tbs::util::utf8_to_odbcString(tableType);

   rc = SQLTables(
         pStmt,                                       // StatementHandle
         NULL, 0,                                     // CatalogName
         NULL, 0,                                     // SchemaName
         NULL, 0,                                     // TableName
         (SQLTCHAR*)tableTypeStr.c_str(), SQL_NTS);   // TableType

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   while (true)
   {
      rc = SQLFetch(pStmt);
      statementDiagRecord(pStmt, rc).throwOnError(this);

      if (SQL_SUCCEEDED(rc))
      {
         SQLTCHAR   buff[8192] = { 0 };
         SQLINTEGER colSize    = sizeof(buff) / sizeof(SQLTCHAR);
         SQLINTEGER realSize   = 0;
         int        nField     = 3;

         SQLRETURN retCode =
            SQLGetData(
                  pStmt,               // SQLHSTMT       StatementHandle
                  nField,              // SQLUSMALLINT   Col_or_Param_Num
                  SQL_C_TCHAR,         // SQLSMALLINT    TargetType
                  buff,                // SQLPOINTER     TargetValuePtr
                  colSize,             // SQLLEN         BufferLength
                  (SQLLEN*)&realSize); // SQLLEN *       StrLen_or_IndPtr

         statementDiagRecord(pStmt, retCode).throwOnError(this);

         std::string strTable = tbs::util::odbcString_to_utf8((const SQLTCHAR*)buff);
         objectNames.push_back(strTable);
      }
      else
         break;
   }

   SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
   pStmt = nullptr;

   return true;
}

bool OdbcConnection::getPrimaryKeyColumns(std::vector<std::string>& primaryKeyCols, const std::string& tableName)
{
   SQLRETURN rc;
   SQLHSTMT pStmt = allocateStatement();
   auto tableStr  = tbs::util::utf8_to_odbcString(tableName);

   // Note:
   // Make sure we allocate enough column name len
   // otherwise we got runtime exception: Stack around the variable 'szPkTable' was corrupted.
   const int TAB_LEN = SQL_MAX_TABLE_NAME_LEN + 100;
   const int COL_LEN = SQL_MAX_COLUMN_NAME_LEN + 100;

   SQLTCHAR    szPkTable[TAB_LEN] = {0};     // Primary key table name
   UCHAR       szPkCol[COL_LEN]   = {0};     // Primary key column
   SQLLEN      cbPkTable, cbPkCol, cbKeySeq;
   SQLSMALLINT iKeySeq;

   rc = SQLBindCol(pStmt, 3, SQL_C_TCHAR,  szPkTable, sizeof(szPkTable) / sizeof(SQLTCHAR), &cbPkTable);
   rc = SQLBindCol(pStmt, 4, SQL_C_TCHAR,  szPkCol,   sizeof(szPkCol) / sizeof(SQLTCHAR),   &cbPkCol);
   rc = SQLBindCol(pStmt, 5, SQL_C_SSHORT, &iKeySeq,  0, &cbKeySeq);

   rc = SQLPrimaryKeys(
         pStmt,                                  // StatementHandle
         NULL, 0,                                // Catalog name
         NULL, 0,                                // Schema name
         (SQLTCHAR*)tableStr.c_str(), SQL_NTS);  // Table name

   statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);

   // Loop through the rows in the result-set
   while (true)
   {
      rc = SQLFetch(pStmt);
      statementDiagRecord(pStmt, rc).throwOnError(this);

      if (SQL_SUCCEEDED(rc))
      {
         std::string tableName      = tbs::util::odbcString_to_utf8((const SQLTCHAR*)szPkTable);
         //int       columnPosition = iKeySeq;
         std::string columnName     = tbs::util::odbcString_to_utf8((const SQLTCHAR*)szPkCol);
         primaryKeyCols.push_back(columnName);
      }
      else
         break;
   }

   SQLFreeHandle(SQL_HANDLE_STMT, pStmt);
   pStmt = nullptr;

   return true;
}

SQLRETURN OdbcConnection::sqlPutData(SQLHSTMT pStmt, const SqlParameterCollection& parameters)
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
      unsigned long paramPos = PtrToUlong(pParamPos);
#else
      unsigned long paramPos = (unsigned long)(unsigned long*) pParamPos;
#endif
      // we need SqlParameter object not OdbcParameter, because SqlParameter holds actual data
      auto& param = parameters[paramPos-1];

      onNotifyTrace(logId() + tbsfmt::format("sqlPutData: Processing data-at-execution parameter no: {} name: {}", paramPos, param->name() ));

      // the raw binary data pointer
      SQLLEN   lbytes = (SQLLEN) param->size();
      uint8_t* pBlob  = *(param->valueBytePtr());
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

            onNotifyTrace(logId() + tbsfmt::format("sqlPutData: parameter no: {} name: {}, bytes remaining {}:", paramPos, param->name(), lbytes ));

            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
               statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);
            }

            pBlob  += PUTDATA_BUFFER;
            lbytes -= PUTDATA_BUFFER;
         }

         // Put final segment
         rc = SQLPutData(pStmt, (SQLPOINTER)pBlob, lbytes);
         onNotifyTrace(tbsfmt::format("sqlPutData: parameter no: {} name: {}, final bytes {}:", paramPos, param->name(),lbytes ));
      }

      if ( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
         statementDiagRecord(pStmt, rc).throwOnNotSucceeded(this);
      }

      // ask for next parameter
      rc = SQLParamData(pStmt, &pParamPos);
   }

   return rc;
}

bool OdbcConnection::environmentAndConnectionHandleAlive()
{
    return (_pEnv != nullptr) && (_pDbc != nullptr);
}

void OdbcConnection::allocateEnvironmentAndConnectionHandle()
{
   SQLRETURN rc;

   // allocate SQLHENV - environment handle
   rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_pEnv);
   if (SQL_SUCCEEDED(rc))
   {
      // Set the ODBC version environment attribute; we want ODBC 3 support
      rc = SQLSetEnvAttr((SQLHENV)_pEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

      if (SQL_SUCCEEDED(rc))
      {
         // allocate SQLHDBC connection handle
         rc = SQLAllocHandle(SQL_HANDLE_DBC, (SQLHENV)_pEnv, &_pDbc);

         if (SQL_SUCCEEDED(rc))
         {
            // Set login timeout to 30 seconds
            SQLSetConnectAttr(_pDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)30, 0);
         }
         else
         {
            OdbcDiagRecord diag(_pEnv, SQL_HANDLE_ENV, rc);
            onNotifyError(logId() + diag.message());
            SQLFreeHandle(SQL_HANDLE_ENV, _pEnv);
            _pDbc = nullptr;
            _pEnv = nullptr;
            throw tbs::SqlException("Could not allocate ODBC connection handle", "OdbcConnection");
         }
      }
      else
      {
         OdbcDiagRecord diag(_pEnv, SQL_HANDLE_ENV, rc);
         onNotifyError(logId() + diag.message());
         SQLFreeHandle(SQL_HANDLE_ENV, _pEnv);
         _pEnv = nullptr;
         throw tbs::SqlException("Could not get ODBC v3 support", "OdbcConnection");
      }
   }
   else
   {
      OdbcDiagRecord diag(_pEnv, SQL_HANDLE_ENV, rc);
      onNotifyError(logId() + diag.message());
      SQLFreeHandle(SQL_HANDLE_ENV, _pEnv);
      _pEnv = nullptr;
      throw tbs::SqlException("Could not allocate ODBC environment handle", "OdbcConnection");
   }
}

std::string OdbcConnection::getInfo(short infoType)
{
   SQLTCHAR    value[1024] = {0};
   SQLSMALLINT length(0);
   RETCODE     rc;
   SQLSMALLINT valueLen = sizeof(value) / sizeof(SQLTCHAR);

   rc = SQLGetInfo(
           _pDbc,      // SQLHDBC         ConnectionHandle,
           infoType,   // SQLUSMALLINT    InfoType,
           value,      // SQLPOINTER      InfoValuePtr,
           valueLen,   // SQLSMALLINT     BufferLength,
           &length);   // SQLSMALLINT *   StringLengthPtr

   if (rc == SQL_SUCCESS_WITH_INFO)
   {
      OdbcDiagRecord diag(_pDbc, SQL_HANDLE_DBC, rc);
      onNotifyTrace(logId() + tbsfmt::format("state:{} message:{}", diag.state(), diag.message()));
   }

   connectionDiagRecord(_pDbc, rc).throwOnNotSucceeded(this);

   std::string result = tbs::util::odbcString_to_utf8(value);
   return result;
}

} // namespace sql
} // namespace tbs