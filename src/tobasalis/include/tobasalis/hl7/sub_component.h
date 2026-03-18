#pragma once

#include <string>
#include <vector>
#include "tobasalis/hl7/message_element.h"

namespace tbs {
namespace hl7 {

class SubComponent : public MessageElement
{
public:
   ~SubComponent();
   SubComponent(const std::string& val, HL7EncodingPtr encoding);

protected:
   virtual void processValue();
};

using SubComponentPtr = std::shared_ptr<SubComponent>;
using SubComponentCollection = std::vector<SubComponentPtr>;

} // namespace hl7
} // namespace tbs