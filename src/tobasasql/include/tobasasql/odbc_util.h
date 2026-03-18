#pragma once

#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
#endif

#include <string>
#include <sql.h>
#include <sqlext.h>
#include "tobasasql/common_types.h"

namespace tbs {
namespace util {

/// convert UTF-8 string to odbc string
#if defined(_WIN32)
std::wstring utf8_to_odbcString(const std::string& str);
#else
std::string utf8_to_odbcString(const std::string& str);
#endif

/// convert odbc string to UTF-8 string
#if defined(_WIN32)
std::string odbcString_to_utf8(const std::wstring& str);
#else
std::string odbcString_to_utf8(const std::string& str);
#endif

#if !defined(_WIN32)
std::string odbcString_to_utf8(const SQLTCHAR* sqlchar);
#endif

} //namespace util

/// Forward decalaration
struct Notifier;

/**
 * ODBC Diag record
 */
struct OdbcDiagRecord
{
public:
   OdbcDiagRecord();
   OdbcDiagRecord(
      SQLHANDLE   handle,
      SQLSMALLINT handleType,
      RETCODE     retCode,
      std::string source="",
      const char* fl = "",
      int         ln = 0);

   void throwException(const Notifier* notifier=nullptr);
   void throwOnError(const Notifier* notifier=nullptr);
   void throwOnNotSucceeded(const Notifier* notifier=nullptr);

   int code() const;
   std::string message() const;
   std::string state() const;

   // Note: https://docs.microsoft.com/en-us/sql/connect/odbc/cpp-code-example-app-connect-access-sql-db?view=sql-server-ver15

   void extractDiagRecord(
      SQLHANDLE   handle,
      SQLSMALLINT handleType,
      SQLRETURN   retCode);

private:
   static const int LEN_MSG   = 1024;
   static const int LEN_STATE = SQL_SQLSTATE_SIZE + 1;

   SQLHANDLE   _handle;
   SQLSMALLINT _handleType;
   RETCODE     _returnCode;
   std::string _source;
   std::string _file;
   int         _line;

   int         _diagCode;
   std::string _diagMessage;
   std::string _diagState;
};

OdbcDiagRecord statementDiagRecord(
   SQLHSTMT    hanlde,
   RETCODE     retCode,
   std::string source="",
   const char* fl = "",
   int         ln = 0);

OdbcDiagRecord connectionDiagRecord(
   SQLHDBC     hanlde,
   RETCODE     retCode,
   std::string source="",
   const char* fl = "",
   int         ln = 0);

OdbcDiagRecord environmentDiagRecord(
   SQLHDBC     hanlde,
   RETCODE     retCode,
   std::string source="",
   const char* fl = "",
   int         ln = 0);


namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Get ODBC type from string.
short odbcDataTypeFromString(std::string type);

/// Convert ODBC data type to string.
std::string odbcDataTypeToString(short type);

 /// Convert sql::DataType from ODBC type.
short odbcTypeFromDataType(DataType type);

/// Convert ODBC type to sql::DataType.
DataType odbcTypeToDataType(short type);

/// Convert sql::ParameterDirection to ODBC Parameter Direction.
int odbcParamDirectionFromParamDirection(ParameterDirection direction);

/// Get Type class from  ODBC type.
TypeClass typeClassFromOdbcType(const long type);

/** @}*/

} // namespace sql
} // namespace tbs