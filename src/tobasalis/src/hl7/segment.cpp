#include <iostream>
#include <string>
#include <tobasa/util_string.h>
#include "tobasalis/hl7/exception.h"
#include "tobasalis/hl7/encoding.h"
#include "tobasalis/hl7/field.h"
#include "tobasalis/hl7/segment.h"

namespace tbs {
namespace hl7 {

#ifdef TBS_HL7_DEBUG_INSTANCE
namespace {
static int segmentId = 0;   
}
#endif

Segment::~Segment()
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   std::cout << "~Segment " << _myId << "\n";
#endif   
}

Segment::Segment(HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   _myId = segmentId++;
#endif

   _encoding = encoding;
}

Segment::Segment(const std::string& name, HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   _myId = segmentId++;
#endif

   _name = name;
   _encoding = encoding;
}

void Segment::processValue()
{
   std::vector<std::string> allFields = util::split(_value, _encoding->fieldDelimiter());

   if (!allFields.empty()) {
      allFields.erase(allFields.begin());
   }

   for (int i = 0; i < allFields.size(); i++)
   {
      std::string strField = allFields[i];
      auto field = std::make_shared<Field>(_encoding);

      if (_name == "MSH" && i == 0)
         field->isDelimitersField(true); // special case

      field->value(strField);
      _fieldList.add(field);
   }

   if (_name == "MSH")
   {
      auto field1 = std::make_shared<Field>(_encoding);
      field1->isDelimitersField(true);
      field1->value(std::string(1, _encoding->fieldDelimiter()));

      _fieldList.putAtFirt(field1);
   }
}

Segment Segment::deepCopy()
{
   Segment newSegment(_name, _encoding);
   newSegment.value(_value);

   return std::move(newSegment);
}

void Segment::addEmptyField()
{
   addNewField(std::string());
}

void Segment::addNewField(const std::string& content, int position)
{
   addNewField( std::make_shared<Field>(content, _encoding), position);
}

void Segment::addNewField(const std::string& content, bool isDelimiters)
{
   auto newField = std::make_shared<Field>(_encoding);

   if (isDelimiters)
      newField->isDelimitersField(true); // Prevent decoding

   newField->value(content);
   addNewField(newField, -1);
}

bool Segment::addNewField(const FieldPtr& field, int position)
{
   try
   {
      if (position < 0) {
         _fieldList.add(field);
      }
      else
      {
         position = position - 1;
         _fieldList.add(field, position);
      }

      return true;
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Unable to add new field in segment ") + _name + std::string(" Error - ") + ex.what());
   }
}

FieldPtr Segment::fields(int position)
{
   position = position - 1;

   try
   {
      return _fieldList.at(position);
   }
   catch(const std::exception& ex)
   {
      throw HL7Exception(std::string("Field not available Error - ") + ex.what());
   }
}

FieldCollection& Segment::getAllFields()
{
   return _fieldList;
}

int Segment::sequenceNo() const
{
   return _sequenceNo;
}


} // namespace hl7
} // namespace tbs
