#pragma once

//#include "tobasahttp/client/common.h"
#include "tobasahttp/http_connection.h"
#include "tobasahttp/exception.h"
#include "tobasahttp/util.h"
#include "tobasahttp/client/response.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Http Client connection
 * \tparam Traits
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
   ResponseHandler           _responseHandler;
   /// request to send
   std::unique_ptr<Request>  _pRequest;

   std::function<void()>     _responseHandledCb;
   bool                     _busy {false};
   size_t                   _requestsHandled {0};
   size_t                   _maxRequests {200};
   std::atomic<uint32_t>    _currentRequestId { 0 };

public:

   explicit ClientConnection(
      Socket          socket,
      Settings&       settings,
      Logger&         logger,
      ResponseHandler handler)
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
         [this, self = this->shared_from_this()] (const std::error_code& error)
         {
            this->_remoteEndpoint = this->_socket.lowest_layer().remote_endpoint();

            if (!error)
            {
               this->_logger.debug("[{}] [conn:{}] Start sending request to {}", this->logHttpType(), this->id(), toString(this->_remoteEndpoint));
               write();
            }
            else
               this->processError(self->id(), error, ErrorType::system, "ClientConnection");
         }
      );
  }

   virtual void write()
   {
      this->_logger.info("[{}] [conn:{}] Processing request to: {} {} {} {}", this->logHttpType(),
         this->id(), toString(this->_remoteEndpoint), _pRequest->target(),
         _pRequest->contentType(), _pRequest->contentLength() );

      std::ostream sendStream(&this->_sendBuffer);
      RequestSerializer serializer(*_pRequest);

      auto rawRequest = serializer.getString();
      sendStream << std::move(rawRequest);

      asio::async_write(
         this->_socket,
         this->_sendBuffer,
         asio::bind_executor(
            this->executor(),
            [this, self = this->shared_from_this()](std::error_code error, std::size_t)
            {
               if (!error)
               {
                  this->_processingStopWatch.stop();

                  // start async read, this will return immediately
                  read();
                  // fire up read timer
                  this->startTimer(this->timeoutRead());
               }
               else
               {
                  this->processError(self->id(), error, ErrorType::system, "ClientConnection");
               }
         })
      );

      // async_write return immediately, fire up write timer
      this->startTimer(this->timeoutWrite());
   }

   /// Set request to be send to server
   void request(std::unique_ptr<Request> request)
   {
      _pRequest = std::move(request);
   }

   /// Set request to be send to server
   void request(Request&& request)
   {
      _pRequest = std::make_unique<Request>( std::forward<Request>( request ));
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

   virtual void read()
   {
      if ( ! this->_parser.isReading() )
      {
         this->_parser.isReading(true);

         this->_socket.async_read_some(
            this->getReadBuffer(),
            asio::bind_executor(
               this->executor(),
               [this, self = this->shared_from_this()](std::error_code error, size_t bytesTransferred)
               {
                  this->_parser.isReading(false);

                  if ( !error )
                  {
                     // connection might be already closed(because of timed out or any other reasons)
                     // when this handler run. so we stop here.
                     if ( this->closed() )
                     {
                        this->_logger.warn("[{}] [conn:{}] Attempting to read data while already closed", this->logHttpType(), this->_connId);
                        return;
                     }
                     else
                     {
                        try
                        {
                           this->_logger.trace("[{}] [conn:{}] Received {} bytes", this->logHttpType(), this->_connId, bytesTransferred);

                           auto info = this->_parser.parse(bytesTransferred);
                           if ( info.success() )
                           {
                              if ( this->_parser.contentDone() )
                                 retrieveResponse();
                              else
                                 read();
                           }
                           else
                              this->processError(self->id(), std::string{info.message()}, ErrorType::internal, ERROR_CODE_PARSER_FAIL, "ClientConnection");
                        }
                        catch(const std::exception& ex)
                        {
                           this->processError(self->id(), ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, "ClientConnection");
                        }
                     }
                  }
                  else {
                     this->processError(self->id(), error, ErrorType::system, "ClientConnection");
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

         _responseHandler(resp);

         if (_requestsHandled >= this->_settings.maxRequestsPerConnection() && this->_settings.maxRequestsPerConnection()!=0 )
         {
            this->processCompleted(this->id(), "Request completed");
            return;
         }

         if (shouldKeepAlive(resp))
         {
            if (_responseHandledCb)
               _responseHandledCb();   // return to pool

            // we are in keep-alive connection
            this->_parser.prepareForNextMessage();
            auto curentRequestId = ++_currentRequestId;   // get new request id  
            this->_parser.parsingId( curentRequestId);    // set new request id  as parsing id
            this->_processingStopWatch.lap(curentRequestId, "start" );
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

   bool shouldKeepAlive(ResponsePtr resp)
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


};

/** @}*/

} // namespace http
} // namespace tbs