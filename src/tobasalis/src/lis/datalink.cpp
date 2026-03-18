#include <cctype>
#include <iostream>
#include <iomanip>
#include <thread>
#include <asio.hpp>
#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/lis/datalink.h"

namespace tbs {
namespace lis {

LinkLimit::LinkLimit()
{
   receiveTimeOut = 30;
   enqTimeOut     = 15;
   ackTimeOut     = 15;
   sendRetry      = 5;
   sendEachRecordInOneFrame        = false;
   sendRecordAsIntermediateFrame   = false;
   incomingDataAsIntermediateFrame = false;
}

DataLink::DataLink(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const LinkLimit& limit)
   : _receiveTimeOutTimer(io_ctx)
{
   setStatus(LinkState::Idle);
   setConnection(connection);
   _limit = limit;
}

LinkState DataLink::state()
{
   return _state;
}

std::string DataLink::stateStr()
{
   if (_state == LinkState::Idle)
      return "Idle";
   else if (_state == LinkState::Sending)
      return "Sending";
   else if (_state == LinkState::Receiving)
      return "Receiving";
   else if (_state == LinkState::Establishing)
      return "Establishing";
   else 
      return "Unknown";
}

bool DataLink::connected()
{
   if (_pConnection && _pConnection->connected())
      return true;
   else
      return false;
}

void DataLink::setStatus(LinkState state)
{
   _state = state;
}

void DataLink::setConnection(std::shared_ptr<lis::Connection> connection)
{
   _pConnection = connection;
   _pConnection->onReceiveData = std::bind(&DataLink::connectionOnReceiveData, this, std::placeholders::_1);
}

void DataLink::disConnect()
{
   if (_state != LinkState::Idle)
   {
      try
      {
         // Jika dalam 15 detik tidak dapat sinyal, periksa _state
         using sec = std::chrono::seconds;
         std::unique_lock<std::mutex> lock(_mx_EOT);
         if (_cvWait_EOT.wait_for(lock, sec(15)) == std::cv_status::timeout)
         {
            if (_state != LinkState::Idle)
               throw tbs::AppException("[lis_link] DataLink, error closing connection: no EOT received.");
         }
      }
      catch (const std::exception & e)
      {
         Logger::logD("[lis_link] DataLink disConnect() exception: {}, file datalink.cpp:{}", e.what(), __LINE__);
         throw tbs::AppException(tbsfmt::format("[lis_link] LIS DataLink, error closing connection: {}", e.what()).c_str() );
      }
   }

   try
   {
      _pConnection->disConnect();

      if (onLinkClosed)
         onLinkClosed("Lis Connection Closed");

      stopReceiveTimeoutTimer();
   }
   catch (std::exception &e)
   {
      Logger::logD("[lis_link] DataLink disConnect() exception: {}, file datalink.cpp:{}", e.what(), __LINE__);
      throw tbs::AppException( tbsfmt::format("LIS DataLink error closing connection: {}", e.what()).c_str() );
   }
}

void DataLink::connect()
{
   _pConnection->connect();
}

void DataLink::onTimerExpire(const asio::error_code& ec)
{
   if (ec != asio::error::operation_aborted)
   {
      Logger::logI("[lis_link] DataLink, receiveTimeOutTimer expired");

      _state = LinkState::Idle;
      stopReceiveTimeoutTimer();

      if (onReceiveTimeOut)
         onReceiveTimeOut("No incoming data within timeout");

      Logger::logI("[lis_link] Idle");
   }
}

void DataLink::stopReceiveTimeoutTimer()
{
   _receiveTimeOutTimer.cancel();
}

// Starts the Receive Timeout counter, see CLSI LIS01-A2 Chapter 6.5.2
void DataLink::startReceiveTimeoutTimer()
{
   _receiveTimeOutTimer.expires_after(std::chrono::seconds(_limit.receiveTimeOut));
   _receiveTimeOutTimer.async_wait(std::bind(&DataLink::onTimerExpire, this, std::placeholders::_1));
}

std::string DataLink::strInRaw(const std::string& data)
{
   std::string out;
   for (size_t i = 0; i < data.size(); ++i)
   {
      if (data[i] == '\x2')
         out.append("<STX>");
      else if (data[i] == '\x3')
         out.append("<ETX>");
      else if (data[i] == '\x17')
         out.append("<ETB>");
      
      else if (data[i] == '\x05')
         out.append("<ENQ>");
      else if (data[i] == '\x06')
         out.append("<ACK>");
      else if (data[i] == '\x15')
         out.append("<NAK>");
      else if (data[i] == '\x04')
         out.append("<EOT>");

      else if (data[i] == '\r')
         out.append("<CR>");
      else if (data[i] == '\n')
         out.append("<LF>");

      else if (data[i] == '\x1e')
         out.append("<RS>");
      else if (data[i] == '\x1d')
         out.append("<GS>");

      else if (data[i] == '\x00')
         out.append("<NUL>");

      else if (data[i] == '\x0B')
         out.append("<VT>");
      else if (data[i] == '\x1C')
         out.append("<FS>");
      else if (data[i] == '\0x10')
         out.append("<ENQ_HL7>");
      else if (data[i] == '\0x0F')
         out.append("<ETX_HL7>");

      else 
      {
         if (isprint(static_cast<unsigned char>(data[i])))
            out.append(1, data[i]);
         else 
         {
            out.append("<x");
            out.append(1, "0123456789ABCDEF"[(data[i] >> 4) & 0xF]);
            out.append(1, "0123456789ABCDEF"[data[i] & 0xF]);
            out.append(">");
         }
      }
   }

   return out;
}



} // namespace lis
} // namespace tbs