#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#import "c:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "EndOfFile")

#include "tobasasql/common_types.h"

namespace tbs {
namespace util {

/// convert std::string (UTF-8 by default) to _bstr_t (wchar_t Unicode)
_bstr_t utf8_to_bstr_t(const std::string& str);

/// create std::string (UTF-8 by default) from _bstr_t (wchar_t Unicode)
std::string utf8_from_bstr_t(const _bstr_t& str);

} // namespace util

/** \addtogroup SQL
   @{
 */

/**
 * COM Error
 */
struct ComError
{
   ComError(_com_error& e, const char* fl="", int ln=0);

   std::string fullMessage;
   std::string message;
   std::string source;
   std::string description;
   std::string file;
   int line;
};

#define EXTRACT_COM_ERROR(com_error, comError) extractComError(com_error, comError, __FILE__, __LINE__)

/// Extract COM error
void extractComError(_com_error& e, ComError& comError, const char* file="", int line=0);

/** @}*/

namespace sql {

/** \addtogroup SQL
 * @{
 */

 /// Convert ADODB::DataTypeEnum from std::string
ADODB::DataTypeEnum adoDataTypeFromString(const std::string& ftype);

/// Convert ADODB::DataTypeEnum to std::string
std::string adoDataTypeToString(ADODB::DataTypeEnum type);

/// Convert sql::DataType to ADODB::DataTypeEnum.
ADODB::DataTypeEnum adoDataTypeFromDataType(DataType type);

/// Convert ADODB::DataTypeEnum to sql::DataType.
DataType adoDataTypeToDataType(ADODB::DataTypeEnum ftype);

/// Convert sql::ParameterDirection to ADODB::ParameterDirectionEnum.
ADODB::ParameterDirectionEnum adoParamDirectionFromParamDirection(ParameterDirection direction);

/// Get Type class from Adodb data type.
TypeClass typeClassFromAdodbType(const long type);

/** @}*/

} // namespace util
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)