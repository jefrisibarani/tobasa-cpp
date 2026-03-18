#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/bind_executor.hpp>
#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include "tobasalis/lis/connection_tcp.h"

namespace tbs {
namespace lis {

TCPConnection::TCPConnection(
   asio::io_context&  io_ctx,
   const std::string& address,
   const std::string& port,
   bool               server)
   : _resolver { io_ctx }
   , _acceptor { io_ctx }
   , _executor { io_ctx.get_executor() }
   , _ipaddr   { address }
   , _port     { port }
   , _serverMode     { server }
   , _sessionManager {}
   , _connected      { false }
{}

void TCPConnection::connect()
{
   try
   {
      if (_serverMode)
      {
         Logger::logI("[lis_link] TCP connection, listening for incoming connection {}:{}", _ipaddr, _port);
         startListen();
      }
      else
      {
         // Client Mode part
         Logger::logI("[lis_link] TCP connection, initiating connection as client");
         using namespace std::placeholders;

         asio::ip::tcp::socket socket(_resolver.get_executor());
         auto client = _sessionManager.addClient(std::make_shared<NetSession>(std::move(socket), _sessionManager));
         client->onReceiveData   = std::bind(&TCPConnection::Session_onReceiveData, this, _1);
         client->onConnected     = std::bind(&TCPConnection::Session_onConnected, this, _1);
         client->onConnectFailed = std::bind(&TCPConnection::Session_onConnectFailed, this, _1);
         client->onClosed        = std::bind(&TCPConnection::Session_onClosed, this, _1);
         client->onDisconnected  = std::bind(&TCPConnection::Session_onDisconnected, this, _1);

         auto epResult = _resolver.resolve(_ipaddr, _port);
         client->asyncConnect(epResult);

         _pSession = client;
      }
   }
   catch (std::exception& e)
   {
      Logger::logD("[lis_link] TCPConnection connect() exception: {}", e.what());
      throw tbs::AppException(tbsfmt::format("LIS TCP Connection connect error: {}", e.what()).c_str());
   }
}

void TCPConnection::disConnect()
{
   if (_serverMode)
      _acceptor.close();

   _sessionManager.stopAll();

   _connected = false;

   Logger::logI("[lis_link] TCP connection is disconnected");
}

void TCPConnection::send(const std::string& value)
{
   if (!_pSession)
      throw AppException("LIS TCP connection is not connected");
   
   _pSession->send(value);
}

void TCPConnection::asyncSend(const std::string& data)
{
   if (!_pSession)
      throw AppException("LIS TCP connection is not connected");
   
   _pSession->asyncSend(data);
}

bool TCPConnection::connected()
{
   return _connected;
}

// -------------------------------------------------------
// Server mode parts

void TCPConnection::startListen()
{
   // we are in server mode
   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   auto results = _resolver.resolve(_ipaddr, _port);

   tcp::endpoint endpoint = results.begin()->endpoint();

   _acceptor.open(endpoint.protocol());
   _acceptor.set_option(tcp::acceptor::reuse_address(true));
   _acceptor.bind(endpoint);
   _acceptor.listen();

   doAccept();
}

void TCPConnection::doAccept()
{
   _acceptor.async_accept(
      asio::bind_executor(
         this->executor(),
         [this](std::error_code error, asio::ip::tcp::socket socket)
         {
            // Check whether the server was stopped by a signal before this
            // completion handler had a chance to run.
            if (!_acceptor.is_open())
               return;

            if (!error)
            {
               Logger::logI("[lis_link] Accepting connection from {}", toString(socket.remote_endpoint()) );

               // publish connected event
               if (onConnected)
                  onConnected("");

               auto server = _sessionManager.addListener(std::make_shared<NetSession>(std::move(socket), _sessionManager));
               server->onReceiveData = std::bind(&TCPConnection::Session_onReceiveData, this, std::placeholders::_1);
               server->startReceive();
               _pSession = server;
               _connected = true;
            }

            doAccept();
         }
      ));
}


// -------------------------------------------------------
// Netsession Event Handlers
void TCPConnection::Session_onReceiveData(const std::string& data)
{
   if (onReceiveData)
      onReceiveData(data);
}

void TCPConnection::Session_onConnected(const std::string& data)
{
   _connected = true;
   _retryConnectCount = 0;

   if (onConnected)
      onConnected(data);
}

void TCPConnection::Session_onConnectFailed(const std::string& data)
{
   if (_retryConnectCount < 3)
   {
      _retryConnectCount++;
      connect();
   }
   else
   {
      if (onConnectFailed)
         onConnectFailed(data);
   }
}

void TCPConnection::Session_onClosed(const std::string& data)
{
}

void TCPConnection::Session_onDisconnected(const std::string& data)
{
   if (!_serverMode)
   {
      // Reconnect
      connect();
   }
}


} // namespace lis
} // namespace tbs