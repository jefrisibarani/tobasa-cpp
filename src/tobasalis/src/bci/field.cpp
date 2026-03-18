#include <utility>
#include <string>
#include "tobasalis/bci/field.h"

namespace tbs {
namespace bci {

BciField::BciField(int index, const std::string& code, const std::string& name, const std::string& value)
{
   _index      = index;
   _code       = code;
   _name       = name;
   _fieldType  = FieldType::Uknown;
   _value      = value;
   _valueType  = FieldValueType::String;
}

BciField::BciField(const std::string& code, const std::string& name, const std::string& value)
{
   _index      = -1;
   _code       = code;
   _name       = name;
   _fieldType  = FieldType::Uknown;
   _value      = value;
   _valueType  = FieldValueType::String;
}

} // namespace bci
} // namespace tbs
