#pragma once

#include <string>

namespace tbs {
namespace bci {

/** \ingroup LIS
 * FieldValueType
 */
enum class FieldValueType
{
   Integer,
   String,
   Enum,
   DateTime,
   Unknown
};

/** \ingroup LIS
 * FieldType
 */
enum class FieldType
{
   Normal,
   Repeat,
   Component,
   Uknown
};

/** \ingroup LIS
 * BciField
 */
class BciField
{
public:
   BciField(int index, const std::string& code, const std::string& name, const std::string& value = "");
   BciField(const std::string& code, const std::string& name, const std::string& value = "");
   BciField() = default;
   ~BciField() = default;

   int index() { return _index; }
   std::string name() { return _name; }
   std::string code() { return _code; }
   FieldType type() { return _fieldType; }
   std::string value() { return _value; }
   FieldValueType valueType() { return _valueType; }

   void setIndex(int idx) { _index = idx; }
   void setName(const std::string& name) { _name = name; }
   void setCode(const std::string& code) { _code = code; }
   void setType(FieldType type) { _fieldType = type; }
   void setValue(const std::string& val) { _value = val; }
   void setValueType(FieldValueType valType) { _valueType = valType; }

private:
   int            _index;
   std::string    _code;
   std::string    _name;
   FieldType      _fieldType;
   std::string    _value;
   FieldValueType _valueType;
};

} // namespace bci
} // namespace tbs 