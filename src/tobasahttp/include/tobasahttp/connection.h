#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <asio/ip/tcp.hpp>
#include "tobasahttp/type_common.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Base class for connection.
 */
class Connection
   : public std::enable_shared_from_this<Connection>
{
protected:
   ConnectionId _connId       { 0 };
   bool         _closed       { true };
   bool         _isTls        { false };
   bool         _isWebSocket  { false };
   std::string  _identifier;

   InstanceType _instanceType;

   OnComplete   _onComplete;
   OnError      _onError;
   OnTimeOut    _onTimeOut;
   OnClosed     _onClosed;

   std::chrono::system_clock::time_point _startTime;

   HttpVersion  _httpVersion = HttpVersion::one;

public:
   Connection();

   virtual ~Connection();

   void id(ConnectionId id);

   ConnectionId id() const;

   virtual void start() = 0;

   /// @brief Closes the connection.
   /// This function closes the underlying socket and stops any associated timers.
   virtual void close() = 0;

   /// @brief Closes the connection by calling registered OnComplete handler
   virtual void callClose(const std::string& reason="");

   virtual bool closed() const;

   virtual bool isTls() { return _isTls; }

   virtual bool isWebSocket() { return _isWebSocket; }

   virtual void setTls(bool tls) { _isTls = tls; }

   virtual void identifier(const std::string& id)  { _identifier = id; }
   virtual std::string identifier() { return _identifier; }

   void instanceType(InstanceType value) {_instanceType = value; }

   InstanceType instanceType() { return _instanceType;}

   virtual asio::ip::tcp::endpoint remoteEndpoint() = 0;

   std::chrono::system_clock::time_point startTime() { return _startTime; }

   HttpVersion httpVersion() { return _httpVersion; }
   void httpVersion(HttpVersion version) { _httpVersion = version; }

   void onComplete(OnComplete handler);
   void onError(OnError handler);
   void onTimeOut(OnTimeOut handler);
   void onClosed(OnClosed handler);

   virtual void wsSendBinary(const std::string& data, WsSendErrorHandler callback = nullptr) {}
   virtual void wsSendText(  const std::string& data, WsSendErrorHandler callback = nullptr) {}
   virtual void wsSendClose( int status, const std::string &reason = "", WsSendErrorHandler callback = nullptr) {}
};

/** @}*/

} // namespace http
} // namespace tbs