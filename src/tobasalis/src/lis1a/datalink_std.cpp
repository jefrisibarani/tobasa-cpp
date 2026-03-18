#include <iomanip>
#include <thread>
#include <asio.hpp>
#include <asio/bind_executor.hpp>
#include <tobasa/logger.h>
#include <tobasa/util_string.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/lis1a/datalink_std.h"

namespace {
   static int receiveNonEnqWhenIdle = 0;
} // anonymous namesapace


namespace tbs {
namespace lis1a {

DataLinkStd::DataLinkStd(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : DataLinkBase(io_ctx, connection, limit) {}

/*
<STX>4R|1|^^^Glucose|0|mg/dL^B<CR><ETB>7B<CR><LF>
calculateChecksum calculate after <STX> up to and include <ETB> or <ETX>
param @data characters after <STX> up to and include <ETB> or <ETX>
*/
std::string DataLinkStd::calculateChecksum(const std::string& data)
{
   int total = 0;
   if (data.length() > 0)
   {
      for (std::string::const_iterator it = data.begin(); it < data.end(); ++it)
      {
         char i = *it;
         total += (int)i;
      }
   }

   int checksum = total % 256;

   std::stringstream ss;
   ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << checksum;
   return ss.str();
}

bool DataLinkStd::checkChecksum(const std::string& line)
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

   char etxOrEtb = line[lineLength - 5];
   if (etxOrEtb != ETX && etxOrEtb != ETB)
      return result;

   _lastFrameWasIntermediate = (etxOrEtb == ETB);
   
   if (line[lineLength - 6] != CR)
      return result;

   std::string tempChecksum   = line.substr(lineLength - 4, 2);
   std::string tempLine       = line.substr(1, lineLength - 5);
   std::string tempCheckSum2  = calculateChecksum(tempLine);

   return !(tempChecksum != tempCheckSum2) || result;
}

void DataLinkStd::connectionOnReceiveData(const std::string& data)
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
               // Ada Sinyal ENQ dari host lain, kirim sinyal ACK, lalu masuk ke mode RECEIVING
               Logger::logI("[lis_link] Read: <ENQ>");
               _pConnection->asyncSend({ACK});
               Logger::logI("[lis_link] Send: <ACK>");

               _state = lis::LinkState::Receiving;

               _tempReceiveBuffer.str("");
               _tempIntermediateFrameBuffer = "";

               // segera aktifkan timer selama 15 detik, menunggu respon berikutnya dari Pengirim
               // bila tak ada respon dalam 15 detik, kembalikan state IDLE
               startReceiveTimeoutTimer();
            }
            else
            {
               // we may get into loop, when other party also in IDLE state
               receiveNonEnqWhenIdle++;
               if (receiveNonEnqWhenIdle > 3 )
               {
                  if (receiveNonEnqWhenIdle <= 6)
                  {
                     _pConnection->asyncSend({NAK});
                     Logger::logI("[lis_link] Send: <NAK>, because non <ENQ> received when Idle");
                     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                  }
                  else
                  {
                     // reset counter, and stop the loop by not sending NAK
                     receiveNonEnqWhenIdle = 0;
                  }
               }
               else
               {
                  _pConnection->asyncSend({NAK});
                  Logger::logI("[lis_link] Send: <NAK>, because non <ENQ> received when Idle");
               }
            }
            break;
         case lis::LinkState::Sending:
         {
            _ackReceived = (ch == ACK);

            if (_ackReceived) {
               Logger::logI("[lis_link] Read: <ACK>");
            }

            _cvWait_ACK.notify_one();
         }
         break;
         case lis::LinkState::Receiving:

            stopReceiveTimeoutTimer();
            startReceiveTimeoutTimer();

            if (ch == ENQ)
            {
               Logger::logI("[lis_link] Read: <ENQ>");
               _pConnection->asyncSend({NAK});
               Logger::logI("[lis_link] Send: <NAK>, because <ENQ> received when receiving data");
               return;
            }

            if (ch == EOT)
            {
               stopReceiveTimeoutTimer();

               Logger::logI("[lis_link] Read: <EOT>, entering Idle state");
               Logger::logI("[lis_link] ----------------------------------------------------");

               _state = lis::LinkState::Idle;
               
               _cvWait_EOT.notify_one();

               if (onIdle) {
                  onIdle(_pConnection);
               }
               
               return;
            }

            if (ch != '\0') {
               _tempReceiveBuffer << ch;
            }

            if (ch == LF)
            {
               std::string tempReceiveBuffer = _tempReceiveBuffer.str();

               if (checkChecksum(tempReceiveBuffer))
               {
                  Logger::logI("[lis_link] Read: {}", strInRaw(tempReceiveBuffer));
                  Logger::logI("[lis_link] Send: <ACK>");

                  _pConnection->asyncSend({ACK});

                  // Remove Low Level control characters
                  std::string cleanReceiveBuffer = tempReceiveBuffer.substr(2, tempReceiveBuffer.length() - 7);
                  if (_lastFrameWasIntermediate) {
                     _tempIntermediateFrameBuffer += cleanReceiveBuffer;
                  }
                  else if (onReceiveData)
                  {
                     // send data to parser
                     std::string line = _tempIntermediateFrameBuffer + cleanReceiveBuffer;
                     onReceiveData(line); 
                     
                     _tempIntermediateFrameBuffer = "";

                     if (_limit.incomingDataAsIntermediateFrame)
                     {
                        // some LIS always send data as intermediate frame, followed with last end frame
                        // <STX> FN Text <ETB> C1 C2 <CR> <LF>
                        // so we reset _tempIntermediateFrameBuffer here
                        _tempIntermediateFrameBuffer = ""; 
                     }
                  }
                  _tempReceiveBuffer.str("");

                  if (!_limit.incomingDataAsIntermediateFrame) {
                     _tempIntermediateFrameBuffer = "";
                  }
               }
               else
               {
                  _pConnection->asyncSend({NAK});
                  Logger::logI("[lis_link] Send: <NAK>, because bad checksum in received data");
                  _tempReceiveBuffer.str("");
               }
            }
            break;
         case lis::LinkState::Establishing:
            if (ch == ACK)
            {
               Logger::logI("[lis_link] Read: <ACK>");
               _state = lis::LinkState::Sending;
               _cvWait_ENQ.notify_one();

               return;
            }

            if (ch == ENQ)
            {
               // If incoming Char is ENQ = Contention (see 6.2.7.1 in LIS01-A2)
               Logger::logI("[lis_link] Read: <ENQ>");
               std::this_thread::sleep_for(std::chrono::milliseconds(1000));
               Logger::logI("[lis_link] Send: <ENQ>");
               _pConnection->asyncSend({ENQ});

               return;
            }
            break;
         }
      }
   }
}

void DataLinkStd::sendMessage(const std::string& messageStr, const std::string& endOfLine)
{
   Logger::logD("[lis_link] Sending Lis message string");

   if (messageStr.length() == 0)
      return;

   try
   {
      establishSendMode();

      // set new line character for splitting message
      std::string newLineCharacter;
      if ( endOfLine == "CR" )
         newLineCharacter = "\r";
      else if ( endOfLine == "LF")
         newLineCharacter = "\n";
      else
         newLineCharacter = "\r\n";

      std::string fullMessage;
      std::vector<std::string> records = tbs::util::split(messageStr, newLineCharacter,/*keepEmpty*/false);
      int sendCounter = 0;
      size_t recCount = records.size();

      std::vector<std::string>::const_iterator it;
      for (it = records.begin(); it != records.end(); ++it)
      {
         std::string record = *it;

         // Note: To send LIS2-A2 messave over wire, use CR as record separator
         record.append(1, '\r');

         if ( _limit.sendEachRecordInOneFrame )
         {  
            // Send each record in it own frame
            doSendMessage(record);
            double progess = (double)sendCounter / (double)recCount;

            //if (onSendProgress)
            //   onSendProgress(progess);

            sendCounter++;
         }
         else
            fullMessage += record;
      }

      // Send all records in one Frame(inside one <STX>.....<ETX>)
      if (! _limit.sendEachRecordInOneFrame )
         doSendMessage(fullMessage);
   }
   catch (std::exception & ex)
   {
      Logger::logE("[lis_link] Error sending record: {}", ex.what());
   }

   // make sure to exit send mode
   stopSendMode();
}

void DataLinkStd::sendMessage(const std::vector<std::shared_ptr<lis::Record>>& messageVec)
{
   Logger::logD("[lis_link] Sending Lis records vector");

   if (messageVec.size() == 0)
      return;

   try
   {
      establishSendMode();

      std::string fullMessage;
      int sendCounter = 0;
      size_t recCount = messageVec.size();

      std::vector<std::shared_ptr<lis::Record>>::const_iterator it;
      for (it = messageVec.begin(); it != messageVec.end(); ++it)
      {
         auto record = *it;
         std::string _msg = record->toString();

         if ( _limit.sendEachRecordInOneFrame )
         {
            // Send each record in it own frame
            doSendMessage(_msg);
            double progess = (double)sendCounter / (double)recCount;

            //if (onSendProgress)
            //   onSendProgress(progess);

            sendCounter++;
         }
         else
            fullMessage += _msg;
      }

      // Send all records in one Frame(inside one <STX>.....<ETX>)
      if (! _limit.sendEachRecordInOneFrame)
         doSendMessage(fullMessage);
   }
   catch (std::exception & ex)
   {
      Logger::logE("[lis_link] error sending record: {}", ex.what());
   }

   // make sure to exit send mode
   stopSendMode();
}

} // namespace lis1a
} // namespace tbs