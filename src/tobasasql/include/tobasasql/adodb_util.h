#pragma once

#include "tobasasql/exp_imp.h"

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#import "c:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "EndOfFile")

#include "tobasasql/adodb_types.h"
#include "tobasasql/common_types.h"
#include "tobasasql/com_variant.h"

namespace tbs {

namespace sql {

/** \addtogroup SQL
 * @{
 */

 /// Convert ADODB::DataTypeEnum from std::string
TBS_SQL_API ADODB::DataTypeEnum adoDataTypeFromString(const std::string& ftype);

/// Convert ADODB::DataTypeEnum to std::string
TBS_SQL_API std::string adoDataTypeToString(ADODB::DataTypeEnum type);

/// Convert sql::DataType to ADODB::DataTypeEnum.
TBS_SQL_API ADODB::DataTypeEnum adoDataTypeFromDataType(DataType type);

/// Convert ADODB::DataTypeEnum to sql::DataType.
TBS_SQL_API DataType adoDataTypeToDataType(ADODB::DataTypeEnum ftype);

/// Convert sql::ParameterDirection to ADODB::ParameterDirectionEnum.
TBS_SQL_API ADODB::ParameterDirectionEnum adoParamDirectionFromParamDirection(ParameterDirection direction);

/// Get Type class from Adodb data type.
TBS_SQL_API TypeClass typeClassFromAdodbType(const long type);

/// convert std::string (UTF-8 by default) to _bstr_t (wchar_t Unicode)
TBS_SQL_API _bstr_t utf8_to_bstr_t(const std::string& str);

/// create std::string (UTF-8 by default) from _bstr_t (wchar_t Unicode)
TBS_SQL_API std::string utf8_from_bstr_t(const _bstr_t& str);


TBS_SQL_API std::string adoDbTime2ToString(const sql::AdoDbTime2& time);

TBS_SQL_API std::string adoDbTime2VariantToString(const tbs::ComVariantType& variantVal);

TBS_SQL_API std::string adoDbTime2BytesToString(const std::vector<uint8_t>& data);

TBS_SQL_API std::string decimalToString(const DECIMAL& value);

/** @}*/
} // namespace sql ------------------------------------------------------------------


// ----------------------------------------------------------------------------------
/** \addtogroup SQL
   @{
 */


/// COM Error
struct TBS_SQL_API ComError
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
TBS_SQL_API void extractComError(_com_error& e, ComError& comError, const char* file="", int line=0);

/** @}*/
// ----------------------------------------------------------------------------------


} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)