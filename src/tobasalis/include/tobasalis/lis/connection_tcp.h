#pragma once

#include <mutex>
#include <set>
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>
#include "tobasalis/lis/connection.h"
#include "tobasalis/lis/netsession.h"
#include "tobasalis/lis/netsession_manager.h"

using asio::ip::tcp;

namespace tbs {
namespace lis {

/** \ingroup LIS
 * TCPConnection
 */
class TCPConnection
   : public Connection
{
public:
   using AsioExecutor = asio::any_io_executor;
   using Executor     = asio::strand<AsioExecutor>;

   TCPConnection(
      asio::io_context& io_ctx,
      const std::string& address,
      const std::string& port,
      bool server = false);

   TCPConnection() = default;
   virtual ~TCPConnection() = default;

   virtual void connect();
   virtual void disConnect();
   virtual void send(const std::string& value);
   virtual void asyncSend(const std::string& data);
 
   virtual bool connected();

   void startListen();
   void doAccept();
   auto & executor() noexcept { return _executor; }

private:

   tcp::resolver     _resolver;
   tcp::acceptor     _acceptor;
   Executor          _executor;
   std::string       _ipaddr;
   std::string       _port;
   bool              _serverMode;

   NetsessionManager _sessionManager;
   NetSessionPtr     _pSession;
   bool              _connected;

   int               _retryConnectCount {0};

   // Netsession event handlers
   void Session_onReceiveData(const std::string& data);

   // tcp client mode handler
   void Session_onConnected(const std::string& data);
   void Session_onConnectFailed(const std::string& data);
   void Session_onClosed(const std::string& data);
   void Session_onDisconnected(const std::string& data);

   /** to string
    */
   template <typename T> std::string toString(const T& v)
   {
      std::ostringstream ostr;
      ostr << v ;
      return ostr.str() ;
   }   
};

} // namespace lis
} // namespace tbs 