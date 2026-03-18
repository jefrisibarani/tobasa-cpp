#include <cstring>
#include <tobasa/format.h>
#include <tobasa/notifier.h>
#include <tobasa/util_utf.h>
#include "tobasasql/exception.h"
#include "tobasasql/odbc_util.h"

namespace tbs {
namespace util {

//! convert UTF-8 string to odbc string
#if defined(_WIN32)
std::wstring utf8_to_odbcString(const std::string& str)
#else
std::string utf8_to_odbcString(const std::string& str)
#endif
{
#if defined(_WIN32)
   return utf8_to_wstring(str);
#else
   return str;
#endif
}


//! convert odbc string to UTF-8 string
#if defined(_WIN32)
std::string odbcString_to_utf8(const std::wstring& str)
#else
std::string odbcString_to_utf8(const std::string& str)
#endif
{
#if defined(_WIN32)
   return wstring_to_utf8(str);
#else
   return str;
#endif
}

#if !defined(_WIN32)
std::string odbcString_to_utf8(const SQLTCHAR* sqlchar)
{
   std::string res{(char*)sqlchar};
   return res;
   /*
   //auto strLen = (sizeof(sqlchar) / sizeof(SQLTCHAR));
   auto strLen = (sizeof(sqlchar) / sizeof((sqlchar)[0]) );
   std::string res;
   res.reserve(strLen);
   std::copy(sqlchar, sqlchar + strLen, res.begin());
   return res;
   */
}
#endif

} //namespace util


OdbcDiagRecord::OdbcDiagRecord()
{
   _handle      = nullptr;
   _handleType  = 0;
   _returnCode  = 0;
   _source      = "";
   _file        = "";
   _line        = 0;
   _diagCode    = 0;
   _diagMessage = "";
   _diagState   = "";
}

OdbcDiagRecord::OdbcDiagRecord(
   SQLHANDLE   handle,
   SQLSMALLINT handleType,
   RETCODE     retCode,
   std::string source,
   const char* fl,
   int         ln)
{
   _handle      = handle;
   _handleType  = handleType;
   _returnCode  = retCode;
   _source      = source;
   _diagCode    = 0;
   _diagMessage = "";
   _diagState   = "";

   extractDiagRecord(handle, handleType, retCode);

   std::string msgori_ = _diagMessage;

   if (strlen(fl)>0 && ln != 0)
   {
      _diagMessage = tbsfmt::format("{}, File: {}, Line: {}", msgori_, fl, ln);
      _file        = fl;
      _line        = ln;
   }
}

void OdbcDiagRecord::throwException(const Notifier* notifier)
{
   if (_handleType == SQL_HANDLE_STMT)
      SQLFreeHandle(_handleType, _handle);
   else if (_handleType == SQL_HANDLE_DBC) { }
   else if(_handleType == SQL_HANDLE_DBC) { }

   throw tbs::SqlException(_diagMessage, _source);
}

void OdbcDiagRecord::throwOnError(const Notifier* notifier)
{
   if (_returnCode == SQL_ERROR)
      throwException(notifier);
}

void OdbcDiagRecord::throwOnNotSucceeded(const Notifier* notifier)
{
   if (!SQL_SUCCEEDED(_returnCode))
      throwException(notifier);
}

int OdbcDiagRecord::code() const { return _diagCode; }

std::string OdbcDiagRecord::message() const { return _diagMessage; }

std::string OdbcDiagRecord::state() const { return _diagState; }

// Note: https://docs.microsoft.com/en-us/sql/connect/odbc/cpp-code-example-app-connect-access-sql-db?view=sql-server-ver15

void OdbcDiagRecord::extractDiagRecord(
   SQLHANDLE   handle,
   SQLSMALLINT handleType,
   SQLRETURN   retCode)
{
   if (retCode == SQL_INVALID_HANDLE)
   {
      _diagMessage   = "Invalid handle!";
      return;
   }

   SQLRETURN   rc;
   SQLINTEGER  iNativeCode;
   SQLTCHAR    strState[LEN_STATE] = {};
   SQLTCHAR    strMessage[LEN_MSG] = {};
   SQLSMALLINT iMsgLen;
   SQLSMALLINT strMessageLen = (SQLSMALLINT)(sizeof(strMessage) / sizeof(SQLTCHAR));

   rc = SQLGetDiagRec(
         handleType,
         handle,
         1,
         strState,
         &iNativeCode,
         strMessage,
         strMessageLen,
         &iMsgLen);

   if (SQL_SUCCEEDED(rc))
   {
      _diagCode    = (int)iNativeCode;
      _diagMessage = tbs::util::odbcString_to_utf8(strMessage);
      _diagState   = tbs::util::odbcString_to_utf8(strState);
   }
   else if(rc == SQL_INVALID_HANDLE)
   {
      _diagMessage = "Invalid handle!";
      _diagCode    = 0;
      _diagState   = "";
   }
   else
   {
      std::string hType = "";
      if (handleType == SQL_HANDLE_STMT)
         hType = "Statement";
      else if (handleType == SQL_HANDLE_DBC)
         hType = "Connection";
      else if (handleType == SQL_HANDLE_ENV)
         hType = "Environment";
      else
         hType = "Unknown";

      // SQL_NO_DATA, SQL_ERROR
      _diagMessage = tbsfmt::format("Could not get DiagnosticRecord for handle type: {} returnCode: {}", hType, retCode);
      _diagCode    = 0;
      _diagState   = "";
   }
}

OdbcDiagRecord statementDiagRecord(
   SQLHSTMT    hanlde,
   RETCODE     retCode,
   std::string source,
   const char* fl,
   int         ln)
{
   return OdbcDiagRecord(hanlde, SQL_HANDLE_STMT, retCode, source, fl, ln);
}

OdbcDiagRecord connectionDiagRecord(
   SQLHDBC     hanlde,
   RETCODE     retCode,
   std::string source,
   const char* fl,
   int         ln)
{
   return OdbcDiagRecord(hanlde, SQL_HANDLE_DBC, retCode, source, fl, ln);
}

OdbcDiagRecord environmentDiagRecord(
   SQLHDBC     hanlde,
   RETCODE     retCode,
   std::string source,
   const char* fl,
   int         ln)
{
   return OdbcDiagRecord(hanlde, SQL_HANDLE_ENV, retCode, source, fl, ln);
}


namespace sql {

short odbcDataTypeFromString(std::string type)
{
   if (type == "SmallInt")
      return SQL_SMALLINT;
   else if (type == "Integer")
      return SQL_INTEGER;
   else if (type == "TinyInt")
      return SQL_TINYINT;
   else if (type == "BigInt")
      return SQL_BIGINT;
   else if (type == "Decimal")
      return SQL_DECIMAL;
   else if (type == "Numeric")
      return SQL_NUMERIC;
   else if (type == "Real")
      return SQL_REAL;
   else if (type == "Float")
      return SQL_FLOAT;
   else if (type == "Double")
      return SQL_DOUBLE;
   else if (type == "Char")
      return SQL_CHAR;
   else if (type == "Varchar")
      return SQL_VARCHAR;
   else if (type == "LongVarChar")
      return SQL_LONGVARCHAR;
   else if (type == "WChar")
      return SQL_WCHAR;
   else if (type == "WVarchar")
      return SQL_WVARCHAR;
   else if (type == "WLongVarChar")
      return SQL_WLONGVARCHAR;
   else if (type == "Bit")
      return SQL_BIT;
   else if (type == "Binary")
      return SQL_BINARY;
   else if (type == "VarBinary")
      return SQL_VARBINARY;
   else if (type == "LongVarBinary")
      return SQL_LONGVARBINARY;
   else if (type == "Date")
      return SQL_TYPE_DATE;
   else if (type == "Time")
      return SQL_TYPE_TIME;
   else if (type == "TimeStamp")
      return SQL_TYPE_TIMESTAMP;
   else
      throw TypeException(tbsfmt::format("Invalid ODBC data type conversion from string: {}", type), "OdbcUtil");
}

std::string odbcDataTypeToString(short type)
{
   std::string retVal;

   switch (type)
   {
   case SQL_SMALLINT:
      return "SmallInt";
   case SQL_INTEGER:
      return "Integer";
   case SQL_TINYINT:
      return "TinyInt";
   case SQL_BIGINT:
      return "BigInt";
   case SQL_DECIMAL:
      return "Decimal";
   case SQL_NUMERIC:
      return "Numeric";
   case SQL_REAL:
      return "Real";
   case SQL_FLOAT:
      return "Float";
   case SQL_DOUBLE:
      return "Double";
   case SQL_CHAR:
      return "Char";
   case SQL_VARCHAR:
      return "Varchar";
   case SQL_LONGVARCHAR:
      return "LongVarChar";
   case SQL_WCHAR:
      return "WChar";
   case SQL_WVARCHAR:
      return "WVarchar";
   case SQL_WLONGVARCHAR:
      return "WLongVarChar";
   case SQL_BIT:
      return "Bit";
   case SQL_BINARY:
      return "Binary";
   case SQL_VARBINARY:
      return "VarBinary";
   case SQL_LONGVARBINARY:
      return "LongVarBinary";
   case SQL_TYPE_DATE:
      return "Date";
   // Note; https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-date-time/data-type-support-for-odbc-date-and-time-improvements?view=sql-server-ver15
   case -154:    // SQL_SS_TIME2	-154 (SQLNCLI.h)
   case SQL_TYPE_TIME:
      return "Time";
   case -155:   // SQL_SS_TIMESTAMPOFFSET	-155 (SQLNCLI.h)
   case SQL_TYPE_TIMESTAMP:
      return "TimeStamp";
   default:
      throw TypeException(tbsfmt::format("Invalid ODBC data type conversion to string: {}", type), "OdbcUtil");
   }

   return retVal;
}

short odbcTypeFromDataType(DataType type)
{
   short retVal;

   switch (type)
   {
   case DataType::tinyint:
      return SQL_TINYINT;
   case DataType::smallint:
      return SQL_SMALLINT;
   case DataType::integer:
      return SQL_INTEGER;
   case DataType::bigint:
      return SQL_BIGINT;
   case DataType::numeric:
      return SQL_NUMERIC;
   case DataType::float4:
      return SQL_REAL; // or SQL_FLOAT?
   case DataType::float8:
      return SQL_DOUBLE;
   case DataType::boolean:
      return SQL_BIT;
   case DataType::character:
      return SQL_CHAR;
   case DataType::varchar:
   case DataType::text:
      return SQL_WVARCHAR;
   case DataType::date:
      return SQL_TYPE_DATE;
   case DataType::time:
      return SQL_TYPE_TIME;
   case DataType::timestamp:
      return SQL_TYPE_TIMESTAMP;
   case DataType::varbinary:
      return SQL_VARBINARY;
   default:
      throw TypeException(tbsfmt::format("Invalid ODBC data type conversion from Tobasa data type: {}", dataTypeToString(type)), "OdbcUtil");
   }

   return retVal;
}

DataType odbcTypeToDataType(short type)
{
   DataType retVal;

   switch (type)
   {
   case SQL_TINYINT:
      return DataType::tinyint;
   case SQL_SMALLINT:
      return DataType::smallint;
   case SQL_INTEGER:
      return DataType::integer;
   case SQL_BIGINT:
      return DataType::bigint;
   case SQL_DECIMAL:
   case SQL_NUMERIC:
      return DataType::numeric;
   case SQL_REAL:                   // float(24) precission 0 to 24
   case SQL_FLOAT:                  // float(24) precission 0 to 24
      return DataType::float4;
   case SQL_DOUBLE:                 // float(53) precission 25 to 53 or double precision
      return DataType::float8;
   case SQL_BIT:
      return DataType::boolean;
   case SQL_CHAR:
      return DataType::character;
   case SQL_VARCHAR:
   case SQL_WCHAR:
   case SQL_WVARCHAR:
      return DataType::varchar;
   case SQL_LONGVARCHAR:
   case SQL_WLONGVARCHAR:
      return DataType::text;
   case SQL_BINARY:
   case SQL_VARBINARY:
   case SQL_LONGVARBINARY:
      return DataType::varbinary;
   case SQL_TYPE_DATE:
      return DataType::date;
      // Note; https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-date-time/data-type-support-for-odbc-date-and-time-improvements?view=sql-server-ver15
   case -154:    // SQL_SS_TIME2	-154 (SQLNCLI.h)
   case SQL_TYPE_TIME:
      return DataType::time;
   case -155:   // SQL_SS_TIMESTAMPOFFSET	-155 (SQLNCLI.h)
   case SQL_TYPE_TIMESTAMP:
      return DataType::timestamp;
   default:
      throw TypeException(tbsfmt::format("Invalid ODBC data type conversion to Tobasa data type: {}", type), "OdbcUtil");
   }

   return retVal;
}

int odbcParamDirectionFromParamDirection(ParameterDirection direction)
{
   int retVal;

   switch (direction)
   {
   case ParameterDirection::input:
      return SQL_PARAM_INPUT;
   case ParameterDirection::output:
      return SQL_PARAM_OUTPUT;
   case ParameterDirection::inputOutput:
      return SQL_PARAM_INPUT_OUTPUT;
   case ParameterDirection::returnValue:
      return SQL_RETURN_VALUE;
   default:
      throw TypeException("Invalid ODBC parameter direction conversion", "OdbcUtil");
   }

   return retVal;
}

TypeClass typeClassFromOdbcType(const long type)
{
   TypeClass retVal;

   switch (type)
   {
   case SQL_TINYINT:
   case SQL_SMALLINT:
   case SQL_INTEGER:
   case SQL_BIGINT:
   case SQL_DECIMAL:
   case SQL_NUMERIC:
   case SQL_REAL:
   case SQL_FLOAT:
   case SQL_DOUBLE:
      return TypeClass::numeric;
   case SQL_BIT:
      return TypeClass::boolean;
   case SQL_CHAR:
   case SQL_VARCHAR:
   case SQL_WCHAR:
   case SQL_WVARCHAR:
   case SQL_LONGVARCHAR:
   case SQL_WLONGVARCHAR:
      return TypeClass::string;
   case SQL_BINARY:
   case SQL_VARBINARY:
   case SQL_LONGVARBINARY:
      return TypeClass::blob;
      // Note; https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-date-time/data-type-support-for-odbc-date-and-time-improvements?view=sql-server-ver15
   case -154:    // SQL_SS_TIME2	-154 (SQLNCLI.h)
   case -155:    // SQL_SS_TIMESTAMPOFFSET	-155 (SQLNCLI.h)
   case SQL_TYPE_DATE:
   case SQL_TYPE_TIME:
   case SQL_TYPE_TIMESTAMP:
      return TypeClass::date;
   default:
      throw TypeException(tbsfmt::format("Invalid ODBC data type conversion to TypeClass: {}", type), "OdbcUtil");
   }

   return retVal;
}

} // namespace sql
} // namespace tbs