#include <utility>
#include <string>
#include "tobasalis/lis2a/record.h"
#include "tobasalis/lis2a/field.h"

namespace tbs {
namespace lis2a {

LisField::LisField(int index, const std::string& name, const std::string& value)
{
   _index      = index;
   _name       = name;
   _fieldType  = FieldType::Uknown;
   _value      = value;
   _valueType  = FieldValueType::String;
   _pSubRecord = nullptr;
}

void LisField::setSubRecord(Record* subrecord)
{
   _pSubRecord = subrecord;
}

} // namespace lis2a
} // namespace tbs
