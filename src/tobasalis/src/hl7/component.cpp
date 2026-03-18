#include <vector>
#include <string>
#include <iostream>
#include <tobasa/util_string.h>
#include "tobasalis/hl7/exception.h"
#include "tobasalis/hl7/sub_component.h"
#include "tobasalis/hl7/component.h"

namespace tbs {
namespace hl7 {

#ifdef TBS_HL7_DEBUG_INSTANCE
namespace {
static int componentId = 0;
}
#endif

Component::~Component()
{
#ifdef TBS_HL7_DEBUG_INSTANCE
   std::cout << "~Component " << _myId << "\n";
#endif
}

Component::Component(HL7EncodingPtr encoding, bool isDelimiter)
{
#ifdef TBS_HL7_DEBUG_INSTANCE
   _myId = componentId++;
#endif

   _encoding = encoding;
   _isDelimiter = isDelimiter;
}
Component::Component(const std::string& value, HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE
   _myId = componentId++;
#endif

   _value = value;
   _encoding = encoding;
   processValue();
}

void Component::processValue()
{
   std::vector<std::string> allSubComponents;

   if (_isDelimiter)
      allSubComponents.push_back( value() );
   else
      allSubComponents = util::split(_value, _encoding->subComponentDelimiter());

   if (allSubComponents.size() > 1)
      _isSubComponentized = true;

   for (auto strSubComponent: allSubComponents)
   {
      auto subComponent = std::make_shared<SubComponent>(_encoding->decode(strSubComponent), _encoding);
      _subComponentList.push_back(subComponent);
   }
}

SubComponentPtr Component::subComponents(int position)
{
   position = position - 1;

   try
   {
      return _subComponentList[position];
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("SubComponent not availalbe Error-") + ex.what());
   }
}

void Component::subComponents(const SubComponentCollection& subComponentList)
{
   _subComponentList = subComponentList;
}

SubComponentCollection& Component::subComponents()
{
   return _subComponentList;
}

const SubComponentCollection& Component::subComponents() const
{
   return _subComponentList;
}


} // namespace hl7
} // namespace tbs
