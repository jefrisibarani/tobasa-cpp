#pragma once

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * FieldValueType
 */
enum class FieldValueType
{
   Integer,
   String,
   Enum,
   DateTime,
   SubRecord,
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

class Record;

/** \ingroup LIS
 * LisField
 * Holds one field and its metadata
 */
class LisField
{
public:
   LisField(int index, const std::string& name, const std::string& value = "");
   LisField() = default;
   ~LisField() = default;

   int index()                { return _index; }
   std::string name()         { return _name; }
   FieldType type()           { return _fieldType; }
   std::string value()        { return _value; }
   FieldValueType valueType() { return _valueType; }

   void setIndex(int idx)                    { _index = idx; }
   void setName(const std::string& name)     { _name = name; }
   void setType(FieldType type)              { _fieldType = type; }
   void setValue(const std::string& val)     { _value = val; }
   void setValueType(FieldValueType valType) { _valueType = valType; }
   void setSubRecord(Record* subrecord);
   Record* getSubRecord()                    { return _pSubRecord; }

private:
   int            _index;
   std::string    _name;
   FieldType      _fieldType;
   std::string    _value;
   FieldValueType _valueType;
   Record*        _pSubRecord;
};

} // namespace lis2a
} // namespace tbs