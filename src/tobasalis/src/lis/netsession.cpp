#include <utility>
#include <asio.hpp>
#include <asio/query.hpp>
#include <asio/bind_executor.hpp>
#include <asio/ip/tcp.hpp>
#include <tobasa/logger.h>
#include "tobasalis/lis/netsession.h"
#include "tobasalis/lis/netsession_manager.h"

namespace tbs {
namespace lis {

NetSession::NetSession(tcp::socket socket, NetsessionManager& sessionManager)
   : _socket         { std::move(socket) }
   , _sessionManager { sessionManager }
   , _executor       { _socket.get_executor() }
{
   Logger::logT("[lis_link] Creating new NetSession");
   _listener = false;
   _closed = false;
}

void NetSession::startReceive()
{
   // connection might be already closed so we stop here.
   if ( _closed )
   {
      Logger::logT("[lis_link] Attempting to read data while closed");
      return;
   }

   _socket.async_read_some(
      asio::buffer(_readBufRaw, CONNECTION_READ_BUF_SIZE),
      asio::bind_executor(
         this->executor(),
         [this, self=shared_from_this()](std::error_code error, std::size_t bytesTransferred)
         {
            if (!error)
            {
               // connection might be already closed so we stop here.
               if ( _closed )
               {
                  Logger::logE("[lis_link] Attempting to read data while already closed");
                  return;
               }

               for (unsigned int i = 0; i < bytesTransferred; ++i)
               {
                  char c = _readBufRaw[i];

                  _readBufStr += c;
                  if (i == (bytesTransferred - 1))
                  {
                     try
                     {
                        if (onReceiveData)
                           onReceiveData(_readBufStr);
                     }
                     catch(const std::exception& ex)
                     {
                        _sessionManager.stop(self, ex.what());

                        if (!_listener && onDisconnected)
                        {
                           // Exception occurred in onReceiveData, most likely Message format error
                           // if we a a client, we need to reconnect
                           onDisconnected("");
                        }
                     }
                     
                     _readBufStr.clear();
                  }
               }
               startReceive();
            }
            else if (error == asio::error::operation_aborted)
            {
               Logger::logT("[lis_link] Netsession error when reading: ", error.message());
            }
            else if (error != asio::error::operation_aborted)
            {
               _sessionManager.stop(self, error.message());
               if (onDisconnected)
                  onDisconnected("");
            }
         }
      )
   );
}

void NetSession::shutdown()
{
   // Initiate graceful connection closure.
   asio::error_code ignored_ec;
   _socket.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void NetSession::close()
{
   if ( _closed )
   {
      Logger::logT("[lis_link] Socket Already closed");
      return;
   }

   asio::error_code error;
   _socket.lowest_layer().close(error);
   if (error) {
      Logger::logT("[lis_link] Error closing socket: {}", error.message() );
   }   

   shutdown();

   _closed = true;

   if (onClosed)
      onClosed("");
}

void NetSession::send(const std::string& data)
{
   asio::write(_socket, asio::buffer(data));
}

void NetSession::asyncSend(const std::string& data)
{
   asio::post(this->executor(),
      [this, data]()
      {
         bool _writeInProgress = !_writeDataQueue.empty();
         _writeDataQueue.push_back(data);
         if (!_writeInProgress)
            doAsyncSend();
      });
}

void NetSession::doAsyncSend()
{
   auto self(shared_from_this());

   asio::async_write(
      _socket, asio::buffer(_writeDataQueue.front()),
      asio::bind_executor(
         this->executor(),
         [this, self](std::error_code error, std::size_t bytes_transferred)
         {
            if (!error)
            {
               if (OnDataSent)
                  OnDataSent(bytes_transferred);

               _writeDataQueue.pop_front();
               if (!_writeDataQueue.empty())
                  doAsyncSend();
            }
            else if (error == asio::error::operation_aborted)
            {
               Logger::logT("[lis_link] Netsession error when writing: ", error.message());
            } 
            else if (error != asio::error::operation_aborted)
            {
               _sessionManager.stop(self, error.message());
               
               if (onDisconnected)
                  onDisconnected("");
            }
         }) 
   );
}

void NetSession::asyncConnect(tcp::resolver::results_type endpoints)
{
   Logger::logT("[lis_link] NetSession trying connect");

   asio::async_connect(
      _socket,
      endpoints,
      asio::bind_executor(
         this->executor(),
         std::bind(
               &NetSession::ConnectHandler,
               this,
               std::placeholders::_1,
               std::placeholders::_2   // connected endpoint
         )
      )
   );
}

/// Client mode async_connect handler
void NetSession::ConnectHandler(const std::error_code& error, const tcp::endpoint& endpoint)
{
   if (!error)
   {
      Logger::logI("[lis_link] NetSesion successfully connected to {}",  toString(endpoint) );
      // publish connected event
      if (onConnected)
         onConnected("");

      startReceive();
      return;
   }

   // All endpoints failed
   Logger::logE("[lis_link] NetSession connect error : {}", error.message());
   // publish connect failed event
   if (onConnectFailed)
      onConnectFailed(error.message() );
}

} // namespace lis
} // namespace tbs