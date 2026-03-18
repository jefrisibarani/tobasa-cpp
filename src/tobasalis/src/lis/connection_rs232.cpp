#include <iostream>
#include <functional>
#include <string>
#include <asio/thread.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/placeholders.hpp>
#include <asio/bind_executor.hpp>
#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include "tobasalis/lis/connection_rs232.h"

namespace tbs {
namespace lis {

RS232Connection::RS232Connection(asio::io_context& io_ctx)
   : _port      { io_ctx }
   , _executor  { io_ctx.get_executor() }
   , _connected {false} 
{}

void RS232Connection::startReceive()
{
   using namespace std::placeholders;

   _port.async_read_some(
      asio::buffer(_readBufRaw, CONNECTION_READ_BUF_SIZE),
      asio::bind_executor(
            this->executor(),
            std::bind(&RS232Connection::receiveDataHandler, this, _1, _2) 
      )
   );
}

void RS232Connection::receiveDataHandler(const std::error_code& ec, std::size_t  bytes_transferred)
{
   std::lock_guard<std::mutex> guard(_mutex);

   if (!_port.is_open())
      return;

   if (ec)
   {
      startReceive();
      return;
   }

   for (unsigned int i = 0; i < bytes_transferred; ++i)
   {
      char c = _readBufRaw[i];
      _readBufStr += c;

      if (i == (bytes_transferred - 1))
      {
         if (onReceiveData)
            onReceiveData(_readBufStr);
         
         _readBufStr.clear();
      }
   }

   startReceive();
}

void RS232Connection::connect()
{
   Logger::logI("[lis_link] RS232Connection, connecting");

   asio::error_code ec;
   _port.open(_portName, ec);

   if (ec)
   {
      std::string errmsg = tbsfmt::format("RS232Connection, error opening port: {}, {}", _portName, ec.message());

      if (onConnectFailed)
         onConnectFailed(errmsg);

      throw tbs::AppException(errmsg.c_str());
   }

   //Setup port
   _port.set_option(asio::serial_port_base::baud_rate(_baud));
   _port.set_option(asio::serial_port_base::character_size(8));
   _port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
   _port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
   _port.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

   if (_port.is_open())
   {
      _connected = true;
      
      Logger::logI("[lis_link] RS232Connection, {} connected", _portName);
      if (onConnected)
         onConnected("");

      startReceive();
   }
}

void RS232Connection::disConnect()
{
   _connected = false;
   _port.close();
   Logger::logI("[lis_link] RS232Connection, disconnected");
}

void RS232Connection::send(const std::string& text)
{
   asio::write(_port, asio::buffer(text));
}

bool RS232Connection::connected()
{
   return _connected;
}

} // namespace lis
} // namespace tbs