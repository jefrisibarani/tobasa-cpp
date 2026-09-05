#pragma once

#include <vector>
#include <memory>
#include <tobasa/logger.h>
#include <tobasa/variant.h>
#include <tobasa/variant_helper.h>
#include <tobasa/self_counter.h>
#include <tobasa/hextodec.h>
#include "tobasasql/common_types.h"
#include "tobasasql/exception.h"


namespace tbs {
namespace sql {

#if !defined(__cpp_char8_t)
   using char8_t = unsigned char; // for pre c++20
#endif

/** \addtogroup SQL
 * @{
 */

/**
 * @brief Promote an integral value according to the SQL DataType.
 *
 * Rules:
 * - Non-integral values are returned unchanged.
 * - Signed types are returned unchanged, except:
 *   - long is treated as int32_t, throws if out of range.
 * - Unsigned types are range-checked to the signed type matching the DataType:
 *   - tinyint  -> int64_t
 *   - smallint -> int64_t
 *   - integer  -> int64_t
 *   - bigint   -> int64_t
 *   Throws if the value exceeds the max of the target type.
 *
 * @tparam T Type of the input value.
 * @param value The value to promote.
 * @param dataType SQL DataType for determining the promotion.
 * @return Promoted value with type corresponding to DataType.
 * @throws std::out_of_range if the value cannot fit in the target type.
 */
template <typename T>
constexpr auto promoteUnsignedIntegralForDataType(T value, DataType dataType)
{
   uint64_t uval = static_cast<uint64_t>(value);

   switch (dataType)
   {
      case DataType::tinyint:
         if (uval <= static_cast<uint64_t>(std::numeric_limits<int8_t>::max()))
            return static_cast<int64_t>(uval);
         throw std::out_of_range("Unsigned value too large for SQL DataType tinyint");

      case DataType::smallint:
         if (uval <= static_cast<uint64_t>(std::numeric_limits<int16_t>::max()))
            return static_cast<int64_t>(uval);
         throw std::out_of_range("Unsigned value too large for SQL DataType smallint");

      case DataType::integer:
         if (uval <= static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
            return static_cast<int64_t>(uval);
         throw std::out_of_range("Unsigned value too large for SQL DataType integer");

      case DataType::bigint:
      default:
         if (uval <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            return static_cast<int64_t>(uval);
         throw std::out_of_range("Unsigned value too large for SQL DataType bigint");
   }
}

/// Converts the parameter value to a signed integer range that matches the target SQL type.
/// Throws if the value does not fit the type.
template <typename T>
constexpr auto normalizeSignedLongForDataType(T value, DataType dataType)
{
   int64_t val = static_cast<int64_t>(value);

   switch (dataType)
   {
      case DataType::tinyint:
         if (val <= static_cast<int64_t>(std::numeric_limits<int8_t>::max()))
            return static_cast<int64_t>(val);
         throw std::out_of_range("Signed value too large for SQL DataType tinyint");

      case DataType::smallint:
         if (val <= static_cast<int64_t>(std::numeric_limits<int16_t>::max()))
            return static_cast<int64_t>(val);
         throw std::out_of_range("Signed value too large for SQL DataType smallint");

      case DataType::integer:
         if (val <= static_cast<int64_t>(std::numeric_limits<int32_t>::max()))
            return static_cast<int64_t>(val);
         throw std::out_of_range("Signed value too large for SQL DataType integer");

      case DataType::bigint:
      default:
         if (val <= static_cast<int64_t>(std::numeric_limits<int64_t>::max()))
            return static_cast<int64_t>(val);
         throw std::out_of_range("Signed value too large for SQL DataType bigint");
   }
}

/**
 * @brief Represents a SQL parameter.
 * @details 
 * Encapsulates a single parameter used in SQL queries or stored procedures, 
 * including its name, type, value, size, direction, and precision.  
 *
 * Binary parameters are supported in two internal forms:
 * 1. hex-encoded std::string values
 * 2. std::vector<uint8_t> values
 * When a hex string is used, it is decoded into a temporary raw-byte buffer so
 * callers can obtain a stable pointer for backend-specific binding.
 *
 * This class is templated to allow different underlying variant/value 
 * implementations for representing parameter values. By default, it uses 
 * @c DefaultVariantType.
 *
 * @tparam VariantTypeImplemented Variant type used to store the parameter value.  
 *         Defaults to @c DefaultVariantType.
 */
template <typename VariantTypeImplemented = DefaultVariantType >
class Parameter
{
public:
   using VariantType   = VariantTypeImplemented;
   using VariantHelper = tbs::VariantHelper<VariantType>;

   static const uint64_t DEFAULT_SIZE = 0;

   Parameter()
   {
      _name          = "";
      _type          = DataType::unknown;
      _size          = DEFAULT_SIZE;
      _direction     = ParameterDirection::unknown;
      _decimalDigits = 0;
   }

   Parameter(
      const std::string& name,
      DataType           type,
      VariantType        value,
      uint64_t           size = 0,
      ParameterDirection direction = ParameterDirection::input,
      short              decimalDigits = 0 )
      : _name{name}
      , _type{type}
      , _value{value}
      , _size{size}
      , _direction{direction}
      , _decimalDigits{decimalDigits} {}


   // Generic constructor: normalize integral types
   template<typename T>
   Parameter(
      const std::string& name,
      DataType           type,
      T                  value,
      uint64_t           size = DEFAULT_SIZE,
      ParameterDirection direction = ParameterDirection::input,
      short              decimalDigits = 0)
      : _name{name}
      , _type{type}
      , _size{size}
      , _direction{direction}
      , _decimalDigits{decimalDigits}
   {

      if constexpr (  std::is_integral_v<T> && std::is_unsigned_v<T> &&
                     !std::is_same_v<T,bool> &&
                     !std::is_same_v<T,char> &&
                     !std::is_same_v<T,wchar_t> &&
                     !std::is_same_v<T,char8_t> &&
                     !std::is_same_v<T,char16_t> &&
                     !std::is_same_v<T,char32_t>)
      {
         _value = VariantType{ promoteUnsignedIntegralForDataType(std::forward<T>(value), _type) };
      }
      else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
      {
         if constexpr ( sizeof(long) == 8 && (std::is_same_v<T,long> || std::is_same_v<T,int64_t>) ) // on LP64 platforms. Linux/macOS (GCC/Clang)
         {
            _value = VariantType{ normalizeSignedLongForDataType(std::forward<T>(value), _type) };
         }
         else if constexpr (std::is_same_v<T,long long>) {
            _value = static_cast<int64_t>(value);
         }
         else if constexpr (std::is_same_v<T,long>)
         {
           _value = static_cast<int32_t>(value);
         }
         else if constexpr (std::is_same_v<T,int>) {
            _value = static_cast<int32_t>(value);
         }
         else if constexpr (std::is_same_v<T,short>) {
            _value = static_cast<int16_t>(value);
         }
         else
            _value = value;
      }
      else
      {
         // String-like types
         if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
            _value = std::string(value);
         }
         else if constexpr (std::is_same_v<T, const wchar_t*> || std::is_same_v<T, wchar_t*>) {
            _value = std::wstring(value);
         }
         else if constexpr (std::is_same_v<T, std::string>) {
            _value = value;
         }
         else if constexpr (std::is_same_v<T, std::wstring>) {
            _value = value;
         }
         // Binary types 
         else if constexpr (std::is_same_v<T, const void*>) 
         {
            if (size <= 0) {
               throw std::invalid_argument("Binary parameter requires explicit size");
            }
            const std::uint8_t* ptr = static_cast<const std::uint8_t*>(value);
            _value = std::vector<std::uint8_t>(ptr, ptr + size);
         }
         else {
            _value = value;
         }
      }
   }


   ~Parameter(){}

   std::string name()
   {
      return _name;
   }

   DataType type()
   {
      return _type;
   }

   ParameterDirection direction()
   {
      return _direction;
   }

   const VariantType& value()
   {
      return _value;
   }

   void value(VariantType val)
   {
      _value = val;
   }

   uint64_t size()
   {
      return _size;
   }

   short decimalDigits()
   {
      return _decimalDigits;
   }

   /// Returns a pointer to the textual form of the parameter value.
   /// Useful for backends that accept text parameters, such as PostgreSQL.
   /// A caller can optionally modify the generated string before using the pointer.
   std::shared_ptr<char*> valueCharPtr(std::function<void(std::string&)> modifierFn = nullptr)
   {
      _rawValData = VariantHelper::toString(_value);

      if (modifierFn) {
         modifierFn(_rawValData);
      }

      return std::make_shared<char*>( (char*) _rawValData.c_str() );
   }

   /// Returns a pointer to the raw binary payload of the parameter.
   /// It accepts hex-encoded string values or std::vector<uint8_t>.
   /// This is used by backends that need binary data directly.
   std::shared_ptr<uint8_t*> valueBinaryPtr()
   {
      if (_type == DataType::varbinary || _type == DataType::varbit)
      {
         // NOTE_JEFRI: varbinary or varbit param can only hold data in
         // 1. HEX Encoded string stored in std::string
         // 2. std::vector<uint8_t>

         if (std::holds_alternative<std::string>(_value))
         {
            // get binary data from HEX Encoded string stored in std::string

            // note gcc warning: expected ‘template’ keyword before dependent template name [-Wmissing-template-keyword]
            size_t hexLenOri = VariantHelper::template value<std::string>(_value).size();
            if (hexLenOri % 2) {
               throw SqlException("invalid binary string input", "SqlParameter");
            }

            // convert hexadecimal encoded data to a byte array
            size_t hexLen = hexLenOri / 2;

            if (hexLen != _size)
               throw SqlException("invalid binary string data size", "SqlParameter");

            _rawValBinary.reserve(hexLen);
            for (size_t i = 0; i < hexLen; ++i)
            {
               _rawValBinary.emplace_back(0);
            }

            crypt::hexDecode(VariantHelper::template value<std::string>(_value), _rawValBinary.data());

            return std::make_shared<uint8_t*>( _rawValBinary.data() );
         }
         else if (std::holds_alternative<std::vector<uint8_t>>(_value))
         {
            // get binary data stored in std::vector<uint8_t>
            return std::make_shared<uint8_t*>( std::get_if<std::vector<uint8_t>>(&_value)->data() );
         }
         else
            throw SqlException("Invalid variant type for binary data parameter", "SqlParameter");
      }
      else
      {
         throw SqlException("valueBinaryPtr() requires DataType::varbinary or DataType::varbit with a hex string or uint8_t vector payload", "SqlParameter");
      }
   }

   bool forceUnsigned() const { return _forceUnsigned; }
   
   uint64_t dataSize()
   {
      if (std::holds_alternative<std::vector<uint8_t>>(_value))
      {
         size_t len = VariantHelper::template value<std::vector<uint8_t>>(_value).size(); 
         return static_cast<uint64_t>(len);
      }
      else
         return _size;
   }

protected:

   std::string             _name;
   DataType                _type = DataType::unknown;
   VariantType             _value;
   uint64_t                _size = 0;
   ParameterDirection      _direction = ParameterDirection::unknown;
   short                   _decimalDigits = 0;

   /// Temporary text form of _value.
   /// Used when preparing PostgreSQL parameters.
   std::string             _rawValData;

   /// Temporary raw-byte buffer for binary params stored as hex in _value.
   /// Keeps the decoded bytes stable so caller can use a pointer without decoding again.
   std::vector<uint8_t>    _rawValBinary;

   bool     _forceUnsigned = false;
};


/// Default SqlParameter.
using SqlParameter = Parameter<>;

/// Default SqlParameter shared pointer.
using SqlParameterPtr = std::shared_ptr<SqlParameter>;

/// SqlParameter Collection.
using SqlParameterCollection = std::vector<SqlParameterPtr>;

/// Shared pointer to SqlParameter collection.
using SqlParameterCollectionPtr = std::shared_ptr<SqlParameterCollection>;

/** @}*/

} // namespace sql
} // namespace tbs