#pragma once

#include "tobasahttp/http_connection.h"
#include "tobasahttp/exception.h"
#include "tobasahttp/util.h"
#include "tobasahttp/client/request.h"
#include "tobasahttp/client/request_serializer.h"
#include "tobasahttp/client/response.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * \brief HTTP client connection to a remote server.
 *
 * This class owns the client-side socket, serializes HTTP requests, sends
 * them to the remote server, parses the responses, and passes each response
 * to the configured response handler. HTTP/1 keep-alive connections can be
 * reused for multiple requests until the connection or request limit is
 * reached.
 *
 * \tparam Traits Provides the socket, settings, and logger types used by the
 * connection.
 */
template <class Traits>
class ClientConnection
   : public HttpConnection<Traits>
{
public:
   using Socket   = typename Traits::Socket;
   using Settings = typename Traits::Settings;
   using Logger   = typename Traits::Logger;

private:
   ClientResponseHandler          _responseHandler;
   ClientRequestErrorHandler      _requestErrorHandler;
   /// request to send
   std::unique_ptr<ClientRequest> _pRequest;

   std::function<void()>    _responseHandledCb;
   bool                     _busy {false};
   size_t                   _requestsHandled {0};
   size_t                   _maxRequests {200};
   std::atomic<uint32_t>    _currentRequestId { 0 };

   void reportRequestError(const ErrorData& error)
   {
      auto handler = std::move(_requestErrorHandler);
      _requestErrorHandler = nullptr;
      if (handler)
         handler(error);
   }

public:

   explicit ClientConnection(
      Socket                socket,
      Settings&             settings,
      Logger&               logger,
      ClientResponseHandler handler)
      : HttpConnection<Traits> { std::move(socket), settings, logger }
      , _responseHandler { std::move(handler) }
   {
      this->_instanceType = InstanceType::http_client;
      this->_parser.type(parser::Type::RESPONSE);
   }

   virtual ~ClientConnection()
   {
      this->_logger.trace("[{}] [conn:{}] destroyed", this->logHttpType(), this->id());
   }

   /// start() called by connection manager
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
         [self = this->selfPtr()] (const std::error_code& error)
         {
            asio::dispatch(self->executor(), [self, error]
            {
               self->_remoteEndpoint = self->_socket.lowest_layer().remote_endpoint();

               if (!error)
               {
                  self->_logger.debug("[{}] [conn:{}] Start sending request to {}", self->logHttpType(), self->id(), toString(self->_remoteEndpoint));
                  self->write();
               }
               else
                  self->processError(self->id(), error, ErrorType::system, "ClientConnection");
            });
         }
      );
  }

   virtual void write()
   {
      this->_logger.info("[{}] [conn:{}] Processing request to: {} {} {} {}", this->logHttpType(),
         this->id(), toString(this->_remoteEndpoint), this->_pRequest->target(),
         this->_pRequest->contentType(), this->_pRequest->contentLength() );

      std::ostream sendStream(&this->_sendBuffer);
      RequestSerializer serializer(*this->_pRequest);

      auto rawRequest = serializer.getString();
      sendStream << std::move(rawRequest);

      asio::async_write(
         this->_socket,
         this->_sendBuffer,
         asio::bind_executor(
            this->executor(),
            [self=this->selfPtr()](std::error_code error, std::size_t)
            {
               if (!error)
               {
                  self->_processingStopWatch.stop();

                  // start async read, this will return immediately
                  self->read();
                  // fire up read timer
                  self->startTimer(self->timeoutRead());
               }
               else
               {
                  self->processError(self->id(), error, ErrorType::system, "ClientConnection");
               }
         })
      );

      // async_write return immediately, fire up write timer
      this->startTimer(this->timeoutWrite());
   }

   void request(
      std::unique_ptr<ClientRequest> request,
      ClientResponseHandler handler,
      ClientRequestErrorHandler errorHandler)
   {
      _pRequest            = std::move(request);
      _responseHandler     = std::move(handler);
      _requestErrorHandler = std::move(errorHandler);
   }

   void sendRequest(
      std::unique_ptr<ClientRequest> request,
      ClientResponseHandler handler,
      ClientRequestErrorHandler errorHandler)
   {
      auto self = this->selfPtr();
      asio::dispatch(this->executor(),
         [self, request = std::move(request), handler = std::move(handler),
          errorHandler = std::move(errorHandler)]() mutable
         {
            if (self->closed())
               return;

            self->_pRequest            = std::move(request);
            self->_responseHandler     = std::move(handler);
            self->_requestErrorHandler = std::move(errorHandler);
            self->_parser.parsingId(++self->_currentRequestId);
            self->_processingStopWatch.start();
            self->_processingStopWatch.lap(self->_parser.parsingId(), "start");
            self->write();
         });
   }

   void responseHandledCb(std::function<void()> cb)
   {
      _responseHandledCb = std::move(cb);
   }

   bool busy() const
   {
      return _busy;
   }

   void markBusy()
   {
      _busy = true;
   }

   void markIdle()
   {
      _busy = false;
   }


protected:

   std::function<void(typename HttpConnection<Traits>::Timer&)> createTimerHandler(
      TimerType timerType,
      int32_t timerWaitId,
      int32_t timeoutSeconds) override
   {
      auto defaultHandler = HttpConnection<Traits>::createTimerHandler(
         timerType, timerWaitId, timeoutSeconds);

      return [this, timerType, timeoutSeconds,
              defaultHandler = std::move(defaultHandler)](auto& timer)
      {
         const auto message = tbsfmt::format("{} Timer expired after {} seconds",
            timerTypeToString(timerType), timeoutSeconds);
         
         reportRequestError(ErrorData{
            message, ErrorType::system, ERROR_CODE_DATA_FAIL, this->id(), "ClientConnection"});
         
         defaultHandler(timer);
      };
   }

   void processError(
      ConnectionId connId,
      const std::string& message,
      ErrorType errorType,
      int32_t code,
      const std::string& source = "") override
   {
      reportRequestError(ErrorData{message, errorType, code, connId, source});
      HttpConnection<Traits>::processError(connId, message, errorType, code, source);
   }

   void processError(
      ConnectionId connId,
      const std::error_code& error,
      ErrorType errorType,
      const std::string& source = "") override
   {
      if (error != asio::error::operation_aborted)
      {
         reportRequestError(ErrorData{
            error.message(), errorType, error.value(), connId, source});
      }
      HttpConnection<Traits>::processError(connId, error, errorType, source);
   }

   virtual void read()
   {
      if ( ! this->_parser.isReading() )
      {
         this->_parser.isReading(true);

         this->_socket.async_read_some(
            this->getReadBuffer(),
            asio::bind_executor(
               this->executor(),
               [self=this->selfPtr()](std::error_code error, size_t bytesTransferred)
               {
                  self->_parser.isReading(false);

                  if ( !error )
                  {
                     // connection might be already closed (because of timed out or any other reasons)
                     // when this handler run. so we stop here.
                     if ( self->closed() )
                     {
                        self->_logger.warn("[{}] [conn:{}] Attempting to read data while already closed", self->logHttpType(), self->_connId);
                        return;
                     }
                     else
                     {
                        try
                        {
                           self->_logger.trace("[{}] [conn:{}] Received {} bytes", self->logHttpType(), self->_connId, bytesTransferred);

                           auto info = self->_parser.parse(bytesTransferred);
                           if ( info.success() )
                           {
                              if ( self->_parser.contentDone() )
                                 self->retrieveResponse();
                              else
                                 self->read();
                           }
                           else
                              self->processError(self->id(), std::string{info.message()}, ErrorType::internal, ERROR_CODE_PARSER_FAIL, "ClientConnection");
                        }
                        catch(const std::exception& ex)
                        {
                           self->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ClientConnection");
                        }
                     }
                  }
                  else {
                     self->processError(self->id(), error, ErrorType::system, "ClientConnection");
                  }
            })
         );
      }
      else
      {
         this->_logger.error("[{}] [conn:{}] Read operation already running, closing connection", this->logHttpType(), this->_connId);
         this->processError(this->id(), "Read operation but parser is not reading", ErrorType::internal, ERROR_CODE_PARSER_FAIL ,"ClientConnection");
      }
   }

   void retrieveResponse()
   {
      this->_logger.trace("[{}] [conn:{}] Response parsed successfully, preparing response object", this->logHttpType(), this->_connId);

      _requestsHandled++;

      auto resp = std::make_shared<ClientResponse>(HttpVersion::one);

      if (this->_parser.headersDone() && this->_parser.contentDone())
      {
         resp->majorVersion( this->_parser.majorVersion() );
         resp->minorVersion( this->_parser.minorVersion() );
         resp->headers     ( std::move( this->_parser.headers() ) );
         resp->content     ( std::move( this->_parser.content() ) );

         HttpStatus status(static_cast<StatusCode>(this->_parser.statusCode()), this->_parser.statusMessage());
         resp->httpStatus  ( std::move(status) );

         bool keepAlive = shouldKeepAlive(resp);
         if (_requestsHandled >= this->_settings.maxRequestsPerConnection() &&
             this->_settings.maxRequestsPerConnection() != 0)
            keepAlive = false;

         if (keepAlive)
            this->_parser.prepareForNextMessage();

         auto responseHandler = std::move(_responseHandler);
         _responseHandler     = nullptr;
         _requestErrorHandler = nullptr;

         if (responseHandler)
            responseHandler(resp);

         if (keepAlive)
         {
            if (!this->closed() && _responseHandledCb)
               _responseHandledCb();
         }
         else
         {
            this->processCompleted(this->id(), "Request completed");
            return;
         }
      }
      else
         throw http::Exception("Incomplete HTTP response");
   }

   bool shouldKeepAlive(ClientResponsePtr resp)
   {
      // HTTP/1.1 default = keep-alive
      if (resp->majorVersion() == 1 && resp->minorVersion() == 1)
      {
         auto connHeader = resp->headers().value("Connection");
         if (connHeader == "close")
            return false;
         
         return true;
      }

      // HTTP/1.0 default = close unless keep-alive
      if (resp->majorVersion() == 1 && resp->minorVersion() == 0)
      {
         auto connHeader = resp->headers().value("Connection");
         if (util::startsWith(connHeader, "keep-alive"))
            return true;

         return false;
      }

      return false;
   }

   std::shared_ptr<ClientConnection> selfPtr() 
   {
      return std::static_pointer_cast<ClientConnection>(this->shared_from_this());
   }

};

/** @}*/

} // namespace http
} // namespace tbs