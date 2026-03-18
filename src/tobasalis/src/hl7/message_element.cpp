#include <iostream>
#include "tobasalis/hl7/message_element.h"

namespace tbs {
namespace hl7 {

std::string MessageElement::value()
{
   if (_value == _encoding->presentButNull())
      return _value;
   else
      return _encoding->decode(_value);
}

void MessageElement::value(const std::string& val)
{
   _value = val;
   processValue();
}

std::string MessageElement::undecodedValue()
{
   if (_value == _encoding->presentButNull())
      return "";
   else
      return _value;
}

HL7EncodingPtr MessageElement::encoding()
{
   return _encoding;
}

const HL7EncodingPtr& MessageElement::encoding() const
{
   return _encoding;
}

void MessageElement::encoding(HL7EncodingPtr encoding)
{
   _encoding = encoding;
}

} // namespace hl7
} // namespace tbs
