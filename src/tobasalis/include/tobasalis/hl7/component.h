#pragma once

#include <memory>
#include <vector>
#include <string>
#include "tobasalis/hl7/collection.h"
#include "tobasalis/hl7/message_element.h"
#include "tobasalis/hl7/sub_component.h"

namespace tbs {
namespace hl7 {

class Component : public MessageElement
{
private:
   SubComponentCollection _subComponentList;
   bool _isDelimiter = false;
   bool _isSubComponentized = false;

   virtual void processValue();

public:
   virtual ~Component();
   Component(HL7EncodingPtr encoding, bool isDelimiter = false);
   Component(const std::string& value, HL7EncodingPtr encoding);

   void isSubComponentized(bool val) { _isSubComponentized = val; }
   bool isSubComponentized() { return _isSubComponentized; }

   SubComponentPtr subComponents(int position);

   void subComponents(const SubComponentCollection& subComponentList);

   SubComponentCollection& subComponents();
   const SubComponentCollection& subComponents() const;
};

using ComponentPtr = std::shared_ptr<Component>;
using ComponentCollection = Collection<Component>;


} // namespace hl7
} // namespace tbs
