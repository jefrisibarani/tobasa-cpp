#pragma once

#include <vector>
#include <memory>
#include <tobasa/logger.h>
#include <tobasa/variant.h>
#include <tobasa/variant_helper.h>
#include <tobasa/self_counter.h>
#include <tobasa/bin_encode.h>
#include "tobasasql/common_types.h"
#include "tobasasql/exception.h"
#include <tobasa/util.h>


namespace tbs {
namespace sql {

#if !defined(__cpp_char8_t)
   using char8_t = unsigned char; // for pre c++20
#endif

/** \addtogroup SQL
 * @{
 */

/**
 * @brief Converts an unsigned integral value to the supported SQL integer range.
 * @details
 * The value is stored as a signed int64_t after validation. The accepted
 * maximum is determined by the supplied SQL data type.
 *
 * @tparam T Unsigned integral type.
 * @param value Value to validate and convert.
 * @param dataType Target SQL integer type.
 * @return The converted value as int64_t.
 * @throws std::out_of_range If the value exceeds the maximum for dataType.
 */
template <typename T>
constexpr auto normalizeUnsignedIntegralToSqlRange(T value, DataType dataType)
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

/**
 * @brief Converts a signed integral value to the supported SQL integer range.
 * @details
 * The value is stored as a signed int64_t after validation. The accepted
 * maximum is determined by the supplied SQL data type.
 *
 * @tparam T Signed integral type.
 * @param value Value to validate and convert.
 * @param dataType Target SQL integer type.
 * @return The converted value as int64_t.
 * @throws std::out_of_range If the value exceeds the maximum for dataType.
 */
template <typename T>
constexpr auto normalizeSignedIntegralToSqlRange(T value, DataType dataType)
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
 * @details Encapsulates the name, SQL type, value, size, direction, and
 * precision of a parameter used by a query or stored procedure.
 *
 * Binary values for DataType::varbinary and DataType::varbit may be provided
 * as an even-length hexadecimal std::string or as raw std::vector<uint8_t>
 * data.
 *
 * Integral values are validated against the selected SQL type. Unsigned
 * values are stored as signed int64_t and therefore cannot exceed INT64_MAX
 * for DataType::bigint.
 *
 * @tparam VariantTypeImplemented Variant type used to store the value.
 *         Defaults to @c DefaultVariantType.
 *
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
         // TODO_JEFRI: create a branch to handle with unsigned value for MySQL, since MySQL support unsigned integral column
         _value = VariantType { 
                     normalizeUnsignedIntegralToSqlRange(std::forward<T>(value), _type) 
                  };
      }
      else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
      {
         // on LP64 platforms. Linux/macOS (GCC/Clang)
         if constexpr ( sizeof(long) == 8 && (std::is_same_v<T,long> || std::is_same_v<T,int64_t>) ) 
         {
            _value = VariantType { 
                        normalizeSignedIntegralToSqlRange(std::forward<T>(value), _type) 
                     };
         }
         else if constexpr (std::is_same_v<T,long long>)
            _value = static_cast<int64_t>(value);
         else if constexpr (std::is_same_v<T,long>)
           _value = static_cast<int32_t>(value);
         else if constexpr (std::is_same_v<T,int>)
            _value = static_cast<int32_t>(value);
         else if constexpr (std::is_same_v<T,short>)
            _value = static_cast<int16_t>(value);
         else
            _value = value;
      }
      else
      {
         // String-like types

         if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
            _value = std::string(value);
         else if constexpr (std::is_same_v<T, const wchar_t*> || std::is_same_v<T, wchar_t*>)
            _value = std::wstring(value);
         else if constexpr (std::is_same_v<T, std::string>)
            _value = value;
         else if constexpr (std::is_same_v<T, std::wstring>)
            _value = value;

         // Binary types 
         else if constexpr (std::is_same_v<T, const void*>) 
         {
            if (size <= 0)
               throw std::invalid_argument("Binary parameter requires explicit size");

            const std::uint8_t* ptr = static_cast<const std::uint8_t*>(value);
            _value = std::vector<std::uint8_t>(ptr, ptr + size);
         }
         else
            _value = value;
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

   /**
    * @brief Returns a pointer to the parameter value in textual form.
    * @details
    * Intended for PostgreSQL text parameter binding. Raw varbit bytes are
    * converted to a bit string; other values use VariantHelper::toString().
    * The optional modifier is applied before the pointer is returned.
    *
    * @param modifierFn Optional function that may modify the generated text.
    * @return A pointer to the generated text buffer.
    */
   std::shared_ptr<char*> valueCharPtr(std::function<void(std::string&)> modifierFn = nullptr)
   {
      if (_type == DataType::varbit && std::holds_alternative<std::vector<uint8_t>>(_value))
      {
         // convert binary data into its textual bit string. e.g "1010101000000001".
         const uint64_t byteCount = dataSize();
         _valueBufferString = conv::binaryBytesToString( (byte_t*) std::get<std::vector<uint8_t>>(_value).data(), byteCount );

         if (modifierFn)
            modifierFn(_valueBufferString);

         return std::make_shared<char*>(_valueBufferString.data());
      }
      else
      {
         // note: for binary data, VariantHelper::toString() will return hex string 
         _valueBufferString = VariantHelper::toString(_value);

         if (modifierFn) {
            modifierFn(_valueBufferString);
         }

         return std::make_shared<char*>(_valueBufferString.data());
      }
   }

   /**
    * @brief Returns a pointer to the parameter's raw binary payload.
    * @details
    * Valid for DataType::varbinary and DataType::varbit with either an
    * even-length hexadecimal std::string or a std::vector<uint8_t> value.
    * For hexadecimal input, the parameter size must match the decoded byte
    * count.
    *
    * @return A pointer to the raw binary payload.
    * @throws SqlException If the data type, value, or hexadecimal input is
    *         invalid.
    */
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

            _valueBufferBinary.resize(hexLen);

            conv::hexDecode(VariantHelper::template value<std::string>(_value), _valueBufferBinary.data());

            return std::make_shared<uint8_t*>( _valueBufferBinary.data() );
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

   uint64_t dataSize()
   {
      if (std::holds_alternative<std::vector<uint8_t>>(_value))
      {
         size_t len = std::get<std::vector<uint8_t>>(_value).size(); 
         return static_cast<uint64_t>(len);
      }
      else
         return _size;
   }

   bool forceUnsigned() const { return _forceUnsigned; }

protected:

   std::string             _name;
   DataType                _type = DataType::unknown;
   VariantType             _value;
   uint64_t                _size = 0;
   ParameterDirection      _direction = ParameterDirection::unknown;
   short                   _decimalDigits = 0;

   /// Temporary text form of _value.
   /// Used when preparing PostgreSQL parameters.
   std::string             _valueBufferString;

   /// Temporary raw-byte buffer for binary params stored as hex in _value.
   /// Keeps the decoded bytes stable so caller can use a pointer without decoding again.
   std::vector<uint8_t>    _valueBufferBinary;

   bool _forceUnsigned;
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