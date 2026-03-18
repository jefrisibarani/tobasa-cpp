#include <iostream>
#include <string>
#include <tobasa/util_string.h>
#include "tobasalis/hl7/exception.h"
#include "tobasalis/hl7/field.h"

namespace tbs {
namespace hl7 {

#ifdef TBS_HL7_DEBUG_INSTANCE
namespace {
static int fieldId = 0;   
}
#endif

Field::~Field()
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   std::cout << "~Field " << _myId << "\n";
#endif   
}

Field::Field(HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   _myId = fieldId++;
#endif

   _encoding = encoding;
}

Field::Field(std::string value, HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   _myId = fieldId++;
#endif

   _encoding = encoding;
   _value = value;
   processValue();
}

void Field::processValue()
{
   if (isDelimitersField())  // Special case for the delimiters fields (MSH)
   {
      auto subcomponent = std::make_shared<SubComponent>(_value, _encoding);
      auto component= std::make_shared<Component>(_encoding, true);
      component->subComponents().push_back(subcomponent);
      _componentList.add(component);
      
      return;
   }

   _hasRepetitions = (_value.find(_encoding->repeatDelimiter()) != std::string::npos);

   if (_hasRepetitions)
   {
      std::vector<std::string> individualFields = util::split(_value, _encoding->repeatDelimiter());

      for (int index = 0; index < individualFields.size(); index++)
      {
         auto field = std::make_shared<Field>(individualFields[index], _encoding);
         _repetitionList.add(field);
      }
   }
   else
   {
      std::vector<std::string> allComponents = util::split(_value, _encoding->componentDelimiter());

      for (auto strComponent: allComponents)
      {
         auto component = std::make_shared<Component>(_encoding);
         component->value(strComponent);
         _componentList.add(component);
      }

      _isComponentized = _componentList.size() > 1;
   }
}

bool Field::addNewComponent(ComponentPtr com)
{
   try
   {
      _componentList.add(com);
      return true;
   }
   catch(const std::exception& ex)
   {
      throw HL7Exception(std::string("Unable to add new component Error - ") + ex.what());
   }
}

bool Field::addNewComponent(ComponentPtr component, int position)
{
   try
   {
      position = position - 1;
      _componentList.add(component, position);
      return true;
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception( std::string("Unable to add new component Error - ") + ex.what());
   }
}

ComponentPtr Field::components(int position)
{
   position = position - 1;

   try
   {
      return _componentList.at(position);
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Component not available Error - ") + ex.what());
   }
}

ComponentCollection& Field::components()
{
   return _componentList;
}

const ComponentCollection& Field::components() const
{
   return _componentList;
}


FieldCollection& Field::repetitions()
{
   return _repetitionList;
}

FieldPtr Field::repetitions(int repetitionNumber)
{
   if (_hasRepetitions) {
      return _repetitionList[repetitionNumber - 1];
   }

   return std::make_shared<Field>(_encoding);
}

bool Field::removeEmptyTrailingComponents()
{
   try
   {
      for (size_t eachComponent = _componentList.size() - 1; eachComponent >= 0; eachComponent--)
      {
         if ( _componentList.at(eachComponent)->value().empty() )
            _componentList.erase(eachComponent);
         else
            break;
      }

      return true;
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Error removing trailing comonents - ") + ex.what());
   }
}

void Field::addRepeatingField(FieldPtr field)
{
   if (!_hasRepetitions) {
      throw HL7Exception("Repeating field must have repetions (HasRepetitions = true)");
   }

   _repetitionList.add(field);
}

} // namespace hl7
} // namespace tbs
