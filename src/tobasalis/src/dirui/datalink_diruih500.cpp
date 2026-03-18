#include "tobasalis/lis/connection.h"
#include "tobasalis/dirui/datalink_diruih500.h"

namespace tbs {
namespace dirui {

DataLinkDirUih500::DataLinkDirUih500(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : lis::DataLink(io_ctx, connection, limit) {}

void DataLinkDirUih500::connectionOnReceiveData(const std::string& data)
{
   std::string buffer = data;
   if (buffer.length() > 0)
   {
      for (std::string::iterator it = buffer.begin(); it < buffer.end(); ++it)
      {
         char ch = *it;

         // new message is coming, reset _tempReceiveBuffer
         if (ch == STX)
            _tempReceiveBuffer.str("");

         if (ch != '\0')
            _tempReceiveBuffer << ch;

         if (ch == ETX)
         {
            std::string tempReceiveBuffer = _tempReceiveBuffer.str();

            // Remove Low Level control characters STX and ETX
            // STX is first char
            std::string cleanReceiveBuffer = tempReceiveBuffer.substr(1, tempReceiveBuffer.length());

            if (onReceiveData)
               onReceiveData(cleanReceiveBuffer);

            // reset _tempReceiveBuffer
            _tempReceiveBuffer.str(""); 
         }
      }
   }
}

} // namespace dirui
} // namespace tbs