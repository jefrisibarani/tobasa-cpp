#pragma once

#include <memory>
#include <functional>
#include <asio/ip/tcp.hpp>
#include <tobasa/non_copyable.h>
#include "tobasahttp/type_common.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

enum class RequestStatus : std::uint8_t
{
   handled,
   notHandled,
   async
};

class Request;
class Response;
class MultipartBodyReader;


/** 
 * HTTP Context.
 */
class Context : private NonCopyable
{
private:
   std::shared_ptr<Request>   _request          {nullptr};
   std::shared_ptr<Response>  _response         {nullptr};
   asio::ip::tcp::endpoint    _remoteEndpoint;
   std::any                   _userData;
   ConnectionId               _connectionId     {0};
   uint32_t                   _requestId        {0};
   std::string                _sessionId        {};
   int32_t                    _requestHandlerId {0};
   bool                       _closed           {false};
   HttpVersion                _httpVersion      {HttpVersion::one};
   bool                       _keepAlive        {false};

#ifdef TOBASA_HTTP_USE_HTTP2
   int32_t                    _streamId         {-1};
#endif

   std::function<void(WebSocketContextPtr)> _webSockeInitHandler = nullptr;
   std::shared_ptr<MultipartBodyReader> _bodyReader = nullptr;
   

   // Called when request processing is done
   std::function<void(RequestStatus)> _onCompleteHandler = nullptr;

public:
   Context(
      std::shared_ptr<Request>    req
      , std::shared_ptr<Response> resp
      , asio::ip::tcp::endpoint   ep
      , ConnectionId              id
      , uint32_t                  reqId
      , HttpVersion               httpVersion
#ifdef TOBASA_HTTP_USE_HTTP2
      , int32_t                   streamId
#endif
      );

   ~Context();

   std::shared_ptr<Request>& request();

   std::shared_ptr<Response>& response();

   asio::ip::tcp::endpoint& remoteEndpoint();

   std::any& userData();

   void userData(std::any userData);

   ConnectionId connId();

   bool isHttps();

   std::string sessionId();

   void requestHandlerId(int32_t id) { _requestHandlerId = id; }

   int32_t requestHandlerId() { return _requestHandlerId; }

   bool closed() { return _closed;}

   HttpVersion httpVersion() { return _httpVersion; }

   bool keepAlive() const { return _keepAlive; }

   void keepAlive(bool value) { _keepAlive = value; }

   /// Called by HTTP request handler to set WebSocket context
   void webSocketContext(WebSocketContextPtr ctx);

   /// Only called by ServerConnection
   void webSocketInitHandler(std::function<void(WebSocketContextPtr)> handler);

   std::shared_ptr<MultipartBodyReader> getBodyReader();
   
   void setBodyReader(std::shared_ptr<MultipartBodyReader> reader);

   /// Only called by ServerConnection
   void onCompleteHandler(std::function<void(RequestStatus)> handler);

   void complete(RequestStatus status=RequestStatus::handled);

#ifdef TOBASA_HTTP_USE_HTTP2
   int32_t streamId() { return _streamId; }
#endif
};

/// Http Context
using HttpContext           = std::shared_ptr<Context>;

/// Http server request handler
using RequestHandler        = std::function<RequestStatus(const HttpContext&)>;

/// Http server request handler chained, used as middleware
/// \see AutoMiddleware 
using RequestHandlerChained = std::function<RequestStatus(const HttpContext&, const RequestHandler& /*next*/)>;

struct StatusPageData;
using StatusPageBuilder     = std::function<std::string(std::shared_ptr<StatusPageData>)>;

/** @}*/

} // namespace http
} // namespace tbs