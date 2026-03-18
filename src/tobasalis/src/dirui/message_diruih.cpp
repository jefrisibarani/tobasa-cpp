#include "tobasalis/lis/common.h"
#include "tobasalis/dirui/message_diruih.h"

namespace tbs {
namespace dirui {

Message::Message()
   : lis::Message() 
{
   _vendorProtocolId = lis::MSG_DIRUI;
}

std::string Message::toString()
{
   return "";
}

} // namespace dirui
} // namespace tbs