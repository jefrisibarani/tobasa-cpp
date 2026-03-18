#include <thread>
#include <iomanip>
#include <tobasa/logger.h>
#include <tobasa/util_string.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/bci/datalink.h"

namespace tbs {
namespace bci {

DataLinkBci::DataLinkBci(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : DataLinkBase(io_ctx, connection, limit)
{}

/*
<STX><RS>ThisIsTheData<GS>c6<CR><LF>
calculateChecksum calculate start from <RS> up to and include <GS>
param @data characters starts with <RS> up to and include <GS>
*/
std::string DataLinkBci::calculateChecksum(const std::string& data)
{
   std::string output;
   int total = 0;
   if (data.length() > 0)
   {
      for (std::string::const_iterator it = data.begin(); it < data.end(); ++it)
      {
         char i = *it;
         total += (int)i;
      }
   }

   int checksum = total;

   std::stringstream ss;
   ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << checksum;
   std::string checksumStr = ss.str();
   output = checksumStr.substr((checksumStr.length() - 2), 2);

   return output;
}
/* Set BCI Link to append <CR><LF> After <GS> Character */
/*  <STX><RS>ThisIsTheData<GS>C6<CR><LF>                */
bool DataLinkBci::checkChecksum(const std::string& line)
{
   bool result = false;
   auto lineLength = line.length();
   if (lineLength < 5)
      return result;

   if (line[0] != STX)
      return result;

   if (line[lineLength - 1] != LF)
      return result;

   if (line[lineLength - 2] != CR)
      return result;

   char gs = line[lineLength - 5];  // <GS> character
   if (gs != GS)
      return result;

   std::string tempChecksum  = line.substr(lineLength - 4, 2);   // get checksum value (2 chars) / truncated to 8 bit
   std::string tempLine      = line.substr(1, lineLength - 5);   // get chars starts from <RS> up to <GS>
   std::string tempCheckSum2 = calculateChecksum(tempLine);
   std::string oriChecksum   = tbs::util::toUpper(tempChecksum);

   return !(oriChecksum != tempCheckSum2) || result;
}

void DataLinkBci::connectionOnReceiveData(const std::string& data)
{
   std::string buffer = data;
   if (buffer.length() > 0)
   {
      for (std::string::iterator it = buffer.begin(); it < buffer.end(); ++it)
      {
         char ch = *it;
         switch (_state)
         {
         case lis::LinkState::Idle:
            if (ch == ENQ)
            {
               Logger::logI("[lis_link] Read <ENQ>");
               _pConnection->send("\x06");
               Logger::logI("[lis_link] Send <ACK>");
               _state = lis::LinkState::Receiving;

               _tempReceiveBuffer.str("");

               startReceiveTimeoutTimer();
            }
            else
            {
               _pConnection->send("\x15");
               Logger::logI("[lis_link] Send <NAK>");
               Logger::logI("[lis_link] ConnectionDataReceived, Send <NAK> , LinkState: Idle , Cause: receive non <ENQ> when Idle");
            }
            break;
         case lis::LinkState::Sending:
         {
            _ackReceived = (ch == ACK);

            if (_ackReceived)
               Logger::logI("[lis_link] Read <ACK>");

            _cvWait_ACK.notify_one();
         }
         break;
         case lis::LinkState::Receiving:

            stopReceiveTimeoutTimer();
            startReceiveTimeoutTimer();

            if (ch == ENQ)
            {
               Logger::logI("[lis_link] Read <ENQ>");

               _pConnection->send("\x15");

               Logger::logI("[lis_link] Send <NAK>");
               Logger::logI("[lis_link] ConnectionDataReceived, Send <NAK> , LinkState: Receiving, cause: receive <ENQ> when receiving data");

               return;
            }

            if (ch == ETX)
               Logger::logI("[lis_link] Read <ETX>");

            if (ch == EOT)
            {
               stopReceiveTimeoutTimer();

               Logger::logI("[lis_link] Read <EOT>");
               Logger::logI("[lis_link] ----------------------------------------------------");

               _state = lis::LinkState::Idle;
               _cvWait_EOT.notify_one();

               return;
            }

            if (ch != '\0')
               _tempReceiveBuffer << ch;

            if (ch == LF)
            {
               std::string tempReceiveBuffer = _tempReceiveBuffer.str();
               if (checkChecksum(tempReceiveBuffer))
               {
                  Logger::logI("[lis_link] Read {}", strInRaw(tempReceiveBuffer));
                  Logger::logI("[lis_link] Send <ACK>");

                  _pConnection->send("\x06");

                  // Remove Low Level control characters
                  std::string cleanReceiveBuffer = tempReceiveBuffer.substr(2, tempReceiveBuffer.length() - 7);

                  if (onReceiveData)
                  {
                     // cleanReceiveBuffer still has RS character
                     std::string line = cleanReceiveBuffer;
                     onReceiveData(line);
                  }
                  _tempReceiveBuffer.str("");
               }
               else
               {
                  _pConnection->send("\x15");

                  Logger::logI("[lis_link] Send <NAK>");
                  Logger::logI("[lis_link] In ConnectionDataReceived(), Send <NAK> , LinkState: Receiving, cause: bad checksum ");

                  _tempReceiveBuffer.str("");
               }
            }
            break;
         case lis::LinkState::Establishing:
            if (ch == ACK)
            {
               Logger::logI("[lis_link] Read <ACK>");

               _state = lis::LinkState::Sending;
               _cvWait_ENQ.notify_one();

               return;
            }

            if (ch == ENQ)
            {
               Logger::logI("[lis_link] Read <ENQ>");
               std::this_thread::sleep_for(std::chrono::milliseconds(1000));
               Logger::logI("[lis_link] Send <ENQ>");
               _pConnection->send("\x05");

               return;
            }
            break;
         }
      }
   }
}

} // namespace bci
} // namespace tbs
