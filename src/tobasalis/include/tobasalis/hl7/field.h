#pragma once

#include <string>
#include <vector>
#include "tobasalis/hl7/collection.h"
#include "tobasalis/hl7/encoding.h"
#include "tobasalis/hl7/component.h"
#include "tobasalis/hl7/message_element.h"

namespace tbs {
namespace hl7 {

class Field;
using FieldPtr = std::shared_ptr<Field>;
using FieldCollection = hl7::Collection<Field>;

class Field : public MessageElement
{
private:
   FieldCollection     _repetitionList;
   ComponentCollection _componentList;

   bool _isComponentized   = false;
   bool _hasRepetitions    = false;
   bool _isDelimitersField = false;

protected:
   virtual void processValue();

public:
   ~Field();
   Field(HL7EncodingPtr encoding);
   Field(std::string value, HL7EncodingPtr encoding);

   bool isComponentized() { return _isComponentized; }
   bool hasRepetitions() { return _hasRepetitions; }
   bool isDelimitersField() { return _isDelimitersField; }
   void isComponentized(bool val) { _isComponentized = val; }
   void hasRepetitions(bool val) { _hasRepetitions = val; }
   void isDelimitersField(bool val) { _isDelimitersField = val; }

   bool addNewComponent(ComponentPtr com);

   bool addNewComponent(ComponentPtr component, int position);

   ComponentPtr components(int position);

   ComponentCollection& components();
   const ComponentCollection& components() const;

   FieldCollection& repetitions();

   FieldPtr repetitions(int repetitionNumber);

   bool removeEmptyTrailingComponents();

   void addRepeatingField(FieldPtr field);
};


} // namespace hl7
} // namespace tbs
