#pragma once

#include <string>
#include "tobasalis/hl7/encoding.h"

namespace tbs {
namespace hl7 {

class MessageElement
{
protected:
   int _myId = 0;
   std::string _value;
   HL7EncodingPtr _encoding = nullptr;;

public:
   MessageElement() = default;
   virtual ~MessageElement() = default;

   std::string value();

   void value(const std::string& val);

   std::string undecodedValue();

   HL7EncodingPtr encoding();
   const HL7EncodingPtr& encoding() const;

protected:
   void encoding(HL7EncodingPtr encoding);
   virtual void processValue() = 0;
};

} // namespace hl7
} // namespace tbs