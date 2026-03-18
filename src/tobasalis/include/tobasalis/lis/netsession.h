#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>
#include <tobasa/non_copyable.h>
#include "tobasalis/lis/connection.h"

namespace tbs {
namespace lis {

typedef std::deque<std::string> DataQueue;

using asio::ip::tcp;
class NetsessionManager;

/** \ingroup LIS
 * NetSession
 */
class NetSession
   : public std::enable_shared_from_this<NetSession>
   , private NonCopyable
{
public:
   using AsioExecutor = asio::any_io_executor;
   using Executor     = asio::strand<AsioExecutor>;

   explicit NetSession(tcp::socket socket, NetsessionManager& sessionManager);
   ~NetSession() = default;

   tcp::socket& socket() { return _socket; }
   void startReceive();
   void send(const std::string& data);
   void asyncSend(const std::string& data);
   void doAsyncSend();
   void close();

   /// Connect in client mode
   void asyncConnect(tcp::resolver::results_type endpoints);

   std::function<void(const std::string&)> onReceiveData;
   std::function<void(std::size_t val)>    OnDataSent;
   std::function<void(const std::string&)> onConnected;
   std::function<void(const std::string&)> onConnectFailed;
   std::function<void(const std::string&)> onClosed;
   std::function<void(const std::string&)> onDisconnected;

   bool isListener() { return _listener; }
   void setListener() { _listener = true; }
   
   auto & executor() noexcept { return _executor; }

private:
   tcp::socket        _socket;
   char               _readBufRaw[CONNECTION_READ_BUF_SIZE]; ///< Buffer for incoming data.
   std::string        _readBufStr;
   bool               _listener;
   NetsessionManager& _sessionManager;
   Executor           _executor;
   DataQueue          _writeDataQueue;
   bool               _writeInProgress;
   bool               _closed { true };

   void ConnectHandler(const std::error_code& error, const tcp::endpoint& endpoint);
   void shutdown();

   /** to string
    */
   template <typename T> std::string toString(const T& v)
   {
      std::ostringstream ostr;
      ostr << v ;
      return ostr.str() ;
   }
};

typedef std::shared_ptr<NetSession> NetSessionPtr;

} // namespace lis
} // namespace tbs 