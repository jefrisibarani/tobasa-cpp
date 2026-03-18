#pragma once

#include <functional>

#include "tobasahttp/http_parser.h"
#include "tobasahttp/request.h"
#include "tobasahttp/response.h"
#include "tobasahttp/stopwatch.h"
#include "tobasahttp/connection.h"
#include "tobasahttp/util.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * \brief HTTP Connection
 * \tparam Traits
 */
template <class Traits>
class HttpConnection
   : public Connection
{
public:
   using Socket           = typename Traits::Socket;
   using Settings         = typename Traits::Settings;
   using Logger           = typename Traits::Logger;
   using ResponsePtr      = std::shared_ptr<Response>;
   using RequestPtr       = std::shared_ptr<Request>;
   using AsioExecutor     = asio::any_io_executor;

   /** 
    * \brief Executor type for async operation
    * in single in single threaded, the type is asio::any_io_executor
    * in multithreaded the type is asio::strand<asio::any_io_executor>
    */
   using Executor         = asio::strand<AsioExecutor>;
   using Timer            = asio::steady_timer;

   // In TLS socket OnStartCallback, is actually 
   using OnStartCallback  = std::function<void(const std::error_code&)>;
   using OnStartHandler   = std::function<void(Socket& socket, OnStartCallback)>;

protected:

   struct Timeout
   {
      int32_t           value  {0};
      TimerType         type;
      std::string_view  name   {};
   };

   Socket                  _socket;
   Settings&               _settings;
   Logger&                 _logger;
   /// Receive/read Buffer
   std::vector<char>       _readBuffer;
   /// Send/write Buffer
   asio::streambuf         _sendBuffer;

   parser::Parser          _parser;
   Timer                   _timer;

   /// we use strand to ensure the connection's handlers are not called concurrently.
   Executor                _executor;
   StopWatch               _processingStopWatch;

   /// Reading request timer
   Timeout                 _timeoutRead;
   /// Writing response time
   Timeout                 _timeoutWrite;
   /// Request handler processing timer
   Timeout                 _timeoutProcessing;
   inline static int32_t   _timerWaitId {0};

   // For logging purpose
   asio::ip::tcp::endpoint _remoteEndpoint;

public:
   OnStartHandler          onStart;

public:
   explicit HttpConnection(
      Socket    socket,
      Settings& settings,
      Logger&   logger)
      : _socket     { std::move(socket) }
      , _executor   { _socket.get_executor() }
      , _settings   { settings }
      , _logger     { logger }
      , _readBuffer ( _settings.readBufferSize() )
      , _sendBuffer {}
      , _parser     { parser::Type::REQUEST, _readBuffer, 
                      _settings.maxHeaderSize(), _settings.enableMultipartParsing(), _settings.temporaryDir() }
      , _timer      { _socket.get_executor() }
   {
      _closed = false;
      timeoutRead( _settings.timeoutRead());
      timeoutWrite( _settings.timeoutWrite());
      timeoutProcessing( _settings.timeoutProcessing());
   }

   virtual ~HttpConnection()
   {
      onStart = nullptr;
   }

   auto & executor() noexcept { return _executor; }

   Socket& socket()
   {
      return _socket;
   }

   virtual void start() = 0;

   virtual void write() = 0;

   virtual void read()  = 0;

   virtual void close()
   {
      if ( _closed )
      {
         if (_settings.logVerbose())
            _logger.trace("[{}] [conn:{}] Already closed", logHttpType(), this->id() );
         
         return;
      }
      else
      {
         if (_settings.logVerbose())
            _logger.trace("[{}] [conn:{}] Cancelling all timer handler", logHttpType(), this->id() );

         _timer.cancel();


         if (_settings.logVerbose())
            _logger.trace("[{}] [conn:{}] Shutting down socket", logHttpType(), this->id() );
         
         shutdown();


         if (_settings.logVerbose())
            _logger.trace("[{}] [conn:{}] Closing socket", logHttpType(), this->id() );

         asio::error_code error;
         _socket.lowest_layer().close(error);
         if (error) {
            if (_settings.logVerbose())
               _logger.trace("[{}] [conn:{}] {}", logHttpType(), this->id(), error.message() );
         }

         _closed = true;

         if (_onClosed)
            _onClosed( id() );
      }
   }

   virtual void shutdown()
   {
      // Initiate graceful connection closure.
      asio::error_code error;
      _socket.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
      if (error)
      {
         // Note: error is : The file handle supplied is not valid
         //_logger.trace("[{}] [conn:{}] {}", logHttpType(), this->id(), error.message() );
      }
   }

   virtual asio::ip::tcp::endpoint remoteEndpoint()
   {
      return _remoteEndpoint;
   }

protected:

   Timeout timeoutRead() const { return _timeoutRead; }
   Timeout timeoutWrite() const { return _timeoutWrite; }
   Timeout timeoutProcessing() const { return _timeoutProcessing; }

   void timeoutRead(int32_t value)
   {
      _timeoutRead.value = value;
      _timeoutRead.type = TimerType::read;
      _timeoutRead.name  = "Read";
   }

   void timeoutWrite(int32_t value)
   {
      _timeoutWrite.value = value;
      _timeoutWrite.type = TimerType::write;
      _timeoutWrite.name  = "Write";
   }

   void timeoutProcessing(int32_t value)
   {
      _timeoutProcessing.value = value;
      _timeoutProcessing.type = TimerType::process;
      _timeoutProcessing.name  = "Process";
   }

   /** 
    * \brief Get Receive/read Buffer
    * \return asio::const_buffer
    */
   auto getReadBuffer() noexcept
   {
      return asio::buffer( _readBuffer.data(), _readBuffer.size() );
   }

   virtual void processError(ConnectionId connId, const std::string& message, ErrorType errorType, int32_t code, const std::string& source="")
   {
      ErrorData errorData = { message, errorType, code, connId, source };
      if (_onError)
         _onError(std::move(errorData));
   }

   virtual void processError(ConnectionId connId, const std::error_code& error, ErrorType errorType, const std::string& source="")
   {
      ErrorData errorData = { error.message(), errorType, error.value(), connId, source};

      if (error == asio::error::operation_aborted)
         _logger.debug("[{}] [conn:{}] Error code: {}, {}", logHttpType(), connId, errorData.code, errorData.message);
      else if (error == asio::error::connection_reset || error == asio::error::eof)
      {
         if (_onComplete)
            _onComplete(connId, error.message());
      }
      else
      {
         // Expired timer, will trigger onTimeOut event which is handled by connection manager,
         // which will close/destroy this connection
         // such operation produce error code: 1236:  "The network connection was aborted by the local system."
         if (_onError)
            _onError(std::move(errorData));
      }
   }

   virtual void processCompleted(ConnectionId connId, const std::string& message)
   {
      if (_onComplete)
         _onComplete(connId, message );
   }

   /** 
    * \brief Create default timer handler.
    * Used when no handler specified in startTimer()
    * this handler will be called by _timer's completion handler, only if connection still alive.
    */
   virtual std::function<void(Timer&)> createTimerHandler(TimerType timerType, int32_t timerWaitId, int32_t timeoutSeconds)
   {
      return
         [this, timerType, timerWaitId, timeoutSeconds] (Timer& timer)
         {
            if (timer.expiry() <= Timer::clock_type::now())
            {
               
               _logger.trace("[{}] [conn:{}] {} Timer [{}] timed out", logHttpType(), this->id(), timerTypeToString(timerType), timerWaitId);
               
               if (_onTimeOut)
               {
                  std::string message = tbsfmt::format("{} Timer expired after {} seconds", timerTypeToString(timerType), timeoutSeconds );
                  TimeOutData data = { this->id(), timerType, message, timeoutSeconds };
                  _onTimeOut(std::move(data));
               }
            }

            // There is no longer an active deadline. The expiry is set to the
            // maximum time point so that the actor takes no action until a new
            // deadline is set.
            timer.expires_at(Timer::time_point::max());
         };
   }

   void startTimer(int32_t timeoutSeconds, TimerType timerType, std::function<void(Timer&)> handler = nullptr)
   {
      // this will cancel previous timer
      auto cancelled = _timer.expires_after(std::chrono::seconds(timeoutSeconds));

      // pass connection's weak pointer to async_wait's handler, to make sure we operate on a live connection
      // because at the time async_wait's handler run, connection may already destroyed by  onComplete event.
      std::weak_ptr<Connection> connWeak = shared_from_this();

      int32_t timerWaitId = ++_timerWaitId;

      // no handler given, use default handler.
      if (handler == nullptr){
         handler = createTimerHandler(timerType, timerWaitId, timeoutSeconds);
      }

      // We managed to cancel the timer. Start new asynchronous wait.
      _timer.async_wait(
         [this,
          timeoutSeconds,
          timerType,
          connWeak = std::move(connWeak),
          handler  = std::move(handler),
          connId   = this->id(),
          timerWaitId ] (const auto & error)
         {
            if (error == asio::error::operation_aborted) {
               if (_settings.logVerbose())
                  _logger.trace("[{}] [conn:{}] {} Timer [{}] aborted", logHttpType(), connId, timerTypeToString(timerType), timerWaitId);
            }

            if (auto tmpSelf = connWeak.lock())
            {
               if (tmpSelf->closed())
                  return;

               if ( !error )
                  handler(_timer);
            }
         }
      );

      if (_settings.logVerbose())
         _logger.trace("[{}] [conn:{}] {} Timer [{}] started", logHttpType(), this->id(), timerTypeToString(timerType), timerWaitId);
   }

   void startTimer(Timeout timeout, std::function<void(Timer&)> handler = nullptr)
   {
      if (_settings.logVerbose())
         _logger.trace("[{}] [conn:{}] Starting Timer [{}]", logHttpType(), this->id(), timeout.name);
      startTimer(timeout.value, timeout.type, std::move(handler));
   }

   void cancelTimer()
   {
      if (_settings.logVerbose())
         _logger.trace("[{}] [conn:{}] Cancelling timer", logHttpType(), this->id() );
      _timer.cancel();
   }

   std::string logHttpType() const
   {
      return logHttpTypeInfo(_instanceType, _settings.tlsMode());
   }

};

/** @}*/

} // namespace http
} // namespace tbs