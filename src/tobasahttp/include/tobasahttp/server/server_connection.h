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
      "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS"/*, "CONNECT", "TRACE", "PATCH" */
   };
   return knownMethods;
}; 

inline bool isKnownHttpMethod(const std::string& method) 
{
   return knownHttpMethods().count(method) > 0;
}

inline std::string knownHttpMethodsCsv() {
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
 * @brief Represents a single HTTP client connection.
 *
 * ServerConnection is responsible for:
 * - Reading and parsing the HTTP request using http::parser::Parser.
 *   This covers HTTP headers, content length, transfer encoding, and
 *   other core protocol details.
 * - Creating an HttpContext with request/response objects.
 * - Handling body input either directly (simple requests) or via a
 *   BodyReader for advanced middleware (e.g. multipart streaming).
 * - Managing WebSocket upgrade requests:
 *   - Detecting the upgrade handshake in http request
 *   - Building the appropriate WebSocket response.
 *   - Creating a WebSocketConn object that wraps this connection for
 *     further frame-based communication.
 * - Passing the HttpContext to the configured request handler
 * - Writing the final HTTP response back to the client.
 *
 * Design:
 * Core parsing ensures protocol correctness and produces a clean HttpContext.
 * Middleware parsing (like MultipartMiddleware) is layered on top, using
 * BodyReader to stream and process complex bodies before resuming the pipeline.
 * For WebSocket requests, ServerConnection transitions into WebSocketConn
 * after the upgrade, switching from HTTP message handling to frame-based I/O.
 */
template <class Traits>
class ServerConnection
   : public HttpConnection<Traits>
{
public:
   using Socket         = typename Traits::Socket;
   using Settings       = typename Traits::Settings;
   using Logger         = typename Traits::Logger;

   ServerConnection(const ServerConnection&) = delete;
   ServerConnection& operator=(const ServerConnection&) = delete;

private:
   RequestHandler&         _requestHandler;
   StatusPageBuilder       _statusPageBuilder;

   /// Http context available for request handler
   HttpContext             _httpContext            { nullptr };
   int64_t                 _totalBytesTransferred  { 0 };
   std::atomic<uint32_t>   _currentRequestId       { 0 };

#ifdef TOBASA_HTTP_USE_HTTP2
   http2::Http2OptionPtr   _http2Option            { nullptr };
   http2::Http2SessionPtr  _http2Session           { nullptr };
#endif

   ws::WebSocketConnUPtr    _wsConnection;

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
            [this, self = this->shared_from_this()] (std::error_code error, std::size_t byteTransferred)
            {
               auto resp = this->_httpContext->response();
               resp->updateTotalTransferred(byteTransferred);

               // consume the bytes we just wrote from the send buffer
               if (byteTransferred > 0)
                  this->_sendBuffer.consume(byteTransferred);
               
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
                        this->markRequestCompletedAndLog();
                        this->processCompleted(self->id(), error.message());
                        return;
                     }
                  } 

                  if (!this->closed())
                     this->processError(self->id(), error, ErrorType::system, "ServerConnection");

                  return;
               }

               if ( resp->useChunkedEncoding() && resp->compressionEnabled() && resp->compressionActive() )
                  return write();
               else if ( resp->useChunkedEncoding() && !resp->compressionEnabled() && resp->totalTransferred() < resp->expectedTotalTransferred() )
                  return write();
               else if (resp->dataSource()->readLeft > 0)
                  return write();

               if (resp->totalTransferred() != resp->expectedTotalTransferred() && !resp->compressionEnabled())
               {
                  this->_logger.error("[{}] [conn:{}] Response total transferred {} does not match expected {}", 
                     this->logHttpType(), self->id(), resp->totalTransferred(), resp->expectedTotalTransferred() );
                  
                  this->processCompleted(self->id(), "Response total transferred does not match expected");
                  return;
               }

               this->markRequestCompletedAndLog();
               return handleKeepAliveOrClose();
            }
         )
      );
   }

   /// @brief Returns a shared_ptr<ServerConnection> casted from Connection's shared_from_this()
   /// This is safe because ServerConnection is always allocated via make_shared in the connection manager.
   /// Use this in lambda captures to get the properly typed shared_ptr for async callbacks:
   ///    [this, self = this->selfPtr()] { self->someMethod(); }
   std::shared_ptr<ServerConnection> selfPtr() {
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
            [this, self =  this->shared_from_this()](std::error_code error, size_t bytesTransferred)
            {
               if (error)
               {
                  if (!this->closed())
                     this->processError(self->id(), error, ErrorType::system, "ServerConnection");

                  return;
               }

               // connection might be already closed (because of timed out or any other reasons)
               // when this handler run. So we stop here.
               if ( this->closed() )
               {
                  this->_logger.debug("[{}] [conn:{}] Attempting to read data while already closed", this->logHttpType(), this->id());
                  return;
               }

               try
               {
                  _totalBytesTransferred = _totalBytesTransferred + static_cast<int64_t>(bytesTransferred);
                  size_t totalData = bytesTransferred <= this->_readBuffer.size() ? bytesTransferred : this->_readBuffer.size();

                  tbs::span<const char> dataSpan( this->_readBuffer.data(), totalData );
                  
                  parser::Info info;
                  if ( this->_parser.hasChunkedEncoding() )
                     info = this->_parser.parse(bytesTransferred);
                  else 
                     info = _httpContext->getBodyReader()->feed( dataSpan , dataSpan.size() );

                  if (!info.success())
                  {
                     handleRequestError(std::move(info));
                     return;
                  }

                  if (info.success() &&  info.message() == "multipart-done" && _httpContext->getBodyReader()->done() )
                     this->_processingStopWatch.lap(this->_parser.parsingId(), "multipart" );

                  if (info.success() && ! _httpContext->getBodyReader()->done())
                     readBody();
               }
               catch(const std::exception& ex)
               {
                  if (!this->closed())
                     this->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ServerConnection");
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
               [this, self =  this->shared_from_this()](std::error_code error, size_t bytesTransferred)
               {
                  this->_parser.isReading(false);

                  if (error)
                  {
                     if ( !this->closed() )
                        this->processError(self->id(), error, ErrorType::system, "ServerConnection");

                     return;
                  }

                  // connection might be already closed (because of timed out or any other reasons)
                  // when this handler run. So we stop here.
                  if ( this->closed() )
                  {
                     this->_logger.debug("[{}] [conn:{}] Attempting to read data while already closed", this->logHttpType(), this->id());
                     return;
                  }

                  try
                  {
                     _totalBytesTransferred = _totalBytesTransferred + static_cast<int64_t>(bytesTransferred);
                     
                     if (this->_settings.logVerbose())
                        this->_logger.trace("[{}] [conn:{}] Received {} bytes", this->logHttpType(), this->id(), _totalBytesTransferred);

                     if (this->_parser.totalParsedBytes() == 0)
                     {
                        // TODO_JEFRI: FIX this: restart stopwatch in very beginning of receiving data
                        //this->_processingStopWatch.start();
                     }

                     auto info = this->_parser.parse(bytesTransferred);
                     if ( !info.success() )
                     {
                        // process error we got from parser, e.g: unsupported http method, 
                        // request too large, invalid header
                        handleRequestError(std::move(info));
                        return;
                     }

                     // We need more data
                     if ( this->_settings.enableMultipartParsing() 
                          && info.lastIndex() == this->_readBuffer.size()-1 
                          && (!this->_parser.contentDone() || !this->_parser.headersDone()) ) 
                     {
                        return read();
                     }

                     if ( info.httpStatus().code() == StatusCode::CONTINUE && !this->_parser.contentDone() )
                     {
                        handleExpect100Continue(); // send 100-continue response
                        return read();
                     }

                     if ( this->_parser.headersDone() || this->_parser.contentDone() )
                     {
                        if ( _httpContext == nullptr ||  // new connection
                             (_httpContext != nullptr && _httpContext->request()->id() != this->_parser.parsingId()) // new request in keep-alive connection
                           ) {
                           retrieveRequest(); // initialize _httpContext
                        }
                     }

                     // Redirect multipart parsing into a request handler (middleware)
                     if (    !this->_settings.enableMultipartParsing() 
                          && !this->_parser.contentDone() 
                          && this->_parser.hasMultipart()
                          /* && this->_parser.hasContentLength() && this->_parser.contentLength()*/ )
                     {
                        // create MultipartBodyReader with last _parser's state, to be used on MultipartBodyReader's read() 
                        // Note: Body reader use readBody() to retrieve data
                        std::shared_ptr<MultipartBodyReader> bodyReader = 
                           this->makeBodyReader(info.lastIndex(), bytesTransferred-info.bytesRead() );

                        // Multipart with Chunked Transfer Encoding
                        if (this->_parser.hasChunkedEncoding() && !this->_parser.hasContentLength())
                        {
                           this->_parser.parseChunkedMultipartHandler = bodyReader->chunkedHandler();
                           bodyReader->setProcessBodyStarter( this->_parser.processBodyStarter );
                           bodyReader->setMultipartWithChunkedTransferEncoding();
                        }

                        _httpContext->setBodyReader(std::move(bodyReader));

                        // The request handler must parse multipart body.
                        // handleRequest() will execute request handler until completed then call write() 
                        // which internally starting write timer, which will cancel read or process timer
                        return handleRequest();
                     }

                     if ( this->_parser.contentDone() )
                     {
                     
                        this->_logger.trace("[{}] [conn:{}] Received total {} bytes", this->logHttpType(), this->id(), _totalBytesTransferred);
                        
                        // get content from parser
                        _httpContext->request()->content( std::move( this->_parser.content() ) );

                        auto upgradeHeader = _httpContext->request()->headers().value("Upgrade");
                        if ( !upgradeHeader.empty() )
                           return handleUpgradeRequest(upgradeHeader);

                        // We got all headers and body. Cancel read timer by starting process timer. 
                        // Connection will close after process timer expired
                        // Note: request handler activity stil running in background
                        if (this->timeoutProcessing().value == 0)
                           this->cancelTimer();
                        else
                        {
                           // wa re going to handle the request, make sure request handler
                           // do the work within time out processing
                           this->startTimer(this->timeoutProcessing());
                        }
                        
                        // We are parsing multipart here, so get the result from parser
                        if ( this->_settings.enableMultipartParsing() && this->_parser.hasMultipart())
                           _httpContext->request()->multipartBody( std::move( this->_parser.multipartBody() ) );

                        // handleRequest() will execute request handler until completed then call write() 
                        // which internally starting write timer, which will cancel read or process timer
                        return handleRequest();
                     }
                     
                     // Read more data
                     {
                        return read();
                     }
                  }
                  catch(const std::exception& ex)
                  {
                     if (!this->closed())
                        this->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ServerConnection");
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
      //_httpContext->request()->content(      std::move( this->_parser.content() ) );

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
         else if (status == RequestStatus::handled) {
            self->write();
         }
         else if (status == RequestStatus::async)
         {
            // Do nothing here.
            // The middleware will call nextHandler(ctx) later,
            // and that path must eventually call write().
            auto x=1;
         }
      };

      // setup http context on complete handler (for async request handler)
      _httpContext->onCompleteHandler(processStatus);

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
            [this](std::error_code error, std::size_t) 
            {
               if (!error)
                  this->_parser.markContinueSent();

               if (error && !this->closed())
                  this->processError(this->id(), error, ErrorType::system, "ServerConnection");
            }
         )
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
   // Web Socket
   // -------------------------------------------------------

   /**
    * Handle WebSocket Upgrade request.
    * If request handler accept the upgrade request, it must set webSocketContext in httpContext
    * and _wsConnection will be set here.
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
         [this](WebSocketContextPtr ctx)
         {
            if (ctx)
            {
               _wsConnection = std::make_unique<ws::WebSocketConn>();
               _wsConnection->wsPtr = std::make_shared<WebSocket>(
                                          this->shared_from_this(), 
                                          _httpContext->userData(), 
                                          _httpContext->remoteEndpoint(),
                                          _httpContext->request()->headers() );

               _wsConnection->wsContext = ctx;
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

                  // TODO_JEFRI: should we force conection to close?
                  // set http context to close connection after successfull write
                  //_httpContext->keepAlive(false);

                  // tell client we are closing connection
                  //_httpContext->response()->addHeader("Connection", "close");

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
                  bool wsContextInitialized = (self->_wsConnection && self->_wsConnection->wsContext);
                  if (!wsContextInitialized)
                  {
                     self->_logger.error("[{}] [conn:{}] Websocket context not initialized", self->logHttpType(), self->id());
                     self->buildErrorResponse(StatusCode::INTERNAL_SERVER_ERROR);
                     self->write();
                     return;
                  }

                  // http request handler said OK to upgrade this http request to websocket
                  // now we can process this Upgrade request
                  auto secWebSocketKey = self->_httpContext->request()->headers().value("Sec-WebSocket-Key");
                  auto secWebSocketProtocol = self->_httpContext->request()->headers().value("Sec-WebSocket-Protocol");

                  if ( ! secWebSocketKey.empty() )
                  {
                     // Sec-WebSocket-Key  present in http headers, send WebSocket handshake response
                     response->headers().add("Upgrade", "websocket");
                     response->headers().add("Connection", "Upgrade");

                     static auto wsMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                     auto sha1Bytes   = crypt::hashSHA1Bytes(secWebSocketKey + wsMagicString);
                     auto sockAccept  = base64::encode(sha1Bytes);

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

   void wsSendFromQueue()
   {
      if (this->closed())
      {
         this->_logger.error("[{}] [conn:{}] wsSendFromQueue: connection with {} already closed", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         
         ErrorData error;
         error.message = "Connection already closed";
         wsHandleConnError(error);

         _wsConnection.reset();
         return;
      }

      this->_logger.trace("[{}] [conn:{}] Sending ws data to {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));

      std::array<asio::const_buffer, 2> buffers { 
         _wsConnection->sendQueue.begin()->outHeader->streambuf.data(), 
         _wsConnection->sendQueue.begin()->outMessage->streambuf.data() };

      this->startTimer(this->timeoutWrite());
      asio::async_write(
         this->_socket,
         buffers,
         [this](const std::error_code &error, std::size_t bytesTransferred)
         {
            this->cancelTimer();

            if (!error)
            {
               WsSendErrorHandler callback;

               auto it = _wsConnection->sendQueue.begin();
               if (it != _wsConnection->sendQueue.end())
               {
                  try
                  {
                     callback = std::move(it->callback);
                     _wsConnection->sendQueue.erase(it); // erase the element after moving the callback
                  }
                  catch(...)
                  {
                     this->_logger.error("[{}] [conn:{}] {} wsSendFromQueue: error erasing websocket OutData", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
                  }

               }

               if (_wsConnection->sendQueue.size() > 0)
                  wsSendFromQueue();

               if (callback)
               {
                  ErrorData err;
                  err.code    = error.value();
                  err.message = error.message();
                  callback(err);
               }
            }
            else
            {
               ErrorData err;
               err.code    = error.value();
               err.message = error.message();

               this->_logger.error("[{}] [conn:{}] {} wsSendFromQueue: error {} code {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint), error.message(), error.value());
               wsHandleConnError(err, bytesTransferred);
            }
         });
   }

   /// Send WebSocket Upgrade Response and start websocket communication
   virtual void writeWebSocketUpgradeResponse()
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
            [this, self = this->shared_from_this()] (std::error_code error, std::size_t)
            {
               if (!error)
               {
                  this->_processingStopWatch.stop();
                  this->_logger.info("[{}] [conn:{}] Upgrade request finished in {}, websocket established with {} ", 
                              this->logHttpType(),
                              this->id(),
                              this->_processingStopWatch.toString(this->_parser.parsingId(),true),
                              toString(_httpContext->remoteEndpoint()) );

                  this->_isWebSocket = true;
                  // add websocket to wsContext and call onOpen handler
                  _wsConnection->onOpen();

                  readWebSocket();
               }
               else
               {
                  if (!this->closed())
                  {
                     if (_wsConnection && _wsConnection->wsContext)
                        _wsConnection->onError(error, ErrorType::system);

                     this->processError(self->id(), error, ErrorType::system, "ServerConnection");
                  }
               }
            }) // asio::bind_executor
      ); // asio::async_write
   }

   void readWebSocket()
   {
      this->startTimer(this->timeoutRead());
      asio::async_read(
         this->_socket,
         _wsConnection->sendStreamBuf,
         asio::transfer_exactly(2),
         [this, self = this->shared_from_this()](const std::error_code& error, std::size_t bytesTransferred)
         {
            this->cancelTimer();

            if (!error)
            {
               if (bytesTransferred == 0)
               {
                  readWebSocket();
                  return;
               }

               std::istream istream(&_wsConnection->sendStreamBuf);
               std::array < unsigned char, 2 > firstBytes;
               istream.read((char * ) & firstBytes[0], 2);

               unsigned char finRsvOpcode = firstBytes[0];

               // Close connection if unmasked message from client (protocol error)
               if (firstBytes[1] < 128)
               {
                  const std::string reason("message from client not masked");
                  wsSendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
                  _wsConnection->onClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);

                  this->processCompleted(self->id(), "websocket, " + reason);
                  return;
               }

               std::size_t length = (firstBytes[1] & 127);

               if (length == 126)
               {
                  // 2 next bytes is the size of content
                  this->startTimer(this->timeoutRead());
                  asio::async_read(
                     this->_socket,
                     _wsConnection->sendStreamBuf,
                     asio::transfer_exactly(2),
                     [this, self = self->shared_from_this(), finRsvOpcode](const std::error_code& error, std::size_t /*bytesTransferred*/ )
                     {
                        this->cancelTimer();

                        if (!error)
                        {
                           std::istream istream(&_wsConnection->sendStreamBuf);

                           std::array<unsigned char,2> lengthBytes;
                           istream.read((char * ) & lengthBytes[0], 2);

                           std::size_t length = 0;
                           std::size_t numBytes = 2;
                           for (std::size_t c = 0; c < numBytes; c++)
                              length += static_cast < std::size_t > (lengthBytes[c]) << (8 * (numBytes - 1 - c));

                           wsReadMessageContent(length, finRsvOpcode);
                        }
                        else
                        {
                           if (!this->closed())
                           {
                              if (_wsConnection && _wsConnection->wsContext)
                                 _wsConnection->onError(error, ErrorType::system);

                              this->processError(self->id(), error, ErrorType::system, "ServerConnection");
                           }
                           return;
                        }
                     }
                  );
               }
               else if (length == 127)
               {
                  // 8 next bytes is the size of content
                  this->startTimer(this->timeoutRead());
                  asio::async_read(
                     this->_socket,
                     _wsConnection->sendStreamBuf,
                     asio::transfer_exactly(8),
                     [this, self = self->shared_from_this(), finRsvOpcode](const std::error_code& error, std::size_t /*bytesTransferred*/ )
                     {
                        this->cancelTimer();

                        if (!error)
                        {
                           std::istream istream(&_wsConnection->sendStreamBuf);

                           std::array<unsigned char,8> lengthBytes;
                           istream.read((char * ) & lengthBytes[0], 8);

                           std::size_t length = 0;
                           std::size_t numBytes = 8;
                           for (std::size_t c = 0; c < numBytes; c++)
                           {
                              length += static_cast < std::size_t > (lengthBytes[c]) << (8 * (numBytes - 1 - c));
                           }

                           wsReadMessageContent(length, finRsvOpcode);
                        }
                        else
                        {
                           if (!this->closed())
                           {
                              if (_wsConnection && _wsConnection->wsContext)
                                 _wsConnection->onError(error, ErrorType::system);

                              this->processError(self->id(), error, ErrorType::system, "ServerConnection");
                           }
                           return;
                        }
                     }); // asio::async_read()
               }
               else
                  wsReadMessageContent(length, finRsvOpcode);
            }
            else
            {
               if (!this->closed())
               {
                  if (_wsConnection && _wsConnection->wsContext)
                     _wsConnection->onError(error, ErrorType::system);

                  this->processError(self->id(), error, ErrorType::system, "ServerConnection");
               }
               return;
            }
         }); // asio::async_read()
   } // readWebSocket()

   void wsReadMessageContent(std::size_t length, unsigned char finRsvOpcode)
   {
      if (length + (_wsConnection->fragmentedInMessage ? _wsConnection->fragmentedInMessage->length : 0) > this->_settings.wsMessageMaxSize())
      {
         const int32_t status = WS_CLOSE_CODE_MESSAGE_TOO_BIG;
         const std::string reason = "message too big";

         wsSendClose(status, reason);
         _wsConnection->onClose(status, reason);

         this->processCompleted(this->id(), "websocket, " + reason);
         return;
      }

      this->startTimer(this->timeoutRead());
      asio::async_read(
         this->_socket,
         _wsConnection->sendStreamBuf,
         asio::transfer_exactly(4 + length),
         [this, length, finRsvOpcode, self = this->shared_from_this()](const std::error_code &error, std::size_t /*bytes_transferred*/)
         {
            this->cancelTimer();

            if (!error)
            {
               std::istream istream( & _wsConnection->sendStreamBuf );

               // Read mask
               std::array < unsigned char, 4> mask;
               istream.read((char*) &mask[0], 4);

               std::shared_ptr<ws::InMessage> inMessage;

               // If fragmented message
               if ((finRsvOpcode & 0x80) == 0 || (finRsvOpcode & 0x0f) == 0)
               {
                  if (!_wsConnection->fragmentedInMessage)
                  {
                     _wsConnection->fragmentedInMessage = std::shared_ptr<ws::InMessage> (new ws::InMessage(finRsvOpcode, length));
                     _wsConnection->fragmentedInMessage->finRsvOpcode |= 0x80;
                  }
                  else
                     _wsConnection->fragmentedInMessage->length += length;

                  inMessage = _wsConnection->fragmentedInMessage;
               }
               else
                  inMessage = std::shared_ptr<ws::InMessage> (new ws::InMessage(finRsvOpcode, length));

               std::ostream ostream(&inMessage->streambuf);
               for (std::size_t c = 0; c < length; c++)
                  ostream.put(istream.get() ^ mask[c % 4]);

               // If connection close
               if ((finRsvOpcode & 0x0f) == 8)
               {
                  int32_t status = 0;
                  if (length >= 2)
                  {
                     unsigned char byte1 = inMessage->get();
                     unsigned char byte2 = inMessage->get();
                     status = (static_cast<int32_t> (byte1) << 8) + byte2;
                  }

                  auto reason = inMessage->string();
                  wsSendClose(status, reason);
                  _wsConnection->onClose(status, reason);

                  this->processCompleted(self->id(), "websocket, " + reason);
                  return;
               }
               // If ping
               else if ((finRsvOpcode & 0x0f) == 9)
               {
                  // Send pong
                  auto outMessage = std::make_shared<ws::OutMessage> ();
                  *outMessage << inMessage->string();
                  wsSend(outMessage, nullptr, finRsvOpcode + 1);

                  _wsConnection->onPing();

                  // Next message
                  readWebSocket();
               }
               // If pong
               else if ((finRsvOpcode & 0x0f) == 10)
               {
                  _wsConnection->onPong();
                  // Next message
                  readWebSocket();
               }
               // If fragmented message and not final fragment
               else if ((finRsvOpcode & 0x80) == 0)
               {
                  // Next message
                  readWebSocket();
               }
               else
               {
                  _wsConnection->onMessage(inMessage->string());
                  // Next message
                  // Only reset _wsFragmentedInMessage for non-control frames (control frames can be in between a fragmented message)
                  _wsConnection->fragmentedInMessage = nullptr;
                  readWebSocket();
               }
            }
            else
            {
               if (!this->closed())
               {
                  if (_wsConnection && _wsConnection->wsContext)
                     _wsConnection->onError(error, ErrorType::system);

                  this->processError(self->id(), error, ErrorType::system, "ServerConnection");
               }
            }
         }
      );
   }

   /// finRsvOpcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
   /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information.
   void wsSend(std::shared_ptr<ws::OutMessage> outMessage, WsSendErrorHandler callback = nullptr, unsigned char finRsvOpcode = 129)
   {
      if (this->closed())
      {
         this->_logger.debug("[{}] [conn:{}] WebSocket connection with {} already closed", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         return;
      }

      std::size_t length = outMessage->size();
      auto outHeader = std::make_shared<ws::OutMessage>(10); // Header is at most 10 bytes
      outHeader->put(static_cast<char>(finRsvOpcode));
      // Unmasked (first length byte<128)
      if (length >= 126)
      {
         std::size_t numBytes;
         if(length > 0xffff)
         {
            numBytes = 8;
            outHeader->put(127);
         }
         else
         {
            numBytes = 2;
            outHeader->put(126);
         }

         for(std::size_t c = numBytes - 1; c != static_cast<std::size_t>(-1); c--)
            outHeader->put((static_cast<uint64_t>(length) >> (8 * c)) % 256);
      }
      else
         outHeader->put(static_cast<char>(length));

      _wsConnection->sendQueue.emplace_back(std::move(outHeader), std::move(outMessage), std::move(callback));
      if (_wsConnection->sendQueue.size() == 1)
         wsSendFromQueue();
   }

   /// Convenience function for sending a string.
   /// finRsvOpcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
   /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information.
   void wsSend(std::string_view outMessageStr, WsSendErrorHandler callback = nullptr, unsigned char finRsvOpcode = 129)
   {
      if (this->closed())
      {
         this->_logger.debug("[{}] [conn:{}] WebSocket connection with {} already closed", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         return;
      }

      auto outMessage = std::make_shared<ws::OutMessage>();
      outMessage->write(outMessageStr.data(), static_cast<std::streamsize>(outMessageStr.size()));
      wsSend(outMessage, std::move(callback), finRsvOpcode);
   }

   void wsSendClose(int32_t status, const std::string &reason = "", WsSendErrorHandler callback = nullptr)
   {
      // Send close only once (in case close is initiated by server)
      if (_wsConnection->closed)
         return;

      _wsConnection->closed = true;

      auto sendStream = std::make_shared<ws::OutMessage>();

      sendStream->put(status >> 8);
      sendStream->put(status % 256);

      *sendStream << reason;

      // finRsvOpcode=136: message close
      wsSend(std::move(sendStream), std::move(callback), 136);
   }

   void wsSendBinary(const std::string& data, WsSendErrorHandler callback = nullptr)
   {
      if (this->closed())
      {
         this->_logger.debug("[{}] [conn:{}] WebSocket connection with {} already closed", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         return;
      }

      wsSend(data, callback, (unsigned char)130);
   }

   void wsSendText(const std::string& data, WsSendErrorHandler callback = nullptr)
   {
      if (this->closed())
      {
         this->_logger.debug("[{}] [conn:{}] WebSocket connection with {} already closed", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         return;
      }

      wsSend(data, callback, (unsigned char)129);
   }

   void wsHandleConnError(const http::ErrorData& error, std::size_t bytes_transferred=-1)
   {
      // All handlers in the queue is called with error
      std::vector<WsSendErrorHandler> callbacks;
      for (auto &outData : _wsConnection->sendQueue)
      {
         try
         {
            if (outData.callback)
               callbacks.emplace_back(std::move(outData.callback));
         }
         catch(...)
         {
            this->_logger.error("[{}] [conn:{}] {} invalid WebSocket outData callback", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
         }
      }

      _wsConnection->sendQueue.clear();

      for (auto &callback : callbacks)
         callback(error);
   }

#ifdef TOBASA_HTTP_USE_HTTP2
   // -------------------------------------------------------
   // HTTP/2
   // -------------------------------------------------------

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
         this->id(), &this->_sendBuffer, this->_http2Option);

      this->_http2Session->writeHandler(
         [this](http2::HandlerCallback cb)
         {
            asio::post(this->executor(),
               [this, cb=std::move(cb)] () {
                  cb(); // ses->writeSignaled(false);

                  if (this->_http2Option->logVerbose)
                     this->_logger.trace("[{}] [conn:{}] writeHandler call writeHttp2", this->logHttpType(), this->id());

                  this->writeHttp2();
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

   virtual void readHttp2()
   {
      this->startTimer(this->timeoutRead());
      this->_socket.async_read_some(
         this->getReadBuffer(),
         asio::bind_executor(
            this->executor(),
            [this, self =  this->shared_from_this()](std::error_code error, size_t bytesTransferred)
            {
               this->cancelTimer();

               if (!error)
               {
                  // connection might be already closed(because of timed out or any other reasons)
                  // when this handler run. so we stop here.
                  if ( this->closed() )
                  {
                     this->_logger.trace("[{}] [conn:{}] [http2] read, attempting to read data while already closed", this->logHttpType(), this->id());
                     return;
                  }
                  else
                  {
                     if (this->_http2Option->logVerbose)
                     {
                        this->_logger.trace("[{}] [conn:{}] [http2] read,  Receive {} bytes", this->logHttpType(), this->id(), bytesTransferred);
                        this->_logger.trace("[{}] [conn:{}] [http2] read,  calling readIncomingData", this->logHttpType(), this->id());
                     }

                     auto result = this->_http2Session->readIncomingData(this->getReadBuffer(), bytesTransferred);
                     if (! result.success() )
                     {
                        this->_logger.error("[{}] [conn:{}] {}", this->logHttpType(), this->id(), result.message() );
                        this->processError(self->id(), result.message(), ErrorType::internal, ERROR_CODE_READ_FAIL , "ServerConnection");
                        return;
                     }

                     if (this->_http2Option->logVerbose)
                        this->_logger.trace("[{}] [conn:{}] [http2] read, call writeHttp2", this->logHttpType(), this->id());

                     this->writeHttp2();

                     if ( !this->_http2Session->writingData() &&  this->_http2Session->shouldStop())
                     {
                        this->processCompleted(self->id(), "all HTTP/2 data processed [R]");
                        return;
                     }

                     // Once data is written, ensure to continue reading for further frames
                     if (this->_http2Option->logVerbose)
                        this->_logger.trace("[{}] [conn:{}] [http2] read,  call readHttp2", this->logHttpType(), this->id());

                     this->readHttp2();
                  }
               }
               else
               {
                  if (! this->closed())
                     this->processError(self->id(), error, ErrorType::system, "ServerConnection");
               }
            })
      );
   }

   virtual void writeHttp2()
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
         if (this->_http2Session->shouldStop())
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
            [this, self = this->selfPtr(), bytesToTransfer] (std::error_code error, std::size_t bytesTransferred)
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
            }) // asio::bind_executor
      ); // asio::async_write
   }

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
      //this->_logger.trace("[{}] [conn:{}] HTTP/2 Request parsed successfully, preparing request and response object, streamId:{}", this->logHttpType(), this->id(), streamId);

      auto httpContext = std::make_shared<Context>(
              std::make_shared<Request>(HttpVersion::two)
            , std::make_shared<Response>(HttpVersion::two)
            , this->_remoteEndpoint
            , this->id()
            , streamId
            , HttpVersion::two
            , streamId );

      auto streamData = this->_http2Session->findStream(streamId);

      httpContext->request()->id( streamId );
      httpContext->request()->setHttps( this->isTls() );

      httpContext->request()->method( streamData->requestHeader.method );
      // HEAD method is similar to GET, but we should not return the body of the response
      if (httpContext->request()->method() == "HEAD")
      {
         httpContext->request()->method("GET");
         httpContext->request()->isHeadRequest(true);
         httpContext->response()->isHeadRequest(true);
      }

      httpContext->request()->authority(     streamData->requestHeader.authority );
      httpContext->request()->target(        streamData->requestHeader.path );
      //httpContext->request()->path(          streamData->requestHeader.path );
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


      auto processStatus = 
      [self = this->selfPtr(), ctx=httpContext, sid=streamId](RequestStatus status)
      {
         if (status == RequestStatus::notHandled) {
            self->buildErrorResponse(StatusCode::NOT_FOUND);
         }
         else if (status == RequestStatus::handled) {
            auto x=1;
         }
         else if (status == RequestStatus::async)
         {
            // should not be called here, async middlewares keep control
            return http2::Result(0,"",sid);
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

      // execute request handler to get response
      RequestStatus status = _requestHandler(httpContext);
      return processStatus(status);
   }

#endif // TOBASA_HTTP_USE_HTTP2
};

/** @}*/

} // namespace http
} // namespace tbs