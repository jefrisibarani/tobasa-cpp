#include <tobasa/datetime.h>
#include "tobasalis/lis/message.h"

namespace tbs {
namespace lis {

namespace {
   static int MESSAGE_ID_INT = 0;

   std::string leftPad(const std::string& str, char paddingChar, int minLength) 
   {
      if (str.length() >= minLength) {
         return str;
      }

      return std::string(minLength - str.length(), paddingChar) + str;
   }

   std::string createNewMessageId()
   {
      ++MESSAGE_ID_INT;
      DateTime currentDt;
      return currentDt.format("{:%y%m%d}") + leftPad(std::to_string(MESSAGE_ID_INT),'0',4);
   }
}

Message::Message()
{
   _internalId         = createNewMessageId();
   _pHeader            = nullptr;
   _pLastTouchedRecord = nullptr;
}

Message::~Message()
{
   if (_pHeader)
      _pHeader = nullptr;
}

std::string Message::internalId() const 
{ 
   return _internalId; 
}

int Message::instanceCreated()
{
   return MESSAGE_ID_INT;
}

} // namespace lis
} // namespace tbs