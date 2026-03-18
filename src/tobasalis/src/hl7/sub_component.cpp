#include <iostream>
#include <string>
#include "tobasalis/hl7/sub_component.h"

namespace tbs {
namespace hl7 {

#ifdef TBS_HL7_DEBUG_INSTANCE
namespace {
static int subComponentId = 0;   
}
#endif

SubComponent::~SubComponent()
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   std::cout << "~SubComponent " << _myId << "\n";
#endif   
}

SubComponent::SubComponent(const std::string& val, HL7EncodingPtr encoding)
{
#ifdef TBS_HL7_DEBUG_INSTANCE   
   _myId = subComponentId++;
#endif

   _encoding = encoding;
   _value = val;
}

void SubComponent::processValue()
{
}


} // namespace hl7
} // namespace tbs