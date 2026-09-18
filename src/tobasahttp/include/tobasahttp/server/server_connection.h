#pragma once

#include <list>
#include <string>
#include <vector>
#include <unordered_set>
#include <tobasa/base64.h>
#include <tobasa/crypt.h>
#include <tobasa/span.h>
#include <tobasa/util_string.h>
#include "tobasahttp/server/common.h"
#include "tobasahttp/http_connection.h"
#include "tobasahttp/util.h"
#include "tobasahttp/server/status_page.h"
#include "tobasahttp/websocket.h"
#include "tobasahttp/multipart_body_reader.h"
#include "tobasahttp/multipart_parser.h"

#ifdef TOBASA_HTTP_USE_HTTP2
   #include <map>
   #include "tobasahttp/server/http2.h"
#endif

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

// https://datatracker.ietf.org/doc/html/rfc7231#section-4

// common methods : GET, POST, PUT, DELETE, HEAD, OPTIONS, CONNECT, TRACE , PATCH
inline static const std::unordered_set<std::string>& knownHttpMethods()
{
   static const std::unordered_set<std::string> knownMethods = {
      "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "CONNECT" /*, "TRACE", "PATCH" */
   };
   return knownMethods;
}; 

inline bool isKnownHttpMethod(const std::string& method) 
{
   return knownHttpMethods().count(method) > 0;
}

inline std::string knownHttpMethodsCsv() 
{
   std::ostringstream oss;
   for (auto it = knownHttpMethods().begin(); it != knownHttpMethods().end(); ) 
   {
      oss << *it;
      if (++it != knownHttpMethods().end()) oss << ", ";
   }
   return oss.str();
}



// Handling Two Phase Lookup
// note: https://www.modernescpp.com/index.php/surprise-included-inheritance-and-member-functions-of-class-templates
// Two-phase lookup ensures that names are resolved 
// correctly in the context of both template definition and instantiation. 
// Using `this->` helps the compiler understand 
// that a name is a member of the current class or its base classes, 
// which is crucial for correct name resolution in templates.

/**
 * \brief HTTP server connection for one connected client.
 *
 * This class owns the socket for one client connection to the server. It reads
 * raw bytes, parses HTTP requests, builds an HttpContext, and passes it to the
 * app request handler. It also handles body parsing, keep-alive, upgrades,
 * SSE, and WebSocket transitions.
 *
 * For HTTP/1, it parses a request, runs the app handler, and writes the
 * response. A keep-alive connection can then process more requests. For
 * upgrade requests, it detects the Upgrade header, validates the request,
 * builds the handshake response, and switches the connection into a
 * WebSocketState or SSE state for ongoing frame/stream I/O.
 *
 * For HTTP/2, several streams can be active and interleaved on the same
 * connection. The HTTP/2 session and stream ID are used to keep each request,
 * response, WebSocket, and SSE stream separate.
 *
 */
template <class Traits>
class ServerConnection
   : public HttpConnection<Traits>
{
public:
   using Socket   = typename Traits::Socket;
   using Settings = typename Traits::Settings;
   using Logger   = typename Traits::Logger;

   ServerConnection(const ServerConnection&) = delete;
   ServerConnection& operator=(const ServerConnection&) = delete;

private:
   RequestHandler&         _requestHandler;
   StatusPageBuilder       _statusPageBuilder;

   /// Current HTTP/1 request context available to the request handler.
   /// Replaced for each request on a keep-alive connection; 
   /// HTTP/2 requests use separate per-stream contexts instead.
   HttpContext             _httpContext            { nullptr };

   int64_t                 _totalBytesTransferred  { 0 };

   /// Counts HTTP/1 requests handled on this connection. It is used as the
   /// parser/request ID and to enforce maxRequestsPerConnection. HTTP/2 uses
   /// the stream ID to identify each request.
   std::atomic<uint32_t>   _currentRequestId       { 0 };

#ifdef TOBASA_HTTP_USE_HTTP2
   http2::Http2OptionPtr   _http2Option            { nullptr };
   http2::Http2SessionPtr  _http2Session           { nullptr };

   struct Http2WebSocketConnection
   {
      ws::WebSocketStateUPtr state;
   };
   std::map<int32_t, std::shared_ptr<Http2WebSocketConnection>> _http2WebSockets;
#endif

   // WebSocket State
   ws::WebSocketStateUPtr   _wsState;

   //Server-Sent Events (SSE) State
   sse::SseStateUPtr        _sseState;

   CompressionRule compressionRule(const std::string& acceptEncoding)
   {
      CompressionRule rule;
      rule.useCompression  = this->_settings.useCompression();
      rule.minimalBodySize = this->_settings.compressionMinimalLength();
      rule.encoding        = this->_settings.compressionEncoding();
      rule.acceptEncoding  = acceptEncoding;
      rule.mimetypes       = this->_settings.compressionMimeTypes();
      return rule;
   }

public:

   /// @brief Create ServerConnection instance
   /// 
   explicit ServerConnection(
         Socket          socket,
         Settings&       settings,
         Logger&         logger,
         RequestHandler& handler)
      : HttpConnection<Traits> { std::move(socket), settings, logger }
      , _requestHandler { handler }
   {
      this->_instanceType = InstanceType::http_server;
      this->_parser.type(parser::Type::REQUEST);

      this->_parser.onValidateRequestMethod =
         std::bind(&ServerConnection::validateRequestMethod, this,
            std::placeholders::_1, std::placeholders::_2);

      this->_parser.onValidateHeaders =
         std::bind(&ServerConnection::validateHeaders, this,
            std::placeholders::_1, std::placeholders::_2);

      this->_remoteEndpoint = this->_socket.lowest_layer().remote_endpoint();

      this->_parser.onEventDone = [&](const std::string evtType) {
         this->_processingStopWatch.lap(this->_parser.parsingId(),evtType);
      };

   }
 
   virtual ~ServerConnection()
   {
      this->_logger.trace("[{}] [conn:{}] Destroyed", this->logHttpType(), this->id());

#ifdef TOBASA_HTTP_USE_HTTP2
      if(this->_http2Session!=nullptr)
         this->_http2Session->close();
#endif
   }

   void statusPageBuilder(StatusPageBuilder renderer)
   {
      _statusPageBuilder = renderer;
   }

   /// start() called by connection manager, right after set this instance id
   virtual void start()
   {
      this->_logger.trace("[{}] [conn:{}] Starting", this->logHttpType(), this->id());

      this->_parser.connId( this->id() ); // set id, mainly for debugging for now
      // set parsing id with current request id for this connection
      // wh we call Parser's prepareForNextMessage(), we must update this parsing id
      this->_parser.parsingId( ++_currentRequestId );

      this->_processingStopWatch.start();
      this->_processingStopWatch.lap(this->_parser.parsingId(), "start");

      // call onStart handler, passing socket and the callback
      this->onStart(this->_socket,
         // OnStartCallback
         [this] (const std::error_code& error)
         {
            if (!error)
            {
               this->_logger.trace("[{}] [conn:{}] Start reading request from {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint) );

#ifdef TOBASA_HTTP_USE_HTTP2
               if (this->httpVersion() == HttpVersion::two)
                  startHttp2();
               else
                  startHttp1();
#else
               startHttp1();
#endif
            }
            else
            {  
               if (!this->closed())
                  this->processError(this->id(), error, ErrorType::system, "ServerConnection");
            }
         }
      );
   }

   virtual void write()
   {
      if (_sseState)
         return;

      this->_logger.trace("[{}] [conn:{}] Sending response to {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));

      if (!_httpContext->response()->preparedForCompression()) 
      {
         auto accept = _httpContext->request()->headers().value("Accept-Encoding");
         _httpContext->response()->prepareForCompression(compressionRule(accept));
      }

      ResponseSerializer serializer( _httpContext->response() );
      
      try
      {
         auto bytesToTransfer = serializer.serializeHttp1(&this->_sendBuffer, this->_settings.sendBufferSize());
         if (bytesToTransfer == 5 && _httpContext->response()->useChunkedEncoding())
         {
            this->_logger.trace("[{}] [conn:{}] Chunked transfer completed", this->logHttpType(), this->id());
            //return handleKeepAliveOrClose(); 
         }
      }
      catch(const std::exception& ex)
      {
         this->processError(this->id(), ex.what(), ErrorType::system, 5000,"ServerConnection");
         return;
      }

      this->startTimer(this->timeoutWrite());
      asio::async_write(
         this->_socket,
         this->_sendBuffer,
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr()] (std::error_code error, std::size_t byteTransferred)
            {
               auto resp = self->_httpContext->response();
               resp->updateTotalTransferred(byteTransferred);

               // consume the bytes we just wrote from the send buffer
               if (byteTransferred > 0)
                  self->_sendBuffer.consume(byteTransferred);
               
               if (error)
               {
                  if (error == asio::error::operation_aborted) 
                  {
                     // cancelled locally (server-side).Likely socket closed before write finished
                  }
                  else if (error == asio::error::connection_reset || error == asio::error::eof) 
                  {
                     // client closed connection, safe to ignore
                     if (resp->totalTransferred() == resp->expectedTotalTransferred())
                     {
                        self->markRequestCompletedAndLog();
                        self->processCompleted(self->id(), error.message());
                        return;
                     }
                  } 

                  if (!self->closed())
                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");

                  return;
               }

               if ( resp->useChunkedEncoding() && resp->compressionEnabled() && resp->compressionActive() )
                  return self->write();
               else if ( resp->useChunkedEncoding() && !resp->compressionEnabled() && resp->totalTransferred() < resp->expectedTotalTransferred() )
                  return self->write();
               else if (resp->dataSource()->readLeft > 0)
                  return self->write();

               if (resp->totalTransferred() != resp->expectedTotalTransferred() && !resp->compressionEnabled())
               {
                  self->_logger.error("[{}] [conn:{}] Response total transferred {} does not match expected {}", 
                                      self->logHttpType(), self->id(), resp->totalTransferred(), resp->expectedTotalTransferred() );
                  
                  self->processCompleted(self->id(), "Response total transferred does not match expected");
                  return;
               }

               self->markRequestCompletedAndLog();
               return self->handleKeepAliveOrClose();
            })
      );
   }

   /// @brief Returns a shared_ptr<ServerConnection> casted from Connection's shared_from_this()
   /// This is safe because ServerConnection is always allocated via make_shared in the connection manager.
   /// Use this in lambda captures to get the properly typed shared_ptr for async callbacks:
   ///    [this, self = this->selfPtr()] { self->someMethod(); }
   std::shared_ptr<ServerConnection> selfPtr() 
   {
      return std::static_pointer_cast<ServerConnection>(this->shared_from_this());
   }

protected:

   void startHttp1()
   {
      read();
   }

   /**
    * Read incoming body data from client.
    * Used when _parser is not using MultipartParser.
    */
   void readBody()
   {
      if (_httpContext->getBodyReader()->done())
         return;

      this->startTimer(this->timeoutRead());
      this->_socket.async_read_some(
         this->getReadBuffer(),
         asio::bind_executor(
            this->executor(),
            [self= this->selfPtr()](std::error_code error, size_t bytesTransferred)
            {
               if (error)
               {
                  if (!self->closed())
                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");

                  return;
               }

               // connection might be already closed (because of timed out or any other reasons)
               // when this handler run. So we stop here.
               if ( self->closed() )
               {
                  self->_logger.debug("[{}] [conn:{}] Attempting to read data while already closed", self->logHttpType(), self->id());
                  return;
               }

               try
               {
                  self->_totalBytesTransferred = self->_totalBytesTransferred + static_cast<int64_t>(bytesTransferred);
                  size_t totalData = bytesTransferred <= self->_readBuffer.size() ? bytesTransferred : self->_readBuffer.size();

                  tbs::span<const char> dataSpan( self->_readBuffer.data(), totalData );
                  
                  parser::Info info;
                  if ( self->_parser.hasChunkedEncoding() )
                     info = self->_parser.parse(bytesTransferred);
                  else 
                     info = self->_httpContext->getBodyReader()->feed( dataSpan , dataSpan.size() );

                  if (!info.success())
                  {
                     self->handleRequestError(std::move(info));
                     return;
                  }

                  if (info.success() &&  info.message() == "multipart-done" && self->_httpContext->getBodyReader()->done() )
                     self->_processingStopWatch.lap(self->_parser.parsingId(), "multipart" );

                  if (info.success() && ! self->_httpContext->getBodyReader()->done())
                     self->readBody();
               }
               catch(const std::exception& ex)
               {
                  if (!self->closed())
                     self->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ServerConnection");
               }
         })
      );
   }

   virtual void read()
   {
      if ( ! this->_parser.isReading() )
      {
         this->_parser.isReading(true);

         this->startTimer(this->timeoutRead());
         this->_socket.async_read_some(
            this->getReadBuffer(),
            asio::bind_executor(
               this->executor(),
               [self=this->selfPtr()](std::error_code error, size_t bytesTransferred)
               {
                  self->_parser.isReading(false);

                  if (error)
                  {
                     if ( !self->closed() )
                        self->processError(self->id(), error, ErrorType::system, "ServerConnection");

                     return;
                  }

                  // connection might be already closed (because of timed out or any other reasons)
                  // when this handler run. So we stop here.
                  if ( self->closed() )
                  {
                     self->_logger.debug("[{}] [conn:{}] Attempting to read data while already closed", self->logHttpType(), self->id());
                     return;
                  }

                  try
                  {
                     self->_totalBytesTransferred = self->_totalBytesTransferred + static_cast<int64_t>(bytesTransferred);
                     
                     if (self->_settings.logVerbose())
                        self->_logger.trace("[{}] [conn:{}] Received {} bytes", self->logHttpType(), self->id(), self->_totalBytesTransferred);

                     if (self->_parser.totalParsedBytes() == 0)
                     {
                        // TODO_JEFRI: FIX this: restart stopwatch in very beginning of receiving data
                        //self->_processingStopWatch.start();
                     }

                     auto info = self->_parser.parse(bytesTransferred);
                     if ( !info.success() )
                     {
                        // process error we got from parser, e.g: unsupported http method, 
                        // request too large, invalid header
                        self->handleRequestError(std::move(info));
                        return;
                     }

                     // We need more data
                     if ( self->_settings.enableMultipartParsing() 
                          && info.lastIndex() == self->_readBuffer.size()-1 
                          && (!self->_parser.contentDone() || !self->_parser.headersDone()) ) 
                     {
                        return self->read();
                     }

                     if ( info.httpStatus().code() == StatusCode::CONTINUE && !self->_parser.contentDone() )
                     {
                        self->handleExpect100Continue(); // send 100-continue response
                        return self->read();
                     }

                     if ( self->_parser.headersDone() || self->_parser.contentDone() )
                     {
                        if ( self->_httpContext == nullptr ||  // new connection
                             (self->_httpContext != nullptr && self->_httpContext->request()->id() != self->_parser.parsingId()) // new request in keep-alive connection
                           ) 
                        {
                           self->retrieveRequest(); // initialize _httpContext
                        }
                     }

                     // Redirect multipart parsing into a request handler (middleware)
                     if (    !self->_settings.enableMultipartParsing() 
                          && !self->_parser.contentDone() 
                          && self->_parser.hasMultipart()
                          /* && self->_parser.hasContentLength() && self->_parser.contentLength()*/ )
                     {
                        // create MultipartBodyReader with last _parser's state, to be used on MultipartBodyReader's read() 
                        // Note: Body reader use readBody() to retrieve data
                        std::shared_ptr<MultipartBodyReader> bodyReader = 
                           self->makeBodyReader(info.lastIndex(), bytesTransferred - info.bytesRead() );

                        // Multipart with Chunked Transfer Encoding
                        if (self->_parser.hasChunkedEncoding() && !self->_parser.hasContentLength())
                        {
                           self->_parser.parseChunkedMultipartHandler = bodyReader->chunkedHandler();
                           bodyReader->setProcessBodyStarter( self->_parser.processBodyStarter );
                           bodyReader->setMultipartWithChunkedTransferEncoding();
                        }

                        self->_httpContext->setBodyReader(std::move(bodyReader));

                        // The request handler must parse multipart body.
                        // handleRequest() will execute request handler until completed then call write() 
                        // which internally starting write timer, which will cancel read or process timer
                        return self->handleRequest();
                     }

                     if ( self->_parser.contentDone() )
                     {
                     
                        self->_logger.trace("[{}] [conn:{}] Received total {} bytes", self->logHttpType(), self->id(), self->_totalBytesTransferred);
                        
                        // get content from parser
                        self->_httpContext->request()->content( std::move( self->_parser.content() ) );

                        auto upgradeHeader = self->_httpContext->request()->headers().value("Upgrade");
                        if ( !upgradeHeader.empty() )
                           return self->handleUpgradeRequest(upgradeHeader);

                        // We got all headers and body. Cancel read timer by starting process timer. 
                        // Connection will close after process timer expired
                        // Note: request handler activity stil running in background
                        if (self->timeoutProcessing().value == 0)
                           self->cancelTimer();
                        else
                        {
                           // wa re going to handle the request, make sure request handler
                           // do the work within time out processing
                           self->startTimer(self->timeoutProcessing());
                        }
                        
                        // We are parsing multipart here, so get the result from parser
                        if ( self->_settings.enableMultipartParsing() && self->_parser.hasMultipart())
                           self->_httpContext->request()->multipartBody( std::move( self->_parser.multipartBody() ) );

                        // handleRequest() will execute request handler until completed then call write() 
                        // which internally starting write timer, which will cancel read or process timer
                        return self->handleRequest();
                     }
                     
                     // Read more data
                     {
                        return self->read();
                     }
                  }
                  catch(const std::exception& ex)
                  {
                     if (!self->closed())
                        self->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ServerConnection");
                  }
            })
         );
      }
      else
      {
         this->_logger.error("[{}] [conn:{}] Read operation already running, closing connection", this->logHttpType(), this->id());
         this->processError(this->id(), "Read operation but parser is not reading", ErrorType::internal, ERROR_CODE_PARSER_FAIL ,"ServerConnection");
      }
   }
   
   /// Read data from parser and prepare request and response object
   void retrieveRequest()
   {
      this->_logger.trace("[{}] [conn:{}] Request parsed successfully, preparing request and response object", this->logHttpType(), this->id());
      auto requestId = this->_parser.parsingId();

      _httpContext = std::make_shared<Context>(
           std::make_shared<Request>(HttpVersion::one)
         , std::make_shared<Response>(HttpVersion::one)
         , this->_remoteEndpoint
         , this->id()
         , requestId
         , HttpVersion::one
#ifdef TOBASA_HTTP_USE_HTTP2
         , -1
#endif
      );
      
      // use parsing id as request id
      _httpContext->request()->id( requestId );
      _httpContext->request()->setHttps( this->isTls() );

      if (! this->_parser.headersDone() ) {
         throw http::Exception("Incomplete http request");
      }

      // HEAD method is similar to GET, but we should not return the body of the response
      if (this->_parser.method() == "HEAD")
      {
         _httpContext->request()->method("GET");
         _httpContext->request()->isHeadRequest(true);
         _httpContext->response()->isHeadRequest(true);
      }
      else {
         _httpContext->request()->method( this->_parser.method() );
      }

      // Extract parsers data
      _httpContext->request()->target(       this->_parser.requestTarget() );
      _httpContext->request()->line(         this->_parser.requestLine() );
      _httpContext->request()->authority(    this->_parser.findHeader("Host").value());
      _httpContext->request()->majorVersion( this->_parser.majorVersion() );
      _httpContext->request()->minorVersion( this->_parser.minorVersion() );
      _httpContext->request()->headers(      std::move( this->_parser.headers() ) );

      _httpContext->request()->setMultipart( this->_parser.hasMultipart() );

      auto connHeader = _httpContext->request()->headers().value("Connection");
      // check wether we need to keep the connection alive
      // HTTP 1.1 default to keep-alive connection, unless request headers explicitly says close
      if ( connHeader == "close" ) {
         _httpContext->keepAlive(false);
      }
      else
      {
         if ( util::startsWith(connHeader, "keep-alive") )
            _httpContext->keepAlive(true);
         else if ( _httpContext->request()->majorVersion() == 1 && _httpContext->request()->minorVersion() >= 1)
            _httpContext->keepAlive(true);
      }
   }

   /**
    * Handle HTTP request by calling request handler
    * If request handler return notHandled, we will send 404 Not Found response
    */
   void handleRequest()
   {
      this->_logger.info("[{}] [conn:{}] Request[{}] from: {} to:{} {} {}", 
         this->logHttpType(), this->id(),
         _httpContext->request()->id(),   
         toString(_httpContext->remoteEndpoint()),
         _httpContext->request()->headers().value("Host"),
         _httpContext->request()->line(),
         _httpContext->request()->contentLength() );

      auto processStatus = [self = this->selfPtr()](RequestStatus status)
      {
         if (status == RequestStatus::notHandled)
         {
            self->buildErrorResponse(StatusCode::NOT_FOUND);
            self->write();
         }
         else if (status == RequestStatus::handled) 
         {
            if (self->_sseState)
               self->startSseResponse();
            else
               self->write();
         }
         else if (status == RequestStatus::async)
         {
            // Do nothing here.
            // The middleware will call nextHandler(ctx) later,
            // and that path must eventually call write().
         }
      };

      // setup http context on complete handler (for async request handler)
      _httpContext->onCompleteHandler(processStatus);

      // set SSE initialization handler
      _httpContext->sseInitHandler(
         [self = this->selfPtr()](SseContextPtr ctx)
         {
            if (ctx)
            {
               // Start a long-lived HTTP/1 server-sent events response.
               self->createSseConnection(ctx);
            }
         }
      );

      // Call request handler to process the request.
      // Request handler should prepare response object
      RequestStatus status = _requestHandler(_httpContext);
      processStatus(status);
   }



   /**
    * Handle error from Parser, send error response and close the connection
    */
   void handleRequestError(parser::Info info)
   {
      this->_logger.error("[{}] [conn:{}] Request error: {}", this->logHttpType(), this->id(), info.message());
      auto requestId = this->_parser.parsingId();

      _httpContext = std::make_shared<Context>(
              std::make_shared<Request>(HttpVersion::one)
            , std::make_shared<Response>(HttpVersion::one)
            , this->_remoteEndpoint
            , this->id()
            , requestId
            , HttpVersion::one
#ifdef TOBASA_HTTP_USE_HTTP2
            , -1
#endif
            );

      _httpContext->request()->id( requestId );
      _httpContext->request()->setHttps( this->isTls() );
      // set http context to close connection after successfull write
      _httpContext->keepAlive(false);

      HttpStatus status;
      if (info.httpStatus().code() == StatusCode::UNKNOWN)
         status.code(StatusCode::BAD_REQUEST);
      else
      {
         status = info.httpStatus();
         if (status.code() == StatusCode::METHOD_NOT_ALLOWED)
         {
            _httpContext->response()->addHeader("Allow", knownHttpMethodsCsv() );
         }
      }

      buildErrorResponse(status, "Error in http request");

      // tell client we are closing connection
      _httpContext->response()->setHeader("Connection", "close");

      write();
   }

   /**
    * Handler for Parser's onValidateRequestMethod event.
    * If method is not supported return false and set relevant http status code.
    * We support common http methods:
    *   GET, POST, PUT, DELETE, HEAD, OPTIONS
    * \param method     std::string
    * \param httpStatus tbs::http::HttpStatus
    * \return boolean
    */
   bool validateRequestMethod(const std::string& method, HttpStatus& httpStatus)
   {
      if (knownHttpMethods().count(method) > 0)
         return true;
      else
      {
         httpStatus = HttpStatus { StatusCode::METHOD_NOT_ALLOWED };
         return false;
      }
   }

   /**
    * Handler for Parses's onValidateHeaders. HTTP/1.1 parser
    * \param parser     tbs::http::parser::Parser
    * \param httpStatus tbs::http::HttpStatus
    * \return boolean
    */
   bool validateHeaders(parser::Parser& parser, HttpStatus& httpStatus)
   {
      // validate request-target. we only support origin form
      // origin-form    = absolute-path [ "?" query ]
      // https://datatracker.ietf.org/doc/html/rfc7230#section-5.3.1

      // TODO_JEFRI: Validate host and request-target
      auto header = parser.findHeader("Host");
      if (header.valid())
      {
         auto host = header.value();
         auto ch = parser.requestTarget().front();
         if (ch != '/')
         {
            httpStatus = HttpStatus{StatusCode::BAD_REQUEST};
            return false;
         }
      }
      else
      {
         if (parser.majorVersion()==1 && parser.minorVersion()>0 )
         {
            httpStatus = HttpStatus {StatusCode::BAD_REQUEST, "Missing or invalid Host header" };
            return false;
         }
      }

      return true;
   }

   void handleExpect100Continue() 
   {
      this->_logger.trace("[{}] [conn:{}] Sending 100 Continue to {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
      static const std::string response = "HTTP/1.1 100 Continue\r\n\r\n";
      asio::async_write(
         this->_socket,
         asio::buffer(response),
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr()](std::error_code error, std::size_t) 
            {
               if (!error)
                  self->_parser.markContinueSent();

               if (error && !self->closed())
                  self->processError(self->id(), error, ErrorType::system, "ServerConnection");
            })
      );
   }

   void buildErrorResponse(HttpStatus statusCode=StatusCode::BAD_REQUEST, const std::string& longErrorMessage="")
   {
      auto response = _httpContext->response();
      response->httpStatus(statusCode);
      
      std::string content;
      if (_statusPageBuilder)
         content = _statusPageBuilder( statusPageData(response->httpStatus(), longErrorMessage) );
      else
         content = statusPageHtml(response->httpStatus(), longErrorMessage);

      response->content(std::move(content));
      response->setHeaderContentType("text/html");
   }

   void markRequestCompletedAndLog()
   {
      this->_processingStopWatch.lap(this->_parser.parsingId(), "stop");
      this->_processingStopWatch.stop();

      this->_logger.debug("[{}] [conn:{}]] Request[{}] finished in {} |{}|{}|{}|{}|{}", 
         this->logHttpType(), this->id(),
         this->_httpContext->request()->id(),
         this->_processingStopWatch.toString(this->_parser.parsingId()),
         toString(_httpContext->remoteEndpoint()),
         this->_httpContext->response()->statusCode(),
         this->_httpContext->response()->contentType(),
         this->_httpContext->response()->contentLength(),
         this->_processingStopWatch.report(this->_parser.parsingId(), true));
   }

   void handleKeepAliveOrClose() 
   {
      bool keepAlive = this->_httpContext->keepAlive();

      // if maxRequestsPerConnection  is 0, do not check
      if ( (_currentRequestId >= this->_settings.maxRequestsPerConnection())  && (this->_settings.maxRequestsPerConnection() != 0) )
      {
         this->_logger.info("[{}] [conn:{}] Reached maximum requests per connection, closing connection", this->logHttpType(), this->id());
         keepAlive = false;
      }

      if ( keepAlive )
      {
         this->_logger.trace("[{}] [conn:{}] Keep-alive connection, waiting for new request from client", this->logHttpType(), this->id());
         
         // we are in keep-alive connection, start reading for new messages
         this->_parser.prepareForNextMessage();
         auto curentRequestId = ++_currentRequestId;   // get new request id  
         this->_parser.parsingId( curentRequestId);    // set new request id  as parsing id
         this->_processingStopWatch.lap(curentRequestId, "start" );

         read();
      }
      else
      {
         // data written succcessfully, we can close the socket now.
         // Initiate graceful connection closure.
         // call onComplete handler
         this->processCompleted(this->id(), "Request completed");

         return;
      }
   }

   std::shared_ptr<MultipartBodyReader> makeBodyReader(size_t currentIndex, size_t totalData)
   {
      std::function<void()> readCb = 
         [ weakSelf = std::weak_ptr<ServerConnection>( std::static_pointer_cast<ServerConnection>(this->shared_from_this()) )]() {
            if (auto self = weakSelf.lock())
               self->readBody();
         };

      size_t dataStart = currentIndex > 0 ? currentIndex+1 : 0;

      // _buffer, _dataStart and _totalData used only 
      // for Multipart without Chunked Transfer Encoding in first read
      auto reader = std::make_shared<MultipartBodyReader>(
         std::move(readCb)
         , span<const char>(this->_readBuffer.data() + dataStart, totalData) // buffer
         , dataStart                                                         // dataStart
         , totalData
      );

      return std::move(reader);
   }


   // -------------------------------------------------------
   // Server-Sent Events (SSE)
   // -------------------------------------------------------

   void createSseConnection(SseContextPtr sseContext)
   {
      if (_httpContext->httpVersion() != HttpVersion::one || _sseState)
         return;

      _sseState = std::make_unique<sse::SseState>();
      _sseState->context = sseContext;

      auto response = _httpContext->response();
      response->httpStatus(StatusCode::OK);
      response->setHeaderContentType("text/event-stream");
      response->setHeader("Cache-Control", "no-cache");
      response->setHeader("Connection", "keep-alive");
      response->setHeader("X-Accel-Buffering", "no");
      response->useChunkedEncoding(true);
      response->streaming(true);
      response->prepareForCompression({false, 0, {}, {}, {}});

      auto weakSelf = std::weak_ptr<ServerConnection>(this->selfPtr());

      _sseState->ssePtr = std::make_shared<SseConnection>(
         // Send Handler
         [weakSelf](std::string data)
         {
            if (auto self = weakSelf.lock())
            {
               asio::post(self->executor(),
                  [self, data = std::move(data)]() mutable
                  {
                     if (!self->closed() && self->_sseState)
                     {
                        self->_sseState->sendQueue.emplace_back(std::move(data));
                        self->writeNextSseChunk();
                     }
                  });
            }
         },
         // Close Handler
         [weakSelf]()
         {
            if (auto self = weakSelf.lock())
            {
               if (!self->_sseState)
                  return;

               self->removeSseConnection();

               asio::post(self->executor(),
                  [self]
                  {
                     if (!self->closed() && self->_sseState)
                     {
                        self->_sseState->closeRequested = true;
                        self->writeNextSseChunk();
                     }
                  });
            }
         },
         this->shared_from_this(),
         _httpContext->remoteEndpoint(),
         _httpContext->userData());

      if (sseContext)
         sseContext->add(_sseState->ssePtr);
   }

   void removeSseConnection()
   {
      if (!_sseState || !_sseState->ssePtr)
         return;

      auto connection = _sseState->ssePtr;
      if (auto context = _sseState->context.lock())
         context->remove(connection);

      _sseState->ssePtr.reset();
   }

   void startSseResponse()
   {
      if (!_sseState || _sseState->headersWritten || _sseState->writing || this->closed())
         return;

      this->cancelTimer();

      try
      {
         // force content-type to text/event-stream
         _httpContext->response()->setHeaderContentType("text/event-stream");

         ResponseSerializer serializer(_httpContext->response());
         serializer.serializeHttp1(&this->_sendBuffer, this->_settings.sendBufferSize());
         this->startTimer(this->timeoutWrite());
         asio::async_write(
            this->_socket,
            this->_sendBuffer,
            asio::bind_executor(
               this->executor(),
               [self = this->selfPtr()](std::error_code error, std::size_t bytesTransferred)
               {
                  self->cancelTimer();
                  if (!self->_sseState)
                     return;

                  if (error)
                  {
                     self->removeSseConnection();
                     if (!self->closed())
                        self->processError(self->id(), error, ErrorType::system, "ServerConnection");

                     self->_sseState.reset();
                     return;
                  }

                  if (self->_settings.logVerbose())
                     self->_logger.trace("[{}] [conn:{}] startSseResponse. {} bytes SSE data sent to {}", self->logHttpType(), self->id(), bytesTransferred, toString(self->_remoteEndpoint));

                  self->_httpContext->response()->updateTotalTransferred(bytesTransferred);
                  self->_sendBuffer.consume(bytesTransferred);
                  self->_sseState->headersWritten = true;
                  self->writeNextSseChunk();
               })
         );
      }
      catch (const std::exception& ex)
      {
         if (_sseState)
         {
            removeSseConnection();
            _sseState.reset();
         }

         if (!this->closed())
            this->processError(this->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ServerConnection");
      }
   }

   void writeNextSseChunk()
   {
      if (!_sseState || !_sseState->headersWritten || _sseState->writing || this->closed())
         return;

      std::shared_ptr<std::string> output;
      bool finalChunk = false;
      if (!_sseState->sendQueue.empty())
      {
         auto payload = std::move(_sseState->sendQueue.front());
         _sseState->sendQueue.pop_front();

         std::ostringstream chunk;
         chunk << std::hex << payload.size() << "\r\n" << payload << "\r\n";
         output = std::make_shared<std::string>(std::move(chunk).str());
      }
      else if (_sseState->closeRequested)
      {
         output = std::make_shared<std::string>("0\r\n\r\n");
         finalChunk = true;
      }
      else
         return;

      _sseState->writing = true;
      this->startTimer(this->timeoutWrite());
      asio::async_write(
         this->_socket,
         asio::buffer(*output),
         asio::bind_executor(
            this->executor(),
            [self = this->selfPtr(), output, finalChunk](std::error_code error, std::size_t bytesTransferred)
            {
               self->cancelTimer();
               if (!self->_sseState)
                  return;

               self->_sseState->writing = false;

               if (error)
               {
                  self->removeSseConnection();
                  if (!self->closed())
                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");

                  self->_sseState.reset();
                  return;
               }

               if (self->_settings.logVerbose())
                  self->_logger.trace("[{}] [conn:{}] writeNextSseChunk. {} bytes SSE data sent to {}", self->logHttpType(), self->id(), bytesTransferred, toString(self->_remoteEndpoint));

               self->_httpContext->response()->updateTotalTransferred(bytesTransferred);
               if (finalChunk)
               {
                  self->removeSseConnection();
                  self->processCompleted(self->id(), "SSE stream completed");
                  self->_sseState.reset();
                  return;
               }

               self->writeNextSseChunk();
            })
      );
   }


   // -------------------------------------------------------
   // Web Socket
   // -------------------------------------------------------

   void createWebSocketConnection(WebSocketContextPtr context)
   {
      if (_wsState)
         return;

      _wsState = std::make_unique<ws::WebSocketState>();
      _wsState->wsPtr = std::make_shared<WebSocket>(
                           this->shared_from_this(), 
                           this->id(),
                           _httpContext->remoteEndpoint(),
                           _httpContext->request()->headers(),
                           _httpContext->userData() );
      
      _wsState->wsContext = context;

      auto self = this->selfPtr();
      ws::WebSocketTransport transport;

      transport.closed           = [self] { return self->closed(); };
      transport.messageMaxSize   = this->_settings.wsMessageMaxSize();
      transport.startReadTimer   = [self] { self->startTimer(self->timeoutRead()); };
      transport.startWriteTimer  = [self] { self->startTimer(self->timeoutWrite()); };
      transport.cancelTimer      = [self] { self->cancelTimer(); };
      
      transport.read = [self](std::size_t bytes, ws::SocketReadHandler callback)
      {
         asio::async_read(
            self->_socket,
            self->_wsState->sendStreamBuf,
            asio::transfer_exactly(bytes),
            asio::bind_executor(
               self->executor(),
               [self, callback = std::move(callback)](const std::error_code& error, std::size_t transferred)
               {
                  if (self->_settings.logVerbose())
                     self->_logger.trace("[{}] [conn:{}] {} bytes of websocket data read from {}", self->logHttpType(), self->id(), transferred, toString(self->_remoteEndpoint));

                  if (self->_wsState)
                     callback(error, transferred);
               }));
      };

      transport.write = [self](const ws::OutData& outData, ws::SocketWriteHandler callback)
      {
         std::array<asio::const_buffer, 2> buffers {outData.outHeader->streambuf.data(), outData.outMessage->streambuf.data() };

         asio::async_write(
            self->_socket,
            buffers,
            asio::bind_executor(
               self->executor(),
               [self, callback = std::move(callback)](const std::error_code& error, std::size_t transferred)
               {
                  if (self->_settings.logVerbose())
                     self->_logger.trace("[{}] [conn:{}] {} bytes websocket data sent to {}", self->logHttpType(), self->id(), transferred, toString(self->_remoteEndpoint));

                  callback(error, transferred);
               }));
      };

      transport.error = [self](const std::error_code& error, ErrorType type)
      {
         if (!self->closed())
            self->processError(self->id(), error, type, "ServerConnection");
      };

      transport.complete = [self](const std::string& reason)
      {
         self->processCompleted(self->id(), reason);
      };

      _wsState->configureTransport(std::move(transport));

      _wsState->wsPtr->setTransport(
         // Send text handler
         [self](const std::string& data, ws::SendErrorHandler callback)
         {
            if (self->_wsState)
               self->_wsState->sendText(data, std::move(callback));
         },
         // Send binary handler
         [self](const std::string& data, ws::SendErrorHandler callback)
         {
            if (self->_wsState)
               self->_wsState->sendBinary(data, std::move(callback));
         },
         // Send close handler
         [self](int32_t status, const std::string& reason, ws::SendErrorHandler callback)
         {
            if (self->_wsState)
               self->_wsState->sendClose(status, reason, std::move(callback));

            self->callClose(reason);
         });
   }

   /**
    * Handle WebSocket Upgrade request.
    * If request handler accept the upgrade request, it must set webSocketContext in httpContext
    */
   void handleUpgradeRequest(const std::string& upgradeType)
   {
      this->_logger.info("[{}] [conn:{}] Processing upgrade request {} {} {}", this->logHttpType(),
                  this->id(),
                  toString(_httpContext->remoteEndpoint()),
                  _httpContext->request()->line(),
                  _httpContext->request()->contentLength() );

      if (upgradeType != "websocket")
      {
         this->_logger.error("[{}] [conn:{}] Non WebSocket upgrade request", this->logHttpType(), this->id());
         buildErrorResponse(StatusCode::BAD_REQUEST);
         write();
         return;
      }

      // set web socket initialization handler
      _httpContext->webSocketInitHandler(
         [self = this->selfPtr()](WebSocketContextPtr ctx)
         {
            if (ctx)
            {
              self->createWebSocketConnection(ctx);
            }
         }
      );

      
      auto processStatus = [self = this->selfPtr()](RequestStatus status)
      {
         switch (status)
         {
            case RequestStatus::notHandled:
            {
               if (self->_socket.lowest_layer().is_open())
               {
                  // Request is rejected, so our socket
                  // must not be moved out to websocket connection.

                  // If handler refused request, say not implemented
                  self->buildErrorResponse(StatusCode::NOT_IMPLEMENTED, "WebSocket endpoint not implemented on this server");
                  self->write();
                  return;
               }
               else
               {
                  // Request is rejected, but the socket
                  // was moved out to somewhere else???
                  self->_logger.info("[{}] [conn:{}] upgrade request handler rejects request,"
                                    " but socket was moved out from connection", self->logHttpType(), self->id() );
                  return;
               }
            }
            break;

            case RequestStatus::handled:
            {
               auto response = self->_httpContext->response();
               // Get http status code set in http response processed by http request handler
               auto httpStatus = response->httpStatus();

               if ( httpStatus.code() == http::StatusCode::OK)
               {
                  // to further process websocket upgrade request,
                  // request handler must already set webSocketContext in http context
                  bool wsContextInitialized = (self->_wsState && self->_wsState->wsContext);
                  if (!wsContextInitialized)
                  {
                     self->_logger.error("[{}] [conn:{}] Websocket context not initialized", self->logHttpType(), self->id());
                     self->buildErrorResponse(StatusCode::INTERNAL_SERVER_ERROR);
                     self->write();
                     return;
                  }

                  // http request handler said OK to upgrade this http request to websocket
                  // now we can process this Upgrade request
                  auto secWebSocketKey      = self->_httpContext->request()->headers().value("Sec-WebSocket-Key");
                  auto secWebSocketProtocol = self->_httpContext->request()->headers().value("Sec-WebSocket-Protocol");

                  if ( ! secWebSocketKey.empty() )
                  {
                     // Sec-WebSocket-Key  present in http headers, send WebSocket handshake response
                     response->headers().add("Upgrade", "websocket");
                     response->headers().add("Connection", "Upgrade");

                     static auto wsMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                     auto sha1Bytes  = crypt::hashSHA1Bytes(secWebSocketKey + wsMagicString);
                     auto sockAccept = base64::encode(sha1Bytes);

                     response->headers().add("Sec-WebSocket-Accept", sockAccept);

                     if (!secWebSocketProtocol.empty())
                     {
                        // Note: https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/WebSocket#protocols
                        // https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Sec-WebSocket-Protocol

                        // if Sec-WebSocket-Protocol header present, we must echo it back
                        // Note: The server must respond with one of the exact strings that the client sent as a subprotocol

                        // Note: we misused Sec-WebSocket-Protocol header by sending JWT Bearer token
                        //       as a subprotocol, so we can use it later to authenticate the client
                        //      this is not a standard way to use Sec-WebSocket-Protocol header

                        // in web brower we send JWT Bearer token as a subprotocol
                        // like this: var ws = new WebSocket("wss://example.com/ws", ["Bearer", "dfadfdsfasf"] );
                        // so browser send it as two separate protocols, 
                        // Which results in this http request header:
                        // Sec-WebSocket-Protocol: Bearer, dfadfdsfasf
                        // so it is safe to echo back only Bearer
                        auto protos = util::split(secWebSocketProtocol, ",");
                        if (protos.size() > 0)
                        {
                           // We only support Bearer token as a subprotocol
                           auto first = protos[0];
                           if ( first == "Bearer")
                              response->headers().add("Sec-WebSocket-Protocol", "Bearer");
                        }
                     }

                     response->httpStatus(http::StatusCode::SWITCHING_PROTOCOLS);

                     self->writeWebSocketUpgradeResponse();

                     return;
                  }
                  else
                  {
                     // send proper status code to client
                     response->httpStatus(http::StatusCode::UPGRADE_REQUIRED);
                     self->write();
                     return;
                  }
               }
               else
               {
                  if (httpStatus.code() == StatusCode::UNAUTHORIZED)
                     self->buildErrorResponse(StatusCode::UNAUTHORIZED, "Unauthorized");
                  else
                     self->buildErrorResponse(StatusCode::UNAUTHORIZED, "WebSocket endpoint not implemented on this server");

                  self->write();
                  return;
               }
            }
            break;
            
            case RequestStatus::async:
               auto x=1;
            break;
         }
      };

      _httpContext->onCompleteHandler( processStatus );
      RequestStatus status = _requestHandler(_httpContext);
      processStatus(status);
   }

   /// Send WebSocket Upgrade Response and start websocket communication
   void writeWebSocketUpgradeResponse()
   {
      this->_logger.debug("[{}] [conn:{}] Sending Websocket upgrade response to {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));

      ResponseSerializer serializer( _httpContext->response() );
      serializer.serializeHttp1(&this->_sendBuffer);

      this->startTimer(this->timeoutWrite());
      asio::async_write(
         this->_socket,
         this->_sendBuffer,
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr()] (std::error_code error, std::size_t)
            {
               if (!error)
               {
                  self->_processingStopWatch.stop();
                  self->_logger.info("[{}] [conn:{}] Upgrade request finished in {}, websocket established with {} ", 
                              self->logHttpType(),
                              self->id(),
                              self->_processingStopWatch.toString(self->_parser.parsingId(),true),
                              toString(self->_httpContext->remoteEndpoint()) );

                  self->_isWebSocket = true;
                  // add websocket to wsContext and call onOpen handler
                  self->_wsState->onOpen();

                  self->_wsState->readWebSocket();
               }
               else
               {
                  if (!self->closed())
                  {
                     if (self->_wsState && self->_wsState->wsContext)
                        self->_wsState->onError(error, ErrorType::system);

                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");
                  }
               }
            })
      );
   }


#ifdef TOBASA_HTTP_USE_HTTP2

   // -------------------------------------------------------
   // HTTP/2
   // -------------------------------------------------------

   void createSseConnection(HttpContext httpContext, SseContextPtr sseContext)
   {
      if (!httpContext || httpContext->httpVersion() != HttpVersion::two)
         return;

      auto response = httpContext->response();
      response->httpStatus(StatusCode::OK);
      response->setHeaderContentType("text/event-stream");
      response->setHeader("Cache-Control", "no-cache");
      response->setHeader("X-Accel-Buffering", "no");
      response->streaming(true);
      response->prepareForCompression({false, 0, {}, {}, {}});

      auto streamId   = httpContext->streamId();
      auto weakSelf   = std::weak_ptr<ServerConnection>(this->selfPtr());
      auto connection = std::make_shared<SseConnection>(
         // Send handler
         [weakSelf, streamId](std::string data)
         {
            if (auto self = weakSelf.lock())
               asio::post(self->executor(), [self, streamId, data = std::move(data)]() mutable
               {
                  if (!self->closed() && self->_http2Session)
                     self->_http2Session->enqueueSseData(streamId, std::move(data));
               });
         },
         // Close handler
         [weakSelf, streamId]
         {
            if (auto self = weakSelf.lock())
               asio::post(self->executor(), [self, streamId]
               {
                  if (!self->closed() && self->_http2Session)
                     self->_http2Session->closeSseStream(streamId);
               });
         },
         this->shared_from_this(),
         httpContext->remoteEndpoint(),
         httpContext->userData());

      _http2Session->registerSseStream(streamId, connection, sseContext);
      if (sseContext)
         sseContext->add(connection);
   }

   void createHttp2WebSocketConnection(HttpContext context, WebSocketContextPtr wsContext, int32_t streamId)
   {
      if (_http2WebSockets.count(streamId))
         return;

      auto connection   = std::make_shared<Http2WebSocketConnection>();
      connection->state = std::make_unique<ws::WebSocketState>();

      connection->state->wsPtr = std::make_shared<WebSocket>(
                           this->shared_from_this(), 
                           this->id(),
                           context->remoteEndpoint(),
                           context->request()->headers(),
                           context->userData() );
     
      connection->state->wsContext = std::move(wsContext);

      auto self = this->selfPtr();

      ws::WebSocketTransport transport;

      transport.closed         = [self] { return self->closed(); };
      transport.messageMaxSize = this->_settings.wsMessageMaxSize();

      transport.write = [self, streamId](const ws::OutData& outData, ws::SocketWriteHandler callback)
      {
         if (self->_settings.logVerbose())
            self->_logger.trace("[{}] [conn:{}] Sending websocket data to {}. Stream ID {}", self->logHttpType(), self->id(), toString(self->_remoteEndpoint), streamId);

         std::string frame;
         auto headerBuffers = outData.outHeader->streambuf.data();
         auto messageBuffers = outData.outMessage->streambuf.data();
         frame.append(asio::buffers_begin(headerBuffers),  asio::buffers_end(headerBuffers));
         frame.append(asio::buffers_begin(messageBuffers), asio::buffers_end(messageBuffers));
         self->_http2Session->enqueueWebSocketData(streamId, std::move(frame));
         callback({}, 0);
      };

      transport.complete = [self, streamId](const std::string&)
      {
         if (self->_http2Session)
            self->_http2Session->closeWebSocketStream(streamId);
      };

      connection->state->configureTransport(std::move(transport));

      connection->state->wsPtr->setTransport(
         // Send text handler
         [self, streamId](const std::string& data, ws::SendErrorHandler callback) {
            auto it = self->_http2WebSockets.find(streamId);
            if (it != self->_http2WebSockets.end())
               it->second->state->sendText(data, std::move(callback));
         },
         // Send binary Handler
         [self, streamId](const std::string& data, ws::SendErrorHandler callback) {
            auto it = self->_http2WebSockets.find(streamId);
            if (it != self->_http2WebSockets.end())
               it->second->state->sendBinary(data, std::move(callback));
         },
         // Send close Handler
         [self, streamId](int32_t status, const std::string& reason, ws::SendErrorHandler callback) {
            auto it = self->_http2WebSockets.find(streamId);
            if (it != self->_http2WebSockets.end())
               it->second->state->sendClose(status, reason, std::move(callback));

            if (self->_http2Session)
               self->_http2Session->closeWebSocketStream(streamId);
         });

      _http2WebSockets[streamId] = connection;

      this->_http2Session->registerWebSocketStream(
         streamId,
         [self, streamId](const uint8_t* data, size_t length)
         {
            auto it = self->_http2WebSockets.find(streamId);
            if (it != self->_http2WebSockets.end())
            {
               if (self->_settings.logVerbose())
                  self->_logger.trace("[{}] [conn:{}] Reading {} bytes websocket data from {}. Stream ID: {}", self->logHttpType(), self->id(), length, toString(self->_remoteEndpoint), streamId);
               
               it->second->state->feedHttp2WebSocket(data, length);
            }
         },
         [self, streamId]() {
            auto it = self->_http2WebSockets.find(streamId);
            if (it != self->_http2WebSockets.end())
            {
               it->second->state->onClose(WS_CLOSE_CODE_GOING_AWAY, "HTTP/2 stream closed");
               self->_http2WebSockets.erase(it);
            }
         });
   }

   /// @brief Start HTTP/2 session with nghttp2
   /// after a TLS connection is successfully established and HTTP/2 Protocol Negotiation completed
   void startHttp2()
   {
      this->_http2Option = std::make_shared<http2::Http2Option>();
      this->_http2Option->sendBufferSize = this->_settings.sendBufferSize();
      this->_http2Option->maxHeaderSize  = this->_settings.maxHeaderSize();
      this->_http2Option->logVerbose     = this->_settings.logVerboseHttp2();
      this->_http2Option->temporaryDir   = this->_settings.temporaryDir();

      this->_http2Session = std::make_shared<http2::Http2Session>(
                              this->id(), 
                              &this->_sendBuffer, 
                              this->_http2Option);

      this->_http2Session->writeHandler(
         [self=this->selfPtr()](http2::HandlerCallback cb)
         {
            asio::post(self->executor(),
               [self, cb=std::move(cb)]() 
               {
                  cb();

                  if (self->_http2Option->logVerbose)
                     self->_logger.trace("[{}] [conn:{}] writeHandler call writeHttp2", self->logHttpType(), self->id());

                  self->writeHttp2();
               });
         }
      );

      this->_http2Session->addToSendQueueHandler(
         [this](std::shared_ptr<asio::streambuf> buffer, http2::HandlerCallback cb)
         {
         }
      );

      this->_http2Session->requestReadyHandler(
         [this](int32_t streamId)
         {
            return this->handleHttp2Request(streamId);
         }
      );

      this->_http2Session->validateMethodHandler(
         std::bind(&ServerConnection::validateRequestMethod, this, std::placeholders::_1, std::placeholders::_2) );

      //this->_http2Session->validateHeadersHandler(
      //   std::bind(&ServerConnection::validateHeaders, this, std::placeholders::_1, std::placeholders::_2) );

      // init nghttp2, send server connection header to remote endpoint,
      auto result = this->_http2Session->start();
      if (result.success())
      {
         this->_logger.debug("[{}] [conn:{}] HTTP/2 session started succesfully", this->logHttpType(), this->id());
         readHttp2();
      }
      else
         this->processError(this->id(), result.message(), ErrorType::internal, ERROR_CODE_WRITE_FAIL, "ServerConnection");
   }

   void readHttp2()
   {
      if (!this->_http2Session->hasSseStreams())
         this->startTimer(this->timeoutRead());

      this->_socket.async_read_some(
         this->getReadBuffer(),
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr()](std::error_code error, size_t bytesTransferred)
            {
               self->cancelTimer();

               if (!error)
               {
                  // connection might be already closed(because of timed out or any other reasons)
                  // when this handler run. so we stop here.
                  if ( self->closed() )
                  {
                     self->_logger.trace("[{}] [conn:{}] [http2] read, attempting to read data while already closed", self->logHttpType(), self->id());
                     return;
                  }
                  else
                  {
                     if (self->_http2Option->logVerbose)
                     {
                        self->_logger.trace("[{}] [conn:{}] [http2] read,  Receive {} bytes", self->logHttpType(), self->id(), bytesTransferred);
                        self->_logger.trace("[{}] [conn:{}] [http2] read,  calling readIncomingData", self->logHttpType(), self->id());
                     }

                     auto result = self->_http2Session->readIncomingData(self->getReadBuffer(), bytesTransferred);
                     if (! result.success() )
                     {
                        self->_logger.error("[{}] [conn:{}] {}", self->logHttpType(), self->id(), result.message() );
                        self->processError(self->id(), result.message(), ErrorType::internal, ERROR_CODE_READ_FAIL , "ServerConnection");
                        return;
                     }

                     if (self->_http2Option->logVerbose)
                        self->_logger.trace("[{}] [conn:{}] [http2] read, call writeHttp2", self->logHttpType(), self->id());

                     self->writeHttp2();

                      if ( !self->_http2Session->writingData() &&  self->_http2Session->shouldStop()
                         && !self->_http2Session->hasSseStreams())
                     {
                        self->processCompleted(self->id(), "all HTTP/2 data processed [R]");
                        return;
                     }

                     // Once data is written, ensure to continue reading for further frames
                     if (self->_http2Option->logVerbose)
                        self->_logger.trace("[{}] [conn:{}] [http2] read,  call readHttp2", self->logHttpType(), self->id());

                     self->readHttp2();
                  }
               }
               else
               {
                  if (! self->closed())
                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");
               }
            })
      );
   }

   void writeHttp2()
   {
      if (this->_http2Session->writingData()) {
         return;
      }

      size_t bytesFilled = 0;
      auto result = this->_http2Session->fillSendBuffer(bytesFilled);
      size_t bytesToTransfer = this->_sendBuffer.size();

      if (!result.success())
      {
         std::string errMsg = result.message() + " streamId: " + std::to_string(result.streamId());
         this->processError(this->id(), errMsg, ErrorType::internal, ERROR_CODE_EXCEPTION, "ServerConnection");
         return;
      }

      if (bytesToTransfer == 0)
      {
         if (this->_http2Session->shouldStop() && !this->_http2Session->hasSseStreams())
            this->processCompleted(this->id(), "All HTTP/2 data processed [W]");

         return;
      }

      if (this->_http2Option->logVerbose)
         this->_logger.trace("[{}] [conn:{}] [http2] write, data to send {} bytes, to: {}, sendBuffer size:{}", this->logHttpType(), this->id(), bytesToTransfer, toString(this->_remoteEndpoint), this->_sendBuffer.size());
      
      this->_http2Session->writingData(true);

      this->startTimer(this->timeoutWrite());

      asio::async_write(
         this->_socket,
         this->_sendBuffer,
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr(), bytesToTransfer] (std::error_code error, std::size_t bytesTransferred)
            {
               self->cancelTimer();
               self->_http2Session->writingData(false);
               self->_processingStopWatch.stop();

               if (!error)
               {
                  if ( self->closed() )
                  {
                     self->_logger.trace("[{}] [conn:{}] [http2] write, attempting to write data while already closed", self->logHttpType(), self->id());
                     return;
                  }

                  if (self->_http2Option->logVerbose)
                  {
                     self->_logger.trace("[{}] [conn:{}] [http2] write, Send {} bytes, sendBuffer size: {}", self->logHttpType(), self->id(), bytesTransferred, self->_sendBuffer.size());
                     self->_logger.trace("[{}] [conn:{}] [http2] write, call writeHttp2", self->logHttpType(), self->id());
                  }

                  self->writeHttp2();
               }
               else
               {
                  if (! self->closed())
                     self->processError(self->id(), error, ErrorType::system, "ServerConnection");
               }
            })
      );
   }

   /// Prepare http context
   http2::Result handleHttp2Request(int32_t streamId)
   {
      /*
      HTTP/1.1
      --------------------------------------------
         GET /path/to/resource?query=123 HTTP/1.1
         Host: example.com

      HTTP/2
      --------------------------------------------
         :method: GET
         :scheme: https
         :authority: example.com:8085
         :path: /path/to/resource?query=123
      */

      auto httpContext = std::make_shared<Context>(
              std::make_shared<Request>(HttpVersion::two)
            , std::make_shared<Response>(HttpVersion::two)
            , this->_remoteEndpoint
            , this->id()
            , streamId
            , HttpVersion::two
            , streamId );

      auto streamData = this->_http2Session->findStream(streamId);
      if (!streamData)
         return http2::Result::fail("Stream not found", streamId);

      const bool isWebSocket = streamData->isWebSocketConnect();

      httpContext->request()->id( streamId );
      httpContext->request()->setHttps( this->isTls() );
      httpContext->request()->method( isWebSocket ? "GET" : streamData->requestHeader.method );
      
      // HEAD method is similar to GET, but we should not return the body of the response
      if (httpContext->request()->method() == "HEAD")
      {
         httpContext->request()->method("GET");
         httpContext->request()->isHeadRequest(true);
         httpContext->response()->isHeadRequest(true);
      }

      httpContext->request()->authority(     streamData->requestHeader.authority );
      httpContext->request()->target(        streamData->requestHeader.path );
      //httpContext->request()->path(        streamData->requestHeader.path );
      httpContext->request()->line(          streamData->requestHeader.method + " " + streamData->requestHeader.path + " HTTP/2" );

      httpContext->request()->majorVersion(  2 );
      httpContext->request()->minorVersion(  0 );
      httpContext->request()->headers(       std::move(streamData->requestHeaders));

      httpContext->request()->content(       std::move(streamData->requestBody));

      if (streamData->hasMultipartBody)
         httpContext->request()->multipartBody( std::move( streamData->multipartBody ) );

      this->_http2Session->addHttpContext(httpContext);

      return handleHttp2Request(httpContext, streamId);
   }

   http2::Result handleHttp2Request(HttpContext httpContext, int32_t streamId)
   {
      this->_logger.info("[{}] [conn:{}] HTTP/2 Request from: {} to: {} {} {}", 
                  this->logHttpType(), this->id(),
                  toString(httpContext->remoteEndpoint()),
                  httpContext->request()->authority(),
                  httpContext->request()->line(),
                  httpContext->request()->contentLength() );

      
      // Note: For HTTP/1 we setup compression inside write()

      auto streamData = _http2Session->findStream(streamId);
      const bool isWebSocket = streamData && streamData->isWebSocketConnect();

      auto processStatus = 
      [self = this->selfPtr(), ctx=httpContext, sid=streamId, isWebSocket](RequestStatus status)
      {
         if (status == RequestStatus::notHandled) {
            self->buildErrorResponse(StatusCode::NOT_FOUND);
         }
         else if (status == RequestStatus::handled) {}
         else if (status == RequestStatus::async)
         {
            // should not be called here, async middlewares keep control
            return http2::Result(0,"",sid);
         }

         if (isWebSocket)
         {
            if (status != RequestStatus::handled ||
                ctx->response()->httpStatus().code() != StatusCode::OK)
            {
               if (!ctx->response()->preparedForCompression())
               {
                  auto accept = ctx->request()->headers().value("Accept-Encoding");
                  ctx->response()->prepareForCompression(self->compressionRule(accept));
               }
               return self->_http2Session->submitResponse(ctx, sid);
            }

            auto ws = self->_http2WebSockets.find(sid);
            if (ws == self->_http2WebSockets.end() || !ws->second->state->wsContext)
               return http2::Result::fail("WebSocket context not initialized", sid);

            auto result = self->_http2Session->submitWebSocketResponse(sid);
            if (result.success())
               ws->second->state->onOpen();
            return result;
         }

         if (!ctx->response()->preparedForCompression()) 
         {
            auto accept = ctx->request()->headers().value("Accept-Encoding");
            ctx->response()->prepareForCompression( self->compressionRule(accept) );
         }

         return self->_http2Session->submitResponse(ctx, sid);
      };

      // setup http context on complete handler (for async request handler)
      httpContext->onCompleteHandler(
         [func=processStatus](RequestStatus status){
            func(status);
         }
      );

      httpContext->sseInitHandler(
         [self = this->selfPtr(), httpContext](SseContextPtr ctx)
         {
            if (ctx)
               self->createSseConnection(httpContext, ctx);
         }
      );

      if (isWebSocket)
      {
         httpContext->webSocketInitHandler(
            [self = this->selfPtr(), httpContext, streamId](WebSocketContextPtr wsContext)
            {
               if (wsContext)
                  self->createHttp2WebSocketConnection(httpContext, std::move(wsContext), streamId);
            });
      }

      // execute request handler to get response
      RequestStatus status = _requestHandler(httpContext);
      return processStatus(status);
   }

#endif // TOBASA_HTTP_USE_HTTP2
};

/** @}*/

} // namespace http
} // namespace tbs