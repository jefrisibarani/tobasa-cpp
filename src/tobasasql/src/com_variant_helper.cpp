#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <functional>
#include <tobasa/logger.h>
#include <tobasa/datetime.h>
#include <tobasa/exception.h>
#include <tobasa/hextodec.h>
#include <atlsafe.h>
#include "tobasasql/adodb_util.h"
#include "tobasasql/com_variant_helper.h"

/*

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

bool ComVariantHelper::isEmpty(const VariantType& variantValue)
{
   try
   {
      // check native variant first
      _variant_t value;
      if (std::holds_alternative<_variant_t>(variantValue))
      {
         value = std::get<_variant_t>(variantValue);

         if (value.vt == VT_EMPTY || value.vt == VT_NULL)
            return true;
         else
            return false;
      }
      else
      {
         // fallback to check base class method
         return BaseType::isEmpty(variantValue);
      }
   }
   catch (const std::bad_variant_access&)
   {
      throw AppException("ComVariantHelper: isEmpty, bad variant access");
   }

   return true;
}

std::string ComVariantHelper::toString(const VariantType& variantValue)
{
   std::string strValue;
   try
   {
      if (std::holds_alternative<_variant_t>(variantValue))
      {
         // get native _variant_t
         _variant_t vValue = std::get<_variant_t>(variantValue);
         nativeVariantToString(vValue, strValue);
      }
      else
      {
         // fallback to check base class method
         return BaseType::toString(variantValue);
      }
   }
   catch (const std::bad_variant_access& )
   {
      throw AppException("ComVariantHelper: toString, bad variant access");
   }

   return strValue;
}

void ComVariantHelper::nativeVariantToString(const _variant_t& vSource, std::string& outStr)
{
   try
   {
      switch (vSource.vt)
      {
         case VT_BOOL:     // True=-1, False=0
            outStr = util::boolToStr(vSource.boolVal != 0);
            break;
         case VT_I1:       // signed char
         {
            char ch = (char)vSource.cVal;
            outStr.append(1, ch);
         }
         case VT_UI1:      // unsigned char
         {
            char ch = (char)vSource.bVal;
            outStr.append(1, ch);
         }
            break;
         case VT_UI2:      // unsigned short             => uint16_t
            outStr = std::to_string(vSource.uiVal);
            break;
         case VT_I2:       // 2 byte signed int          => int16_t
            outStr = std::to_string(vSource.iVal);
            break;
         case VT_UINT:     //                            => uint32_t
            outStr = std::to_string(vSource.uintVal);
            break;           
         case VT_INT:      // 4 byte signed machine int  => int32_t
            outStr = std::to_string(vSource.intVal);
            break;
         case VT_UI4:      // 4 byte unsigned int        => uint32_t
            outStr = std::to_string(vSource.ulVal);
            break;
         case VT_I4:       // 4 byte signed int          => int32_t
            outStr = std::to_string(vSource.lVal);
            break;
         case VT_UI8:       // 8 byte unsigned int       => uint64_t
            outStr = std::to_string(vSource.ullVal);
            break;            
         case VT_I8:       // 8 byte signed int          => int64_t
            outStr = std::to_string(vSource.llVal);
            break;
         case VT_R4:       // 4 byte real => Float
            outStr = std::to_string(vSource.fltVal);
            break;
         case VT_R8:       // 8 byte real => Double
            outStr = std::to_string(vSource.dblVal);
            break;
         case VT_DECIMAL:  // 16 byte fixed point
         case VT_DATE:     // date
         case VT_CY:       // currency
         case VT_BSTR:     // OLE Automation string => String
         {
            // Note: https://docs.microsoft.com/en-us/cpp/text/how-to-convert-between-various-string-types?view=msvc-160
            // note cast to char*, to correctly convert from _bstr_t
            //outStr = static_cast<const char*>(_bstr_t(vSource));
            outStr = util::utf8_from_bstr_t(_bstr_t(vSource));
            
            // for data type DATE, or adDbTimestamp,
            // data string format we get from ado is dd/mm/yyyy hh:mm:ss
            if (vSource.vt == VT_DATE)
            {
               tbs::DateTime dt;
               bool success = dt.parse(outStr, "%d/%m/%Y %H:%M:%S");
               if (success)
                  outStr = dt.isoDateTimeString();
            }
         }
            break;
         case VT_EMPTY:    // nothing
            outStr = "";
            break;
         case VT_NULL:     // SQL style null
            outStr = sql::NULLSTR;
            break;
         case 8209:        // _variant_t vt 8209 sql server binary data type
         {
            //#include <atlsafe.h>
            try
            {
               // Attach a safe array of int's
               CComSafeArray<BYTE> saData;
               saData.Attach(vSource.parray);

               // Pull out some data...
               for(ULONG i = 0; i < saData.GetCount(); ++i)
               {
                  BYTE b = saData.GetAt(i);
                  outStr += tbs::crypt::decToHex(b);
               }
               // Release
               saData.Detach();

            }
            catch (const CAtlException)
            {
                  throw std::exception("error converting CComSafeArray");
            }
         }
         break;
         default:
            throw AppException("Unsupported COM variant type");
         break;
      }
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      throw AppException(comErr.fullMessage.c_str());
   }
   catch (std::exception& e)
   {
      throw AppException(e);
   }
}

ComVariantHelper::NativeVariant ComVariantHelper::toNativeVariant(const VariantType& variantVal)
{
   _variant_t nativeVariant;
   try
   {         
      if (std::holds_alternative<std::monostate>(variantVal))
      {
         // do nothing nativeVariant is already null
      }
      else if (std::holds_alternative<bool>(variantVal))
      {
         bool boolVal = std::get<bool>(variantVal);
         nativeVariant.vt = VT_BOOL;
         nativeVariant.boolVal = boolVal;
      }
      else if (std::holds_alternative<int8_t>(variantVal))
      {
         auto int8Val = std::get<int8_t>(variantVal);
         nativeVariant.vt = VT_I1;
         nativeVariant.cVal = (char)int8Val;
      }      
      else if (std::holds_alternative<int16_t>(variantVal))
      {
         auto int16Val = std::get<int16_t>(variantVal);
         nativeVariant.vt = VT_I2;
         nativeVariant.iVal = int16Val;
      }
      // else if (std::holds_alternative<int>(variantVal))
      // {
      //    auto intVal = std::get<int>(variantVal);
      //    nativeVariant.vt = VT_INT;
      //    nativeVariant.intVal = intVal;
      // }
      else if (std::holds_alternative<int32_t>(variantVal))
      {
         auto lVal = std::get<int32_t>(variantVal);
         nativeVariant.vt = VT_I4;
         nativeVariant.lVal = (long) lVal;
      }
      else if (std::holds_alternative<int64_t>(variantVal))
      {
         auto llVal = std::get<int64_t>(variantVal);
         nativeVariant.vt = VT_I8;
         nativeVariant.llVal = llVal;
      }
      else if (std::holds_alternative<uint8_t>(variantVal))
      {
         auto uint8Val = std::get<uint8_t>(variantVal);
         nativeVariant.vt = VT_UI1;
         nativeVariant.bVal = uint8Val;
      }      
      else if (std::holds_alternative<uint16_t>(variantVal))
      {
         auto uint16Val = std::get<uint16_t>(variantVal);
         nativeVariant.vt = VT_UI2;
         nativeVariant.uiVal = uint16Val;
      }      
      // else if (std::holds_alternative<unsigned int>(variantVal))
      // {
      //    auto uintVal = std::get<unsigned int>(variantVal);
      //    nativeVariant.vt = VT_UINT;
      //    nativeVariant.uintVal = uintVal;
      // }
      else if (std::holds_alternative<uint32_t>(variantVal))
      {
         auto ulVal = std::get<uint32_t>(variantVal);
         nativeVariant.vt = VT_UI4;
         nativeVariant.ulVal = ulVal;
      }
      else if (std::holds_alternative<uint64_t>(variantVal))
      {
         auto ullVal = std::get<uint64_t>(variantVal);
         nativeVariant.vt = VT_UI8;
         nativeVariant.ullVal = ullVal;
      }
      else if (std::holds_alternative<float>(variantVal))
      {
         float fltVal = std::get<float>(variantVal);
         nativeVariant.vt = VT_R4;
         nativeVariant.fltVal = fltVal;
      }
      else if (std::holds_alternative<double>(variantVal))
      {
         double dblVal = std::get<double>(variantVal);
         nativeVariant.vt = VT_R8;
         nativeVariant.dblVal = dblVal;
      }
      else if (std::holds_alternative<std::string>(variantVal))
      {
         auto& strVal = std::get<std::string>(variantVal);
         /*
          * Note: https://docs.microsoft.com/en-us/cpp/text/how-to-convert-between-various-string-types?view=msvc-160
          * note cast to char*, to convert from std::string to _bstr_t
          * _bstr_t bstr = static_cast<const char*>(strVal.c_str());

          * // Incorrect behavior
          * variant.vt = VT_BSTR;
          * variant.bstrVal = strVal.c_str();  or util::utf8_to_bstr_t(strVal)
          */
         // just assign the variant. nativeVariant may contain incorrect value
         // and lead to memory leak if we set its vt and bstrVal property manually
         nativeVariant = util::utf8_to_bstr_t(strVal);
      }
      else if (std::holds_alternative<std::wstring>(variantVal))
      {
         auto& wstrVal = std::get<std::wstring>(variantVal);
         _bstr_t bstr = wstrVal.c_str();
         nativeVariant = bstr;
      }      
      else if (std::holds_alternative<_variant_t>(variantVal))
      {
         nativeVariant = std::get<_variant_t>(variantVal);
      }
      else
      {
         throw AppException("ComVariantHelper: toNativeVariant, variant type holds unknown alternative");
      }
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      throw AppException(comErr.fullMessage.c_str());
   }
   catch (std::exception& e)
   {
      throw AppException(e);
   }

   return nativeVariant;
}

ComVariantHelper::VariantType ComVariantHelper::fromNativeVariant(const _variant_t& vSource)
{
   try
   {
      switch (vSource.vt)
      {
      case VT_BOOL:     // True=-1, False=0
         return (vSource.boolVal == 1);
      case VT_I1:       // signed char
      {
         std::string retVal;
         char ch = (char)vSource.cVal;
         retVal.append(1, ch);
         return retVal;
      }      
      case VT_UI1:      // unsigned char
      {
         std::string retVal;
         char ch = (char) vSource.bVal;
         retVal.append(1, ch);
         return retVal;
      }
      case VT_UI2:
         return (uint16_t) vSource.uiVal;
      case VT_I2:       // 2 byte signed int          => short
         return (int16_t) vSource.iVal;
      case VT_UINT:
         return (uint32_t) vSource.uintVal;         
      case VT_INT:      // 4 byte signed machine int  => int
         return (int32_t) vSource.intVal;
      case VT_UI4:
         return (uint32_t) vSource.ulVal;
      case VT_I4:       // 4 byte signed int          => long
         return (int32_t) vSource.lVal;
      case VT_UI8:
         return (uint64_t) vSource.ullVal;
      case VT_I8:       // 8 byte signed int          => long long
         return (int64_t) vSource.llVal;
      case VT_R4:       // 4 byte real => Float
         return (float) vSource.fltVal;
      case VT_R8:       // 8 byte real => Double
         return (double) vSource.dblVal;
      case VT_DECIMAL:  // 16 byte fixed point
      case VT_DATE:     // date
      case VT_CY:       // currency
      case VT_BSTR:     // OLE Automation string => String
         {
            // Note: https://docs.microsoft.com/en-us/cpp/text/how-to-convert-between-various-string-types?view=msvc-160
            // note cast to char*, to correctly convert from _bstr_t
            //outStr = static_cast<const char*>(_bstr_t(vSource));
            return util::utf8_from_bstr_t(_bstr_t(vSource));
         }
      case VT_EMPTY:          // nothing
         return std::string("");
      case VT_NULL:           // SQL style null
         return std::monostate{};
      case 8209:              // _variant_t vt 8209 sql server binary data type
      default:
         throw AppException("Unsupported COM variant type");
      }
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      throw AppException(comErr.fullMessage.c_str());
   }
   catch (std::exception& e)
   {
      throw AppException(e);
   }
}

} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)