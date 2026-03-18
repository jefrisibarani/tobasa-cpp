#pragma once

#ifdef TOBASA_HTTP_USE_HTTP2

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <mutex>
#include <asio/streambuf.hpp>
#include <nghttp2/nghttp2.h>
#include "tobasahttp/type_common.h"
#include "tobasahttp/headers.h"
#include "tobasahttp/server/string_ref.h"
#include "tobasahttp/server/settings_tls.h"
#include "tobasahttp/multipart_parser.h"

namespace tbs {
namespace http { class Response; }
namespace http2 {

class Result
{
private:
   bool        _success  {true};
   std::string _message  {};
   int         _code     {0};
   int32_t     _streamId {-1};

public:

   Result(int code, const std::string& message={}, int32_t streamId=-1);
   Result() = default;
   ~Result();// = default;

   static Result fail(const std::string& message, int32_t streamId=-1);

   void success(bool success) { _success = success; }
   void message(std::string message) { _message = message; }
   bool success() { return _success; }
   std::string message() { return _message; }
   int code() { return _code; }
   int32_t streamId() { return _streamId; }
};

struct RequestHeader 
{
   // mandatory
   std::string authority;
   std::string path;
   std::string method;
   std::string scheme;

   std::string host;
   std::string ims;
   std::string expect;
   std::string protocol;

   size_t contentLength {0};
};

class Http2StreamData
{
public:
   Http2StreamData(int32_t id, http::ConnectionId connId);
   
   ~Http2StreamData();

   int32_t id() { return _id;}

   void id(int32_t id ) { _id = id; }

   http::ConnectionId connId() { return _connId; }

   void initMultipartParser(const std::string& temporaryDir);

   http::parser::MultipartParser& multipartParser();

   // Total amount of bytes (sum of name and value length) used in headers.
   size_t headerBufferSize = 0;

   RequestHeader requestHeader;

   // Http headers, which will moved to HttpContext's Request
   http::Headers requestHeaders;

   std::string requestBody;

   bool hasContentLength {false};
   bool hasMultipartBody {false};

   http::MultipartBodyUPtr multipartBody {nullptr};

   bool isWebSocketConnect();

private:

   int32_t _id;
   http::ConnectionId _connId { 0 };
   
   std::unique_ptr<http::parser::MultipartParser> _multipartParser {nullptr};
};

class Http2Session;

struct CallbackGuard 
{
   CallbackGuard(Http2Session &h);
   ~CallbackGuard();
   Http2Session &http2Session;
};

using HandlerCallback            = std::function<void()>;
using WriteHandler               = std::function<void(HandlerCallback)>;
using AddToSendQueueHandler      = std::function<void(std::shared_ptr<asio::streambuf>, HandlerCallback)>;
using StreamDataOnCloseHandler   = std::function<void(int32_t)>;
using RequestReadyHandler        = std::function<Result(int32_t) >;
using ValidateMethodHandler      = std::function<bool(const std::string& , http::HttpStatus& )>;


struct Http2Option
{
   size_t sendBufferSize         = http::HTTP_SEND_BUFFER_SIZE_DEFAULT;
   size_t maxHeaderSize          = http::HTTP_HEADER_MAX_SIZE_DEFAULT;
   bool   logVerbose             = true;

   int    maxConcurrentStreams   = 100;
   int    windowBits             = 16;       // 64 KB
   int    connectionWindowBits   = 16;       // 64 KB
   int    headerTableSize        = 4*1024;   // 4 KB
   int    maximumFrameSize       = 16*1024;  // 16 KB
   std::string temporaryDir {};
};
using Http2OptionPtr = std::shared_ptr<Http2Option>;

class Http2Session : public std::enable_shared_from_this<Http2Session>
{
public:
   Http2Session(http::ConnectionId connId
      , asio::streambuf* sendBuffer
      , std::shared_ptr<http2::Http2Option>& option );

   ~Http2Session();
   Result start();
   bool close();
   
   Http2StreamData* createStreamData(int32_t streamId);
   void closeStream(int32_t streamId);
   Http2StreamData* findStream(int32_t streamId);

   Result handleRequest(int32_t streamId);
   Result submitResponse(http::HttpContext httpContext, int streamId);

   bool shouldStop() const;

   void streamDataCloseHandler(StreamDataOnCloseHandler handler)
   {
      _onStreamDataClose = std::move(handler);
   }

   void writeHandler(WriteHandler handler)
   {
      _onWriteHandler = std::move(handler);
   }

   void addToSendQueueHandler(AddToSendQueueHandler handler)
   {
      _addToSendQueueHandler = std::move(handler);
   }

   void requestReadyHandler(RequestReadyHandler handler)
   {
      _onRequestReadyHandler = std::move(handler);
   }

   void validateMethodHandler(ValidateMethodHandler handler)
   {
      _validateMethodHandler = std::move(handler);
   }

   // void validateHeadersHandler(ValidateHeadersHandler handler)
   // {
   //    _validateHeadersHandler = std::move(handler);
   // }

   void addHttpContext(http::HttpContext context, int32_t streamId=-1);

   void writeData(); // signal_write

   void initiateWrite();
   void enterCallback();
   void leaveCallback();

   nghttp2_session* rawSession() { return _session; }
   http::ConnectionId connId() { return _connId; }
   asio::streambuf& sendBuffer() { return *_sendBuffer;}

   Result fillSendBuffer(size_t& byteToTransfer);
   Result readIncomingData(asio::const_buffer buffer, size_t bytesTransferred);

   bool writingData() { return _writingData; }
   void writingData(bool value) {_writingData = value;};
   void terminateSession(uint32_t errorCode);

   const std::shared_ptr<http2::Http2Option>& option() { return _option; }

   // For logging 
   // ---------------------------------------------
   int      _sendDataCounter              = 0;
   size_t   _sendDataCounterBytes         = 0;
   int      _dataSourceReadCbCounter      = 0;
   size_t   _dataSourceReadCbCounterBytes = 0;
   // ---------------------------------------------

   bool validateMethod(const std::string& method, http::HttpStatus& status)
   {
      if (_validateMethodHandler)
         return _validateMethodHandler(method, status);
      
      return false;
   }

   bool hasPendingData();
   int64_t usePendingData();
   void fillPendingData(asio::streambuf* srcBuff, size_t length);

private:
   Result sendServerConnectionHeader();

   std::map<int32_t, std::unique_ptr<Http2StreamData>> _streams;
   std::map<int32_t, http::HttpContext> _httpContexts;

   nghttp2_session*           _session                = nullptr;
   nghttp2_session_callbacks* _callbacks              = nullptr;
   nghttp2_option*            _nghttp2Option          = nullptr;

   StreamDataOnCloseHandler   _onStreamDataClose      = nullptr;
   WriteHandler               _onWriteHandler         = nullptr;
   AddToSendQueueHandler      _addToSendQueueHandler  = nullptr;
   RequestReadyHandler        _onRequestReadyHandler  = nullptr;
   ValidateMethodHandler      _validateMethodHandler  = nullptr;
   // ValidateHeadersHandler     _validateHeadersHandler = nullptr;
   
   http::ConnectionId         _connId                 = 0;
   bool                       _writingData            = false;
   bool                       _closed                 = false;
   bool                       _insideCallback         = false;
   // true if we have pending on_write call.  This avoids repeated call
   // of io_service::post.
   bool                       _writeSignaled          = false;

   asio::streambuf*           _sendBuffer             = nullptr;

   std::vector<uint8_t>       _dataPendingVec;
   const uint8_t*             _dataPending            = nullptr;
   size_t                     _dataPendinglen         = 0;

   std::shared_ptr<Http2Option>& _option;
};
using Http2SessionPtr = std::shared_ptr<Http2Session>;

} // namespace http2
} // namespace tbs

#endif // TOBASA_HTTP_USE_HTTP2