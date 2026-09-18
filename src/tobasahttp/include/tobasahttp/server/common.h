#pragma once

#include <memory>
#include <functional>
#include <asio/ip/tcp.hpp>
#include <tobasa/non_copyable.h>
#include "tobasahttp/type_common.h"
#include "tobasahttp/sse.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * Result returned by an HTTP request handler.
 */
enum class RequestStatus : std::uint8_t
{
   /// The handler processed the request and prepared a response.
   handled,

   /// The handler did not process the request.
   notHandled,

   /// Request processing continues asynchronously and will complete later.
   async
};

class Request;
class Response;
class MultipartBodyReader;


/**
 * \brief Request and response context for one HTTP request.
 *
 * Context is passed to the application request handler. It provides access to
 * the request, response, remote endpoint, application data, request-body
 * reader, and connection settings such as keep-alive.
 *
 * The handler can use this context to complete request processing or to start
 * a WebSocket or Server-Sent Events connection. With HTTP/2, the context also
 * stores the stream ID that identifies the request within the connection.
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

   /// Reads the request body incrementally, including multipart request data.
   std::shared_ptr<MultipartBodyReader>      _bodyReader = nullptr;

   /// Called when the request handler reports that request processing is done.
   std::function<void(RequestStatus)>        _onCompleteHandler = nullptr;

   /// Called when the request handler accepts a WebSocket connection.
   std::function<void(WebSocketContextPtr)>  _webSockeInitHandler = nullptr;

   /// Called when the request handler starts a Server-Sent Events connection.
   std::function<void(SseContextPtr)>        _sseInitHandler = nullptr;

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

   /// Returns the application-specific data stored in this HTTP context.
   std::any& userData();

   /// Stores application-specific data in this HTTP context.
   void userData(std::any userData);

   ConnectionId connId();

   bool isHttps();

   std::string sessionId();

   void requestHandlerId(int32_t id) { _requestHandlerId = id; }

   int32_t requestHandlerId() { return _requestHandlerId; }

   bool closed() { return _closed;}

   HttpVersion httpVersion() { return _httpVersion; }

   /// Returns whether the connection should remain open for another request.
   bool keepAlive() const { return _keepAlive; }

   /// Sets whether the connection should remain open after the response.
   void keepAlive(bool value) { _keepAlive = value; }

   /// Returns the reader used to consume the request body incrementally.
   std::shared_ptr<MultipartBodyReader> getBodyReader();

   /// Sets the reader used to consume the request body, including multipart data.
   void setBodyReader(std::shared_ptr<MultipartBodyReader> reader);

   /// Only called by ServerConnection
   void onCompleteHandler(std::function<void(RequestStatus)> handler);

   /// Completes request processing and invokes the registered completion handler.
   /// The default status means that the request was handled successfully.
   void complete(RequestStatus status=RequestStatus::handled);

   /// Called by HTTP request handler to set WebSocket context
   void webSocketContext(WebSocketContextPtr ctx);

   /// Only called by ServerConnection
   void webSocketInitHandler(std::function<void(WebSocketContextPtr)> handler);

   /// Called by HTTP request handler to set SSE context
   void sseContext(SseContextPtr context);

   /// Only called by ServerConnection
   void sseInitHandler(std::function<void(SseContextPtr)> handler);

#ifdef TOBASA_HTTP_USE_HTTP2
   int32_t streamId() { return _streamId; }
#endif
};

/// Shared pointer to an HTTP request and response context.
using HttpContext           = std::shared_ptr<Context>;

/// Handles one HTTP request and returns its processing status.
/// The handler uses HttpContext to read the request and prepare the response.
using RequestHandler        = std::function<RequestStatus(const HttpContext&)>;

/// Middleware handler that receives the current request and the next handler.
/// It can process the request itself or call next to continue the chain.
using RequestHandlerChained = std::function<RequestStatus(const HttpContext&, const RequestHandler& /*next*/)>;


struct StatusPageData;

/// Builds the response body for an HTTP status page.
/// ServerConnection calls this callback when a custom error page is configured.
using StatusPageBuilder     = std::function<std::string(std::shared_ptr<StatusPageData>)>;

/** @}*/

} // namespace http
} // namespace tbs