#include <tobasa/logger.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/hl7/datalink_hl7_std.h"


namespace tbs {
namespace hl7 {

DataLinkHl7Std::DataLinkHl7Std(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : lis::DataLink(io_ctx, connection, limit) 
{
   _state = State::Idle;
}

void DataLinkHl7Std::connectionOnReceiveData(const std::string& data)
{
   std::string buffer = data;
   if (buffer.length() > 0)
   {
      for (std::string::iterator it = buffer.begin(); it < buffer.end(); ++it)
      {
         char ch = *it;

         switch (_state)
         {
            case State::Idle:
            {
               // Start of a block of message
               // new message is coming, reset _tempReceiveBuffer
               if (ch == VT)
               {
                  _state = State::GotSB;
                  _tempReceiveBuffer.str("");
                  _previousCharacter = NUL;

                  Logger::logI("[lis_link] Read: <VT>");
               }
            }
            break;

            // we are receiving message
            case State::GotSB:
            {
               // end block marker arrive
               if (ch == NUL) {
                  Logger::logW("[lis_link] Receive NUL inside GotSB state");
               }
               else if (ch == FS && _previousCharacter == CR )
               {
                  // For End of block of message, FS must come after CR
                  // end of a message marker #1 arrive (FS)
                  _state = State::GotEB;
               }
               else {
                  _tempReceiveBuffer << ch;
               }
            }
            break;

            case State::GotEB:
            {
               if (ch == CR  && _previousCharacter == FS  )
               {
                  _state = State::Idle;
                  Logger::logI("[lis_link] Read: <FS><CR>, entering Idle state");
                  Logger::logI("[lis_link] ----------------------------------------------------");

                  // Transfer completed
                  std::string buffer = _tempReceiveBuffer.str();

                  // reset _tempReceiveBuffer
                  _tempReceiveBuffer.str("");
                  // reset _previousCharacter
                  _previousCharacter = NUL;

                  if (onReceiveData)
                     onReceiveData(buffer);

                  // Tell Lis engine, communication channel is idle
                  if (onIdle) {
                     onIdle(_pConnection);
                  }

                  return;
               }
               else
               {
                  // error: CR must come right after FS
                  Logger::logE("[lis_link] connectionOnReceiveData: receive NON CR inside GotEB state, ch: {} previous char: {}", ch, _previousCharacter);
               }
            }
            break;
         }

         _previousCharacter = ch;
      }
   }
}

void DataLinkHl7Std::sendMessage(const std::string& message, const std::string& endOfLine)
{ 
   std::string payload;
   payload.append(1, VT);     // 11 VT (Vertical Tab)
   payload.append(message);
   payload.append(1, FS);     // 28 FS (File Separator)
   payload.append(1, CR);     // 13 CR (Carriage Return)

   Logger::logI("[lis_link] Send: {}", strInRaw(message));
   _pConnection->asyncSend(payload);
}

void DataLinkHl7Std::sendString(const std::string& data)
{
   std::string payload;
   payload.append(1, VT);     // 11 VT (Vertical Tab)
   payload.append(data);
   payload.append(1, FS);     // 28 FS (File Separator)
   payload.append(1, CR);     // 13 CR (Carriage Return)

   Logger::logI("[lis_link] Send: {}", strInRaw(payload));

   _pConnection->send(payload);
}

void DataLinkHl7Std::sendTestMessage(const std::string& rawdata)
{
   Logger::logI("[lis_link] sendTestMessage()");
   sendString(rawdata);
}


} // namespace hl7
} // namespace tbs