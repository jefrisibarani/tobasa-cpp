#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include <tobasa/util_string.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/lis1a/datalink_base.h"

namespace tbs {
namespace lis1a {

DataLinkBase::DataLinkBase(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : lis::DataLink(io_ctx, connection, limit) 
   , _executor {io_ctx.get_executor()} {}

bool DataLinkBase::waitForACK()
{
   // Jika dalam 15 detik tidak dapat sinyal dari thread lain, jalankan stopSendMode()
   using sec = std::chrono::seconds;
   std::unique_lock<std::mutex> lock(_mx_ACK);
   if (_cvWait_ACK.wait_for(lock, sec(_limit.ackTimeOut)) == std::cv_status::timeout)
   {
      stopSendMode();
      throw tbs::AppException("No ACK response from Lis within timeout period.");
   }

   // Bila dalam 15 detik ada sinyal _cvWait_ACK.notify_one() laporkan ACK diterima
   // _ackReceived hanya bernilai TRUE bila kita menerima ACK
   return _ackReceived;
}

void DataLinkBase::establishSendMode()
{
   if (!_pConnection->connected())
      throw tbs::AppException("DataLink not connected");

   _frameNumber = 1;

   if (_state != lis::LinkState::Idle)
      throw tbs::AppException("DataLink connection not idle when trying to establish send mode");

   try
   {
      _state = lis::LinkState::Establishing;

      // Send ENQ, then wait ACK for 15 secs
      // see connectionOnReceiveData(), _cvWait_ENQ.notify_one() will stop the timer
      // in 15 second _state = lis::LinkState::Sending

      _pConnection->asyncSend({ENQ});
      Logger::logI("[lis_link] Send: <ENQ>, now waiting for <ACK> from remote system");

      using sec = std::chrono::seconds;
      std::unique_lock<std::mutex> lock(_mx_ENQ);
      if (_cvWait_ENQ.wait_for(lock, sec(_limit.enqTimeOut)) == std::cv_status::timeout)
      {
         Logger::logI("[lis_link] Idle after timeout waiting ACK from remote lis when establishing send mode");
         _state = lis::LinkState::Idle;

         throw tbs::AppException("DataLink, the LIMS/Instrument did not acknowledge the ENQ request");
      }

      if (_state != lis::LinkState::Sending)
      {
         stopSendMode();
         throw tbs::AppException("DataLink, failed to establish send mode ");
      }
      
      Logger::logI("[lis_link] Send mode established in data link");
   }
   catch (const tbs::AppException& e)
   {
      throw tbs::AppException(e);
   }
   catch (const std::exception& e)
   {
      Logger::logE("[lis_link] DataLink, establishSendMode() exception: {}, file datalink_base.cpp:{}", e.what(), __LINE__);
      throw tbs::AppException(tbsfmt::format("DataLink, error establishing send mode: {}",e.what()).c_str());
   }
}

void DataLinkBase::stopSendMode()
{
   if (_state == lis::LinkState::Idle)
      return;

   Logger::logI("[lis_link] Send: <EOT>, entering Idle state after send mode");
   _pConnection->asyncSend({EOT});
   _state = lis::LinkState::Idle;
}

void DataLinkBase::sendString(const std::string& data)
{
   if (_state == lis::LinkState::Sending)
   {
      try
      {
         int tryCounter = 0;

         std::string tempSendString = std::string{STX} + data + calculateChecksum(data) + std::string{CR} + std::string{LF};
         Logger::logI("[lis_link] Send: {}", strInRaw(tempSendString));
         _pConnection->asyncSend(tempSendString);

         while (!waitForACK())
         {
            // within 15 secs timeout, retry send if no ACK Received, for max 5 or (_limit.sendRetry) retries
            ++tryCounter;
            if (tryCounter > _limit.sendRetry)
            {
               stopSendMode();
               throw tbs::AppException("Max number of send retries reached.");
            }

            Logger::logI("[lis_link] Retrying send Lis string, attempt ({}): {}", tryCounter, strInRaw(tempSendString));
            _pConnection->asyncSend(tempSendString);
         }
         return;
      }
      catch (const std::exception & ex)
      {
         throw tbs::AppException(ex);
      }
   }

   throw tbs::AppException("Connection not in Send mode when trying to send data.");
}

void DataLinkBase::doSendMessage(const std::string& message)
{
   /*
   Frame contains a maximum of 64000 bytes (including frame overhead).
      Intermediate frame <STX> FN Text <ETB> C1 C2 <CR> <LF>
      End frame          <STX> FN Text <ETX> C1 C2 <CR> <LF>

      Note: FN = Frame Number
   */
   std::string msg = message;
   while (msg.length() > 63993)
   {
      std::string intermediateFrame = msg.substr(0, 63993);

      // send intermediate frame and append ETB
      Logger::logT("[lis_link] Send intermediate frame no: {}", _frameNumber);
      sendString(std::to_string(_frameNumber) + intermediateFrame + std::to_string(ETB) );

      msg = msg.erase(0, 63993);

      _frameNumber++;
      if (_frameNumber > 7)
         _frameNumber = 0;
   }

   if (_limit.sendRecordAsIntermediateFrame)
   {
      if (! util::startsWith(msg, {"L"}) )
      {
         // send end frame and append ETB
         Logger::logT("[lis_link] Send frame no: {}", _frameNumber);
         sendString(std::to_string(_frameNumber) + msg + ETB);
      }
      else
      {
         // Only Terminator Record use ETX
         // send end frame and append ETX
         Logger::logT("[lis_link] Send end frame no: {}", _frameNumber);
         sendString(std::to_string(_frameNumber) + msg + ETX);
      }
   }
   else
   {
      // send end frame and append ETX
      Logger::logT("[lis_link] Send end frame no: {}", _frameNumber);
      sendString(std::to_string(_frameNumber) + msg + ETX);
   }

   _frameNumber++;
   
   if (_frameNumber > 7)
      _frameNumber = 0;
}

} // namespace lis1a
} // namespace tbs