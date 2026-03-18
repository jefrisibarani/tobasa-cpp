#pragma once

#include <mutex>
#include <thread>
#include <asio/strand.hpp>
#include <asio/serial_port.hpp>
#include <asio/steady_timer.hpp>
#include <asio/streambuf.hpp>
#include "tobasalis/lis/connection.h"

namespace tbs {
namespace lis {

/** \ingroup LIS
 * Serial port read buffer size
 */
constexpr auto SERIAL_PORT_READ_BUF_SIZE = 128;

/** \ingroup LIS
 * RS232Connection
 */
class RS232Connection
   : public Connection
{
public:
   using AsioExecutor = asio::any_io_executor;
   using Executor     = asio::strand<AsioExecutor>;

   RS232Connection(asio::io_context& io_ctx);
   RS232Connection() = default;
   virtual ~RS232Connection() = default;

   virtual void connect();
   virtual void disConnect();
   virtual void send(const std::string& value);
   virtual void asyncSend(const std::string& data) {send(data);}

   virtual bool connected();

   void setPortName(const std::string& port) { _portName = port; }
   void setBaudRate(int baud) { _baud = baud; }
   auto & executor() noexcept { return _executor; }


private:

   virtual void startReceive();

   asio::serial_port _port;
   Executor          _executor;
   std::mutex        _mutex;

   //Buffer in which to read the serial data
   asio::streambuf   _buffer;
   std::string       _portName;
   int               _baud;
   char              _readBufRaw[SERIAL_PORT_READ_BUF_SIZE];
   std::string       _readBufStr;
   bool              _connected;

   void receiveDataHandler(const std::error_code& ec, std::size_t  bytes_transferred);
};

} // namespace lis
} // namespace tbs 