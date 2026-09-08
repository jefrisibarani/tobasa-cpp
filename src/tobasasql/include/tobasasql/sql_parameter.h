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
 * @brief Checks whether an unsigned integral value fits the SQL signed integer range.
 *
 * @tparam T Unsigned integral type.
 * @param value Value to validate.
 * @param dataType Target SQL integer type.
 * @return true if the value is within the range for dataType.
 */
template <typename T>
constexpr bool unsignedFitsSqlSigned(T value, DataType dataType)
{
   static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
   uint64_t uval = static_cast<uint64_t>(value);
   switch (dataType)
   {
      case DataType::tinyint:
         return  (uval <= static_cast<uint64_t>(std::numeric_limits<int8_t>::max()));
      case DataType::smallint:
         return  (uval <= static_cast<uint64_t>(std::numeric_limits<int16_t>::max()));
      case DataType::integer:
         return  (uval <= static_cast<uint64_t>(std::numeric_limits<int32_t>::max()));
      case DataType::bigint:
      default:
         return (uval <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
   }
}


/**
 * @brief Checks whether an unsigned integral value fits the SQL unsigned integer range.
 *
 * @tparam T Unsigned integral type.
 * @param value Value to validate.
 * @param dataType Target SQL integer type.
 * @return true if the value is within the range for dataType.
 */
template <typename T>
constexpr bool unsignedFitsSqlUnsigned(T value, DataType dataType)
{
   static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
   const uint64_t uval = static_cast<uint64_t>(value);
   switch (dataType)
   {
      case DataType::tinyint:
         return uval <= static_cast<uint64_t>(std::numeric_limits<uint8_t>::max());
      case DataType::smallint:
         return uval <= static_cast<uint64_t>(std::numeric_limits<uint16_t>::max());
      case DataType::integer:
         return uval <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max());
      case DataType::bigint:
      default:
         return uval <= static_cast<uint64_t>(std::numeric_limits<uint64_t>::max());
   }
}


/**
 * @brief Checks whether a signed integral value fits the SQL signed integer range.
 *
 * @tparam T Signed integral type.
 * @param value Value to validate.
 * @param dataType Target SQL integer type.
 * @return true if the value is within the range for dataType.
 */
template <typename T>
constexpr bool signedFitsSqlSigned(T value, DataType dataType)
{
   static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
   const int64_t val = static_cast<int64_t>(value);
   switch (dataType)
   {
      case DataType::tinyint:
         return val >= std::numeric_limits<int8_t>::min() && val <= std::numeric_limits<int8_t>::max();
      case DataType::smallint:
         return val >= std::numeric_limits<int16_t>::min() && val <= std::numeric_limits<int16_t>::max();
      case DataType::integer:
         return val >= std::numeric_limits<int32_t>::min() && val <= std::numeric_limits<int32_t>::max();
      case DataType::bigint:
      default:
         return val >= std::numeric_limits<int64_t>::min() && val <= std::numeric_limits<int64_t>::max();
   }
}


/**
 * @brief Checks whether a signed integral value fits the SQL unsigned integer range.
 *
 * @tparam T Signed integral type.
 * @param value Value to validate.
 * @param dataType Target SQL integer type.
 * @return true if the value is within the range for dataType.
 */
template <typename T>
constexpr bool signedFitsSqlUnsigned(T value, DataType dataType)
{
   static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

   if (value < 0)
      return false;

   const uint64_t uval = static_cast<uint64_t>(value);

   switch (dataType)
   {
      case DataType::tinyint:
         return uval <= std::numeric_limits<uint8_t>::max();
      case DataType::smallint:
         return uval <= std::numeric_limits<uint16_t>::max();
      case DataType::integer:
         return uval <= std::numeric_limits<uint32_t>::max();
      case DataType::bigint:
      default:
         return true; // Any non-negative standard signed integer fits uint64_t.
   }
}


/**
 * @brief Store an unsigned integer into SQL signed type
 *
 * @tparam VariantType Storage type.
 * @tparam T Unsigned integral type.
 * @param storage Destination storage.
 * @param value Value to store.
 * @param dataType Target SQL integer type.
 * @throws std::out_of_range If the value is outside dataType.
 */
template <typename VariantType, typename T>
void storeUnsignedForSqlSigned(VariantType& storage, T value, DataType dataType)
{
   const uint64_t uval = static_cast<uint64_t>(value);

   if (!unsignedFitsSqlSigned(uval, dataType))
   {
      switch (dataType)
      {
         case DataType::tinyint:
            throw std::out_of_range("Unsigned value out of range for SQL DataType tinyint");
         case DataType::smallint:
            throw std::out_of_range("Unsigned value out of range for SQL DataType smallint");
         case DataType::integer:
            throw std::out_of_range("Unsigned value out of range for SQL DataType integer");
         case DataType::bigint:
         default:
            throw std::out_of_range("Unsigned value out of range for SQL DataType bigint");
      }
   }

   switch (dataType)
   {
      case DataType::tinyint:
         storage = static_cast<int8_t>(uval);
         break;
      case DataType::smallint:
         storage = static_cast<int16_t>(uval);
         break;
      case DataType::integer:
         storage = static_cast<int32_t>(uval);
         break;
      case DataType::bigint:
      default:
         storage = static_cast<int64_t>(uval);
         break;
   }
}


/**
 * @brief Stores a signed integer into SQL signed type
 *
 * @tparam VariantType Storage type.
 * @tparam T Signed integral type.
 * @param storage Destination storage.
 * @param value Value to store.
 * @param dataType Target SQL integer type.
 * @throws std::out_of_range If the value is outside dataType.
 */
template <typename VariantType, typename T>
void storeSignedForSqlSigned(VariantType& storage, T value, DataType dataType)
{
   const int64_t sval = static_cast<int64_t>(value);

   if (!signedFitsSqlSigned(sval, dataType))
   {
      switch (dataType)
      {
         case DataType::tinyint:
            throw std::out_of_range("Signed value out of range for SQL DataType tinyint");
         case DataType::smallint:
            throw std::out_of_range("Signed value out of range for SQL DataType smallint");
         case DataType::integer:
            throw std::out_of_range("Signed value out of range for SQL DataType integer");
         case DataType::bigint:
         default:
            throw std::out_of_range("Signed value out of range for SQL DataType bigint");
      }
   }

   switch (dataType)
   {
      case DataType::tinyint:
         storage = static_cast<int8_t>(sval);
         break;
      case DataType::smallint:
         storage = static_cast<int16_t>(sval);
         break;
      case DataType::integer:
         storage = static_cast<int32_t>(sval);
         break;
      case DataType::bigint:
      default:
         storage = static_cast<int64_t>(sval);
         break;
   }
}

/**
 * @brief Stores an unsigned integer into SQL unsigned type
 *
 * @tparam VariantType Storage type.
 * @tparam T Unsigned integral type.
 * @param storage Destination storage.
 * @param value Value to store.
 * @param dataType Target SQL integer type.
 * @throws std::out_of_range If the value is outside dataType.
 */
template <typename VariantType, typename T>
void storeUnsignedForSqlUnsigned(VariantType& storage, T value, DataType dataType)
{
   const uint64_t uval = static_cast<uint64_t>(value);

   if (!unsignedFitsSqlUnsigned(uval, dataType))
   {
      switch (dataType)
      {
         case DataType::tinyint:
            throw std::out_of_range("Unsigned value too large for SQL DataType tinyint");
         case DataType::smallint:
            throw std::out_of_range("Unsigned value too large for SQL DataType smallint");
         case DataType::integer:
            throw std::out_of_range("Unsigned value too large for SQL DataType integer");
         case DataType::bigint:
         default:
            throw std::out_of_range("Unsigned value too large for SQL DataType bigint");
      }
   }

   switch (dataType)
   {
      case DataType::tinyint:
         storage = static_cast<uint8_t>(uval);
         break;
      case DataType::smallint:
         storage = static_cast<uint16_t>(uval);
         break;
      case DataType::integer:
         storage = static_cast<uint32_t>(uval);
         break;
      case DataType::bigint:
      default:
         storage = static_cast<uint64_t>(uval);
         break;
   }
}

/**
 * @brief Stores a signed integer into SQL unsigned type
 *
 * @tparam VariantType Storage type.
 * @tparam T Signed integral type.
 * @param storage Destination storage.
 * @param value Value to store.
 * @param dataType Target SQL integer type.
 * @throws std::out_of_range If the value is outside dataType.
 */
template <typename VariantType, typename T>
void storeSignedForSqlUnsigned(VariantType& storage, T value, DataType dataType)
{
   static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

   if (!signedFitsSqlUnsigned(value, dataType))
   {
      switch (dataType)
      {
         case DataType::tinyint:
            throw std::out_of_range("Signed value too large for SQL DataType unsigned tinyint");
         case DataType::smallint:
            throw std::out_of_range("Signed value too large for SQL DataType unsigned smallint");
         case DataType::integer:
            throw std::out_of_range("Signed value too large for SQL DataType unsigned integer");
         case DataType::bigint:
         default:
            throw std::out_of_range("Signed value too large for SQL DataType unsigned bigint");
      }
   }

   const uint64_t uval = static_cast<uint64_t>(value);

   switch (dataType)
   {
      case DataType::tinyint:
         storage = static_cast<uint8_t>(uval);
         break;
      case DataType::smallint:
         storage = static_cast<uint16_t>(uval);
         break;
      case DataType::integer:
         storage = static_cast<uint32_t>(uval);
         break;
      case DataType::bigint:
      default:
         storage = static_cast<uint64_t>(uval);
         break;
   }
}

/**
 * @brief Stores a SQL parameter value and its binding metadata.
 *
 * Stores a parameter name, SQL data type, value, size, direction, decimal
 * precision, and integer signedness. Integral values are checked against the
 * range of the selected SQL integer type and stored using the corresponding
 * signed or unsigned representation.
 *
 * Text values are stored as strings. Binary values for
 * @c DataType::varbinary and @c DataType::varbit may be supplied as an
 * even-length hexadecimal string or as a byte vector.
 *
 * @tparam VariantTypeImplemented Variant type used to store the parameter
 *         value. Defaults to @c DefaultVariantType.
 */
template <
   typename VariantTypeImplemented = DefaultVariantType>
class Parameter
{
public:
   using VariantType   = VariantTypeImplemented;
   using VariantHelper = tbs::VariantHelper<VariantType>;

   /**
    * @brief Constructs a parameter from an already-typed variant value.
    *
    * The value is stored without conversion or range checking. The caller
    * provides the SQL type and whether the parameter represents an unsigned
    * integer column.
    *
    * @param name Parameter name.
    * @param type SQL data type.
    * @param value Already-typed parameter value.
    * @param size Optional size for binary or text parameters.
    * @param decimalDigits Optional number of decimal digits.
    * @param isUnsigned Whether the SQL integer type is unsigned.
    * @param direction Parameter direction.
    */
   Parameter(
      const std::string& name,
      DataType           type,
      VariantType        value,
      uint64_t           size = 0,
      short              decimalDigits = 0,
      bool               isUnsigned = false,
      ParameterDirection direction = ParameterDirection::input)
      : _name{name}
      , _type{type}
      , _value{value}
      , _size{size}
      , _decimalDigits{decimalDigits}
      , _isUnsigned{isUnsigned}
      , _direction{direction}
      {}


   /**
    * @brief Constructs a parameter from a value and its SQL type.
    *
    * Integral values are range-checked against the selected SQL integer type
    * and stored using the corresponding signed or unsigned representation.
    * String and binary values are stored in their supported parameter form.
    *
    * @tparam T Input value type.
    *
    * @param name Parameter name.
    * @param type SQL data type.
    * @param value Already-typed parameter value.
    * @param size Optional size for binary or text parameters.
    * @param decimalDigits Optional number of decimal digits.
    * @param isUnsigned Whether the SQL integer type is unsigned.
    * @param direction Parameter direction.
    *
    * @throws std::out_of_range If an integral value does not fit the selected
    *         SQL integer type.
    * @throws std::invalid_argument If a binary pointer is provided without a
    *         positive size.
    */
   template<typename T>
   Parameter(
      const std::string& name,
      DataType           type,
      T                  value,
      uint64_t           size = 0,
      short              decimalDigits = 0,
      bool               isUnsigned = false,
      ParameterDirection direction = ParameterDirection::input)
      : _name{name}
      , _type{type}
      , _value{value}
      , _size{size}
      , _decimalDigits{decimalDigits}
      , _isUnsigned{isUnsigned}
      , _direction{direction}
   {

      if constexpr (  std::is_integral_v<T> && std::is_unsigned_v<T> &&
                     !std::is_same_v<T,bool> &&
                     !std::is_same_v<T,char> &&
                     !std::is_same_v<T,wchar_t> &&
                     (!std::is_same_v<T,char8_t> || std::is_same_v<T,uint8_t>) &&
                     !std::is_same_v<T,char16_t> &&
                     !std::is_same_v<T,char32_t>)
      {
         if (_isUnsigned)
            storeUnsignedForSqlUnsigned(_value, value, _type);
         else
            storeUnsignedForSqlSigned(_value, value, _type);
      }
      else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
      {
         if (isUnsigned)
            storeSignedForSqlUnsigned(_value, value, _type);
         else
            storeSignedForSqlSigned(_value, value, _type);
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

   bool isUnsigned() const { return _isUnsigned; }

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

   bool _isUnsigned = false;
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