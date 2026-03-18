#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/format.h>
#include <tobasa/util_string.h>
#include <tobasa/util_utf.h>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/adodb_util.h"

/*
Note: 
+ -------------------------+-------------------------+-------------------+-------------------------------+-------------------------------------------------+
| SQL Server Data Type     | OLE DB/ADO DataTypeEnum | COM VARIANT Type  | C / C++ Equivalent            | Notes                                           |
+ -------------------------+ ------------------------+ ------------------+ ------------------------------+ ------------------------------------------------+
| bit                      | adBoolean               | VT_BOOL           | VARIANT_BOOL (TRUE/FALSE)     | Stored as 16-bit VARIANT_BOOL (-1 = TRUE).      |
| tinyint                  | adUnsignedTinyInt       | VT_UI1            | unsigned char, uint8_t        | Unsigned 8-bit integer.                         |
| smallint                 | adSmallInt              | VT_I2             | short, int16_t                | Signed 16-bit integer.                          |
|                          |                         |                   |                               |                                                 |
| int / integer            | adInteger               | **VT_I4**         | long, int32_t                 | Standard 32-bit integer.                        |
|                          |                         |                   |                               |    (VT_INT also works, but VT_I4 preferred.)    |
|                          |                         |                   |                               |                                                 |
| bigint                   | adBigInt                | VT_I8             | long long, int64_t            | Signed 64-bit integer.                          |
|                          |                         |                   |                               |                                                 |
| decimal, numeric         | adDecimal / adNumeric   | VT_DECIMAL        | DECIMAL,                      | BCD (binary-coded decimal).                     |
|                          |                         |                   | _variant_t handles conversion |                                                 |
|                          |                         |                   |                               |                                                 | 
| money, smallmoney        | adCurrency              | VT_CY             | CY (scaled 10,000)            | 64-bit integer scaled by 10,000.                |
| float                    | adDouble                | VT_R8             | double                        | 64-bit float.                                   |
| real                     | adSingle                | VT_R4             | float                         | 32-bit float.                                   |
|                          |                         |                   |                               |                                                 |
| char, varchar, nchar,    | adBSTR / adVarWChar     | VT_BSTR           | BSTR                          | Unicode string.                                 |
|    nvarchar, text, ntext |                         |                   |                               |                                                 |
|                          |                         |                   |                               |                                                 |
| binary, varbinary, image | adVarBinary             | VT_ARRAY          | VT_UI1                        | SAFEARRAY of BYTE                 Byte arrays.  |
| uniqueidentifier         | adGUID                  | VT_CLSID          | GUID                          | 16-byte GUID struct.                            |
|                          |                         |                   |                               |                                                 |
| datetime, datetime2      | adDate                  | VT_DATE           | DATE (OLE DATE = double)      | Days since 1899-12-30,                          |
|   smalldatetime,         |                         |                   |                               |   fractional part = time.                       |
|                          |                         |                   |                               |                                                 |
| time                     | adDBTime                | VT_DATE / VT_BSTR | DATE or BSTR                  | Stored as fractional day or text.               |
| date                     | adDBDate                | VT_DATE / VT_BSTR | DATE or BSTR                  | ADO often uses VT_DATE.                         |
| datetimeoffset           | adDBTimeStampOffset     | VT_BSTR           | BSTR                          | ADO returns as string.                          |
| sql_variant              | adVariant               | VT_VARIANT        | VARIANT                       | Wrapper around nested variant.                  |
+ -------------------------+---------------------------+-----------------+-------------------------------+-------------------------------------------------+

*/

namespace tbs {
namespace util {

_bstr_t utf8_to_bstr_t(const std::string& str)
{
   std::wstring strW = utf8_to_wstring(str);
   _bstr_t strBW((wchar_t*)strW.c_str());

   return strBW;
}

std::string utf8_from_bstr_t(const _bstr_t& str)
{
   std::string strC = wstring_to_utf8((wchar_t*)str);
   return strC;
}

} // namespace util

ComError::ComError(_com_error& e, const char* fl, int ln)
{
    _bstr_t errMsg = (_bstr_t) e.ErrorMessage();
    _bstr_t errSrc = (_bstr_t) e.Source();
    _bstr_t errDes = (_bstr_t) e.Description();

    errMsg = errMsg.length() == 0 ? _bstr_t("") : errMsg;
    errSrc = errSrc.length() == 0 ? _bstr_t("") : errSrc;
    errDes = errDes.length() == 0 ? _bstr_t("") : errDes;

    std::string fullMessage_;
    if (strlen(fl)>0  && ln != 0)
    {
        fullMessage_ =
        tbsfmt::format("Error Code: {:08x}, Message: {}, Source: {}, Description: {}, File: {}, Line: {}",
            static_cast<unsigned>(e.Error()),
            (char*) errMsg,
            (char*) errSrc,
            (char*) errDes,
            fl,
            ln);

        file = fl;
        line = ln;
    }
    else
    {
        fullMessage_ =
        tbsfmt::format("Error Code: {:08x}, Message: {}, Source: {}, Description: {}",
            static_cast<unsigned>(e.Error()),
            (char*) errMsg,
            (char*) errSrc,
            (char*) errDes);
    }

    fullMessage = fullMessage_;
    message     = (char*) errMsg;
    source      = (char*) errSrc;
    description = (char*) errDes;
}

 void extractComError(_com_error& e, ComError& comError, const char* file, int line)
{
   _bstr_t errMsg = (_bstr_t)e.ErrorMessage();
   _bstr_t errSrc = (_bstr_t)e.Source();
   _bstr_t errDes = (_bstr_t)e.Description();

   errMsg = errMsg.length() == 0 ? _bstr_t("") : errMsg;
   errSrc = errSrc.length() == 0 ? _bstr_t("") : errSrc;
   errDes = errDes.length() == 0 ? _bstr_t("") : errDes;

   std::string fullMessage;
   if (strlen(file)>0 && line != 0)
   {
      fullMessage =
         tbsfmt::format("Error Code: {:08x}, Message: {}, Source: {}, Description: {}, File: {}, Line: {}",
            static_cast<unsigned>(e.Error()),
            (char*)errMsg,
            (char*)errSrc,
            (char*)errDes,
            file,
            line);

      comError.file = file;
      comError.line = line;
   }
   else
   {
      fullMessage =
         tbsfmt::format("Error Code: {:08x}, Message: {}, Source: {}, Description: {}",
            static_cast<unsigned>(e.Error()),
            (char*)errMsg,
            (char*)errSrc,
            (char*)errDes);
   }

   comError.fullMessage = fullMessage;
   comError.message     = (char*)errMsg;
   comError.source      = (char*)errSrc;
   comError.description = (char*)errDes;
}

namespace sql {

ADODB::DataTypeEnum adoDataTypeFromString(const std::string& ftype)
{
   if (ftype == "adTinyInt")
      return ADODB::adTinyInt;
   else if (ftype == "adSmallInt")
      return ADODB::adSmallInt;
   else if (ftype == "adInteger")
      return ADODB::adInteger;
   else if (ftype == "adBigInt")
      return ADODB::adBigInt;
   else if (ftype == "adUnsignedTinyInt")
      return ADODB::adUnsignedTinyInt;
   else if (ftype == "adUnsignedSmallInt")
      return ADODB::adUnsignedSmallInt;
   else if (ftype == "adUnsignedInt")
      return ADODB::adUnsignedInt;
   else if (ftype == "adUnsignedBigInt")
      return ADODB::adUnsignedBigInt;
   else if (ftype == "adSingle")
      return ADODB::adSingle;
   else if (ftype == "adDouble")
      return ADODB::adDouble;
   else if (ftype == "adCurrency")
      return ADODB::adCurrency;
   else if (ftype == "adDecimal")
      return ADODB::adDecimal;
   else if (ftype == "adNumeric")
      return ADODB::adNumeric;
   else if (ftype == "adBoolean")
      return ADODB::adBoolean;
   else if (ftype == "adError")
      return ADODB::adError;
   else if (ftype == "adUserDefined")
      return ADODB::adUserDefined;
   else if (ftype == "adVariant")
      return ADODB::adVariant;
   else if (ftype == "adIDispatch")
      return ADODB::adIDispatch;
   else if (ftype == "adIUnknown")
      return ADODB::adIUnknown;
   else if (ftype == "adGUID")
      return ADODB::adGUID;
   else if (ftype == "adDate")
      return ADODB::adDate;
   else if (ftype == "adDBDate")
      return ADODB::adDBDate;
   else if (ftype == "adDBTime")
      return ADODB::adDBTime;
   else if (ftype == "adDBTimeStamp")
      return ADODB::adDBTimeStamp;
   else if (ftype == "adBSTR")
      return ADODB::adBSTR;
   else if (ftype == "adChar")
      return ADODB::adChar;
   else if (ftype == "adVarChar")
      return ADODB::adVarChar;
   else if (ftype == "adLongVarChar")
      return ADODB::adLongVarChar;
   else if (ftype == "adWChar")
      return ADODB::adWChar;
   else if (ftype == "adVarWChar")
      return ADODB::adVarWChar;
   else if (ftype == "adLongVarWChar")
      return ADODB::adLongVarWChar;
   else if (ftype == "adBinary")
      return ADODB::adBinary;
   else if (ftype == "adVarBinary")
      return ADODB::adVarBinary;
   else if (ftype == "adLongVarBinary")
      return ADODB::adLongVarBinary;
   else if (ftype == "adChapter")
      return ADODB::adChapter;
   else if (ftype == "adFileTime")
      return ADODB::adFileTime;
   else if (ftype == "adPropVariant")
      return ADODB::adPropVariant;
   else if (ftype == "adVarNumeric")
      return ADODB::adVarNumeric;
   else if (ftype == "adArray")
      return ADODB::adArray;
   
   // Note: 
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   // Sql server 2008 and Up, "time" data type with latest MSOLEDBSQL and SQLNCLI,
   // with SQLNCLI w/DataTypeCompatibilyt=80 recognized as adVarWChar
   // unsupported ADO DataTypeEnum 145,  so we set it varchar
   else if (ftype == "adDBTimeX")
      return ADODB::adVarChar;
   else
      throw TypeException("Invalid ADODB data type conversion from string", "AdodbUtil");
}

std::string adoDataTypeToString(ADODB::DataTypeEnum type)
{
   std::string retVal;

   switch (type)
   {
   case ADODB::adTinyInt:
      return "adTinyInt";
   case ADODB::adSmallInt:
      return "adSmallInt";
   case ADODB::adInteger:
      return "adInteger";
   case ADODB::adBigInt:
      return "adBigInt";
   case ADODB::adUnsignedTinyInt:
      return "adUnsignedTinyInt";
   case ADODB::adUnsignedSmallInt:
      return "adUnsignedSmallInt";
   case ADODB::adUnsignedInt:
      return "adUnsignedInt";
   case ADODB::adUnsignedBigInt:
      return "adUnsignedBigInt";
   case ADODB::adSingle:
      return "adSingle";
   case ADODB::adDouble:
      return "adDouble";
   case ADODB::adCurrency:
      return "adCurrency";
   case ADODB::adDecimal:
      return "adDecimal";
   case ADODB::adNumeric:
      return "adNumeric";
   case ADODB::adBoolean:
      return "adBoolean";
   case ADODB::adError:
      return "adError";
   case ADODB::adUserDefined:
      return "adUserDefined";
   case ADODB::adVariant:
      return "adVariant";
   case ADODB::adIDispatch:
      return "adIDispatch";
   case ADODB::adIUnknown:
      return "adIUnknown";
   case ADODB::adGUID:
      return "adGUID";
   case ADODB::adDate:
      return "adDate";
   case ADODB::adDBDate:
      return "adDBDate";
   case ADODB::adDBTime:
      return "adDBTime";
   case ADODB::adDBTimeStamp:
      return "adDBTimeStamp";
   case ADODB::adBSTR:
      return "adBSTR";
   case ADODB::adChar:
      return "adChar";
   case ADODB::adVarChar:
      return "adVarChar";
   case ADODB::adLongVarChar:
      return "adLongVarChar";
   case ADODB::adWChar:
      return "adWChar";
   case ADODB::adVarWChar:
      return "adVarWChar";
   case ADODB::adLongVarWChar:
      return "adLongVarWChar";
   case ADODB::adBinary:
      return "adBinary";
   case ADODB::adVarBinary:
      return "adVarBinary";
   case ADODB::adLongVarBinary:
      return "adLongVarBinary";
   case ADODB::adChapter:
      return "adChapter";
   case ADODB::adFileTime:
      return "adFileTime";
   case ADODB::adPropVariant:
      return "adPropVariant";
   case ADODB::adVarNumeric:
      return "adVarNumeric";
   case ADODB::adArray:
      return "adArray";
   // Note: 
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   // Sql server 2008 up "time" data type with latest MSOLEDBSQL and SQLNCLI,
   // with SQLNCLI w/DataTypeCompatibilyt=80 recognized as adVarWChar
   case 145:
      return "adDBTimeX";
   default:
      throw TypeException("Invalid ADODB data type conversion to string", "AdodbUtil");
   }

   return retVal;
}

ADODB::DataTypeEnum adoDataTypeFromDataType(DataType type)
{
   /*
      Note:
      https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
      https://www.w3schools.com/asp/met_comm_createparameter.asp
      https://docs.microsoft.com/en-us/dotnet/framework/data/adonet/sql-server-data-type-mappings
      https://www.w3schools.com/sql/sql_datatypes.asp
      https://wiki.ispirer.com/sqlways/sql-server/data-types/double_precision
      https://documentation.help/adosql/adoprg02_294j.htm
   */

   ADODB::DataTypeEnum retVal;

   switch (type)
   {
   case DataType::tinyint:
      return ADODB::adTinyInt;
   case DataType::smallint:
      return ADODB::adSmallInt;
   case DataType::integer:
      return ADODB::adInteger;
   case DataType::bigint:
      return ADODB::adBigInt;
   case DataType::numeric:
      return ADODB::adNumeric;
   case DataType::float4:
      return ADODB::adSingle;
   case DataType::float8:
      return ADODB::adDouble;
   case DataType::boolean:
      return ADODB::adBoolean;
   case DataType::character:
      return ADODB::adWChar;
   case DataType::varchar:
      return ADODB::adVarWChar;
   case DataType::text:
      return ADODB::adLongVarWChar;
   case DataType::date:
      return ADODB::adDBDate;
   case DataType::time:
      return ADODB::adDBTime;
   case DataType::timestamp:
      return ADODB::adDBTimeStamp;
   case DataType::varbinary:
      return ADODB::adVarBinary;
   default:
      throw TypeException("Invalid DataType conversion to ADODB data type", "AdodbUtil");
   }

   return retVal;
}

DataType adoDataTypeToDataType(ADODB::DataTypeEnum ftype)
{
   DataType retVal;

   switch (ftype)
   {
   case ADODB::adTinyInt:
      return DataType::tinyint;
   case ADODB::adSmallInt:
      return DataType::smallint;
   case ADODB::adInteger:
      return DataType::integer;
   case ADODB::adBigInt:
      return DataType::bigint;
   case ADODB::adSingle:
      return DataType::float4;
   case ADODB::adDouble:
      return DataType::float8;
   case ADODB::adDecimal:
   case ADODB::adNumeric:
      return DataType::numeric;
   case ADODB::adBoolean:
      return DataType::boolean;
   case ADODB::adDate:
   case ADODB::adDBDate:
      return DataType::date;
   case ADODB::adDBTime:
      return DataType::time;
   case ADODB::adDBTimeStamp:
      return DataType::timestamp;
   case ADODB::adChar:
      return DataType::character;
   case ADODB::adBSTR:
   case ADODB::adVarChar:
   case ADODB::adWChar:
   case ADODB::adVarWChar:
      return DataType::varchar;
   case ADODB::adLongVarWChar:
   case ADODB::adLongVarChar:
      return DataType::text;
   case ADODB::adBinary:
   case ADODB::adVarBinary:
   case ADODB::adLongVarBinary:
      return DataType::varbinary;
   case ADODB::adCurrency:
   case ADODB::adUnsignedTinyInt:
   case ADODB::adUnsignedSmallInt:
   case ADODB::adUnsignedInt:
   case ADODB::adUnsignedBigInt:
   case ADODB::adError:
   case ADODB::adUserDefined:
   case ADODB::adVariant:
   case ADODB::adIDispatch:
   case ADODB::adIUnknown:
   case ADODB::adGUID:
   case ADODB::adChapter:
   case ADODB::adFileTime:
   case ADODB::adPropVariant:
   case ADODB::adVarNumeric:
   case ADODB::adArray:
      return DataType::unknown;
   // Note: 
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   // Sql server 2008 up "time" data type with latest MSOLEDBSQL and SQLNCLI,
   // with SQLNCLI w/DataTypeCompatibilyt=80 recognized as adVarWChar
   case 145:
      return DataType::timestamp;
   default:
      throw TypeException("Invalid DataType conversion from ADODB data type", "AdodbUtil");
   }

   return retVal;
}

ADODB::ParameterDirectionEnum adoParamDirectionFromParamDirection(ParameterDirection direction)
{
   ADODB::ParameterDirectionEnum retVal;

   switch (direction)
   {
   case ParameterDirection::input:
      return ADODB::adParamInput;
   case ParameterDirection::output:
      return ADODB::adParamOutput;
   case ParameterDirection::inputOutput:
      return ADODB::adParamInputOutput;
   case ParameterDirection::returnValue:
      return ADODB::adParamReturnValue;
   default:
      throw TypeException("Invalid Adodb parameter direction conversion", "AdodbUtil");
   }

   return retVal;
}

TypeClass typeClassFromAdodbType(const long type)
{
   TypeClass retVal;

   switch (type)
   {
   case ADODB::adTinyInt:
   case ADODB::adSmallInt:
   case ADODB::adInteger:
   case ADODB::adBigInt:
   case ADODB::adDecimal:
   case ADODB::adDouble:
   case ADODB::adNumeric:
   case ADODB::adSingle:
   case ADODB::adUnsignedInt:
   case ADODB::adUnsignedBigInt:
   case ADODB::adUnsignedSmallInt:
   case ADODB::adUnsignedTinyInt:
   case ADODB::adVarNumeric:
   case ADODB::adCurrency:
      return TypeClass::numeric;
   case ADODB::adBinary:
   case ADODB::adLongVarBinary:
   case ADODB::adChapter:
   case ADODB::adVarBinary:
      return TypeClass::blob;
   case ADODB::adBoolean:
      return TypeClass::boolean;
   case ADODB::adBSTR:
   case ADODB::adChar:
   case ADODB::adLongVarChar:
   case ADODB::adLongVarWChar:
   case ADODB::adUserDefined:
   case ADODB::adVarChar:
   case ADODB::adVarWChar:
   case ADODB::adWChar:
      return TypeClass::string;
   case ADODB::adDBDate:
   case ADODB::adDate:
   case ADODB::adDBTime:
   case ADODB::adDBTimeStamp:
      return TypeClass::date;
   case ADODB::adError:
   case ADODB::adFileTime:
   case ADODB::adPropVariant:
   case ADODB::adVariant:
   case ADODB::adEmpty:
   case ADODB::adGUID:
   case ADODB::adIDispatch:
   case ADODB::adIUnknown:
   default:
      throw TypeException(tbsfmt::format("Invalid Adodb data type conversion to TypeClass: {}", type), "AdodbUtil");
   }

   return retVal;
}

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)