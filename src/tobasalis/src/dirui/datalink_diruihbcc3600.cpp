#include <tobasa/logger.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/dirui/datalink_diruihbcc3600.h"


namespace tbs {
namespace dirui {

DataLinkDirUiBcc3600::DataLinkDirUiBcc3600(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : lis::DataLink(io_ctx, connection, limit) {}

void DataLinkDirUiBcc3600::connectionOnReceiveData(const std::string& data)
{
   std::string buffer = data;
   if (buffer.length() > 0)
   {
      for (std::string::iterator it = buffer.begin(); it < buffer.end(); ++it)
      {
         char ch = *it;
         switch (_state)
         {
            // ENQ : Handshake activated
            // STX : Handshake deactivate
         case lis::LinkState::Idle:
            if (ch == ENQ)
            {
               // Ada Sinyal ENQ dari host lain, kirim sinyal ACK, lalu masuk ke mode RECEIVING
               Logger::logI("[lis_link] Read: <ENQ>");
               _pConnection->send("\x06");
               Logger::logI("[lis_link] Send: <ACK>");

               _state = lis::LinkState::Receiving;
               _tempReceiveBuffer.str("");

               // segera aktifkan timer selama 15 detik, menunggu respon berikutnya dari Pengirim
               // bila tak ada respon dalam 15 detik, kembalikan state IDLE
               startReceiveTimeoutTimer();
            }
            else if (ch == STX)
            {
               // Ada Sinyal STX dari host lain, masuk ke mode RECEIVING
               Logger::logI("[lis_link] Read: <STX>");
               _state = lis::LinkState::Receiving;
               _tempReceiveBuffer.str("");

               // segera aktifkan timer selama 15 detik, menunggu respon berikutnya dari Pengirim
               // bila tak ada respon dalam 15 detik, kembalikan state IDLE
               startReceiveTimeoutTimer();
            }
            else
            {
               _pConnection->send("\x15");
               Logger::logI("[lis_link] Send: <NAK>");
               Logger::logI("[lis_link] DirUiBcc3600DataLink: ConnectionDataReceived, Send <NAK> , LinkState: Idle , Cause :Receive non <ENQ> when Idle");
            }
         break;
         
         case lis::LinkState::Receiving:

            stopReceiveTimeoutTimer();
            startReceiveTimeoutTimer();

            if (ch == ENQ)
            {
               Logger::logI("[lis_link] Read: <ENQ>");
               _pConnection->send("\x15");
               Logger::logI("[lis_link] Send: <NAK>");
               Logger::logI("[lis_link] DirUiBcc3600DataLink: ConnectionDataReceived, Send <NAK> , LinkState: Receiving, Cause: Receive <ENQ> when receiving data");

               return;
            }

            if (ch != NUL) {
               _tempReceiveBuffer << ch;
            }

            // Handshake activated mode
            if (ch == ETX)
            {
               stopReceiveTimeoutTimer();

               std::string tempReceiveBuffer = _tempReceiveBuffer.str();

               // Remove Low Level control characters EOT and ETX
               std::string cleanReceiveBuffer = tempReceiveBuffer.substr(0, tempReceiveBuffer.length() - 2);

               _tempReceiveBuffer.str(""); // reset _tempReceiveBuffer

               Logger::logI("[lis_link] Read: <ETX>");
               _pConnection->send("\x06");
               Logger::logI("[lis_link] Send: <ACK>");
               Logger::logI("[lis_link] ----------------------------------------------------");
               _state = lis::LinkState::Idle;
               _cvWait_EOT.notify_one();

               if (onReceiveData)
                  onReceiveData(cleanReceiveBuffer);

               return;
            }

            // Handshake deactivated mode
            if (ch == _EOF)
            {
               stopReceiveTimeoutTimer();

               std::string tempReceiveBuffer = _tempReceiveBuffer.str();

               // Remove Low Level control characters EOF
               std::string cleanReceiveBuffer = tempReceiveBuffer.substr(0, tempReceiveBuffer.length() - 1);

               _tempReceiveBuffer.str(""); // reset _tempReceiveBuffer

               Logger::logI("[lis_link] Read: <EOF>");
               Logger::logI("[lis_link] ----------------------------------------------------");
               _state = lis::LinkState::Idle;
               _cvWait_EOT.notify_one();

               if (onReceiveData)
                  onReceiveData(cleanReceiveBuffer);
            }
         }
         break;
      }
   }
}

} // namespace dirui
} // namespace tbs