#ifdef TOBASA_HTTP_USE_HTTP2

#include <tobasa/format.h>
#include <tobasa/logger.h>
#include <tobasa/util_string.h>
#include <tobasa/datetime.h>
#include <tobasa/file_reader.h>
#include "tobasahttp/exception.h"
#include "tobasahttp/server/common.h"
#include "tobasahttp/response.h"
#include "tobasahttp/server/http2.h"
#include "tobasahttp/server/http2_lib.h"

namespace {
   constexpr const char LOGTYPE[] = "srv_https";
}

namespace tbs {
namespace http2 {
namespace cb {

int onFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   switch (frame->hd.type) 
   {
      case NGHTTP2_DATA:
      /*
      {
         Logger::logT("[{}] [conn:{}] [http2] Receive (DATA)", LOGTYPE, httpSession->connId());

         // Retrieve the payload length
         // Check that the client request has finished
         if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) 
         {
            auto streamData = httpSession->findStream(frame->hd.stream_id);
            if (!streamData) {
                  return 0; // Stream already closed
            }

            // Process the request when END_STREAM is set
            auto result = httpSession->handleRequest(frame->hd.stream_id);
            if (!result.success())
                  return NGHTTP2_ERR_CALLBACK_FAILURE;
         }
         break;
      }*/
      case NGHTTP2_HEADERS:
      {
         if (frame->hd.type == NGHTTP2_HEADERS)
         {
            if (httpSession->option()->logVerbose)
               Logger::logT("[{}] [conn:{}] [http2] Receive (HEADERS)", LOGTYPE, httpSession->connId());
         }
         else
         {
            if (httpSession->option()->logVerbose)
               Logger::logT("[{}] [conn:{}] [http2] Receive (DATA)", LOGTYPE, httpSession->connId());
         }

         auto streamData = httpSession->findStream(frame->hd.stream_id);
         // For DATA and HEADERS frame, this callback may be called after
         // on_stream_close_callback. Check that stream still alive.
         if (!streamData) {
            return 0; // Stream already closed
         }

         if (frame->headers.cat == NGHTTP2_HCAT_REQUEST)
         {
            // If all headers are received (END_HEADERS), validate method now.
            if (frame->hd.flags & NGHTTP2_FLAG_END_HEADERS)
            {
               http::HttpStatus status;
               if (!httpSession->validateMethod(streamData->requestHeader.method, status))
               {
                  Logger::logW("[{}] [conn:{}] [http2] METHOD not allowed: {}", LOGTYPE, httpSession->connId(), streamData->requestHeader.method);
                  nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, frame->hd.stream_id, NGHTTP2_INTERNAL_ERROR);
                  return 0;
               }
            }

            // WebSocket request
            if (streamData->isWebSocketConnect())
            {
               Logger::logT("[{}] [conn:{}] [http2] Receive WebSocket CONNECT", LOGTYPE, httpSession->connId());
               auto nva = std::vector<nghttp2_nv>();
               nva.push_back(http2::makeNvLs(":status", std::string("200")));

               // Accept the WebSocket connection
               nghttp2_submit_headers(session, NGHTTP2_FLAG_END_HEADERS, frame->hd.stream_id, nullptr, nva.data(), nva.size(), nullptr);
            }
#if 0
            // auto content_length = req.fs.header(http2::HD_CONTENT_LENGTH);
            // if (content_length) {
            //    // libnghttp2 guarantees this can be parsed
            //    req.fs.content_length =
            //       util::parse_uint(content_length->value).value_or(-1);
            // }

            // presence of mandatory header fields are guaranteed by libnghttp2.
            // auto authority = req.fs.header(http2::HD__AUTHORITY);
            // auto path = req.fs.header(http2::HD__PATH);
            // auto method = req.fs.header(http2::HD__METHOD);
            // auto scheme = req.fs.header(http2::HD__SCHEME);

            // auto method_token = http2::lookup_method_token(method->value);
            // if (method_token == -1) {
            //    if (error_reply(downstream, 501) != 0) {
            //       return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            //    }
            //    return 0;
            // }


            // For HTTP/2 proxy, we require :authority.
            // if (method_token != HTTP_CONNECT && config->http2_proxy &&
            //       faddr->alt_mode == UpstreamAltMode::NONE && !authority) {
            //    rst_stream(downstream, NGHTTP2_PROTOCOL_ERROR);
            //    return 0;
            // }

            // req.method = method_token;
            // if (scheme) {
            //    req.scheme = scheme->value;
            // }

            // // nghttp2 library guarantees either :authority or host exist
            // if (!authority) {
            //    req.no_authority = true;
            //    authority = req.fs.header(http2::HD_HOST);
            // }

            // if (authority) {
            //    req.authority = authority->value;
            // }

            // if (path) {
            //    if (method_token == HTTP_OPTIONS && path->value == "*"_sr) {
            //       // Server-wide OPTIONS request.  Path is empty.
            //    } else if (config->http2_proxy &&
            //                faddr->alt_mode == UpstreamAltMode::NONE) {
            //       req.path = path->value;
            //    } else {
            //       req.path = http2::rewrite_clean_path(downstream->get_block_allocator(),
            //                                           path->value);
            //    }
            // }

            // auto connect_proto = req.fs.header(http2::HD__PROTOCOL);
            // if (connect_proto) {
            //    if (connect_proto->value != "websocket"_sr) {
            //       if (error_reply(downstream, 400) != 0) {
            //       return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            //       }
            //       return 0;
            //    }
            //    req.connect_proto = ConnectProto::WEBSOCKET;
            // }

            // if (!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) {
            //    req.http2_expect_body = true;
            // } else if (req.fs.content_length == -1) {
            //    // If END_STREAM flag is set to HEADERS frame, we are sure that
            //    // content-length is 0.
            //    req.fs.content_length = 0;
            // }

            // downstream->inspect_http2_request();

            // downstream->set_request_state(DownstreamState::HEADER_COMPLETE);

            // if (config->http.require_http_scheme &&
            //       !http::check_http_scheme(req.scheme, handler_->get_ssl() != nullptr)) {
            //    if (error_reply(downstream, 400) != 0) {
            //       return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
            //    }
            //    return 0;
            // }
#endif // if 0         
         }


         // Check that the client request has finished
         if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
         {
            // create http request/response object, and process the request
            auto result = httpSession->handleRequest(frame->hd.stream_id);
            if (!result.success())
               return NGHTTP2_ERR_CALLBACK_FAILURE;
         }
         break;
      }
      case NGHTTP2_RST_STREAM:
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Receive (RST_STREAM)", LOGTYPE, httpSession->connId());
         break;
      case NGHTTP2_GOAWAY:
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Receive (GOAWAY)", LOGTYPE, httpSession->connId());
         break;
      default:
         break;
   }
   return 0;
}

int onStreamCloseCallback(nghttp2_session *session, int32_t streamId, uint32_t code, void *userData) 
{
   (void) code;

   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   Http2StreamData* streamData = httpSession->findStream(streamId);
   if (!streamData)
      return 0;

   httpSession->closeStream(streamId);

   return 0;
}

int onHeaderCallback2(nghttp2_session *session, const nghttp2_frame *frame,
                      nghttp2_rcbuf *name, nghttp2_rcbuf *value,
                      uint8_t flags, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   auto namebuf  = nghttp2_rcbuf_get_buf(name);
   auto valuebuf = nghttp2_rcbuf_get_buf(value);

   if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
      return 0;
   }
   //auto streamData = static_cast<Http2StreamData*>(nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));
   auto streamData = httpSession->findStream(frame->hd.stream_id);
   if (streamData == nullptr)
      return 0;

   if (streamData->headerBufferSize + namebuf.len + valuebuf.len > httpSession->option()->maxHeaderSize) 
   {

      Logger::logE("[{}] [conn:{}] [http2] Stream data header buffer size over maximum {} bytes", LOGTYPE, httpSession->connId(), httpSession->option()->maxHeaderSize);
      nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, frame->hd.stream_id, NGHTTP2_INTERNAL_ERROR);
      return 0;
   }

   streamData->headerBufferSize += namebuf.len + valuebuf.len;

   auto token = lookupToken(StringRef{namebuf.base, namebuf.len});

   auto &requestHeader = streamData->requestHeader;

   switch (token) 
   {
      case http2::HD__METHOD:
         requestHeader.method    = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD__SCHEME:
         requestHeader.scheme    = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD__AUTHORITY:
         requestHeader.authority = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD_HOST:
         requestHeader.host      = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD__PATH:
         requestHeader.path      = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD_IF_MODIFIED_SINCE:
         requestHeader.ims       = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD_EXPECT:
         requestHeader.expect    = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      case http2::HD__PROTOCOL:
         requestHeader.protocol  = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);
         break;
      default:
      {
         std::string fieldName  = std::string(reinterpret_cast<const char*>(namebuf.base), namebuf.len);
         std::string fieldValue = std::string(reinterpret_cast<const char*>(valuebuf.base), valuebuf.len);

         if (util::toLower(fieldName) == "content-length")
         {
            requestHeader.contentLength = std::stoull(fieldValue);
            streamData->hasContentLength = true;
         }

         if (util::toLower(fieldName) == "content-type")
         {
            http::MediaType media;
            media.parse(fieldValue);
            if ( media.valid() && util::toLower(media.fullType()) == "multipart/form-data" )
            {
               auto boundary = media.find("boundary");
               if ( boundary && boundary->valid())
               {
                  streamData->initMultipartParser(httpSession->option()->temporaryDir);
                  if (streamData->multipartParser().applyBoundary(boundary->value()))
                  {
                  }
                  else
                  {
                     Logger::logE("[{}] [conn:{}] [http2] Invalid multipart boundary in http header", LOGTYPE, httpSession->connId());
                     return NGHTTP2_ERR_CALLBACK_FAILURE;
                  }
               }
            }
         }
         streamData->requestHeaders.add(fieldName, fieldValue);

         if (streamData->hasMultipartBody)
            streamData->multipartParser().contentLength(requestHeader.contentLength);
      }
      break;
   }

   return 0;
}

int onBeginHeadersCallback(nghttp2_session *session,
                           const nghttp2_frame *frame,
                           void *userData)
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) 
      return 0;

   Http2StreamData* streamData = httpSession->createStreamData(frame->hd.stream_id);

   return 0;
}

int beforeFrameSendCallback(nghttp2_session *session,
                            const nghttp2_frame *frame, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
      return 0;
   }

   return 0;
}

int onInvalidFrameRecvCallback(nghttp2_session *session,
                               const nghttp2_frame *frame,int errorCode, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   return 0;
}

int onFrameSendCallback(nghttp2_session *session, const nghttp2_frame *frame, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   switch (frame->hd.type) 
   {
      case NGHTTP2_HEADERS:
         if (httpSession->findStream(frame->hd.stream_id) != nullptr && httpSession->option()->logVerbose) 
         {
            const nghttp2_nv *nva = frame->headers.nva;

            Logger::logT("[{}] [conn:{}] [http2] ----------------------------", LOGTYPE, httpSession->connId());
            Logger::logT("[{}] [conn:{}] [http2] --      HEADERS SEND      --", LOGTYPE, httpSession->connId());
            for (int i = 0; i < frame->headers.nvlen; ++i) 
            {
               Logger::logT("[{}] [conn:{}] [http2] name -> {} / value -> {}", LOGTYPE, httpSession->connId(), 
                  std::string(reinterpret_cast<const char*>(nva[i].name), nva[i].namelen),
                  std::string(reinterpret_cast<const char*>(nva[i].value), nva[i].valuelen) );
            }
            Logger::logT("[{}] [conn:{}] [http2] ----------------------------", LOGTYPE, httpSession->connId());
         }
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Send (HEADERS)",     LOGTYPE, httpSession->connId());
         break;
      case NGHTTP2_RST_STREAM:
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Send (RST_STREAM)",  LOGTYPE, httpSession->connId());
         break;
      case NGHTTP2_GOAWAY:
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Send (GOAWAY)",      LOGTYPE, httpSession->connId());
         break;
      case NGHTTP2_PUSH_PROMISE:
         if (httpSession->option()->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] Send (PUSH_PROMISE)", LOGTYPE, httpSession->connId());
         break;
   }

   return 0;
}

int onDataChunkRecvCallback(nghttp2_session *session, uint8_t flags,
                            int32_t streamId, const uint8_t *data,
                            size_t len, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession) return NGHTTP2_ERR_CALLBACK_FAILURE;

   auto streamData = httpSession->findStream(streamId);
   if (streamData) 
   {
      if (streamData->hasMultipartBody)
      {
         auto result = streamData->multipartParser().parse(data, len);
         if (streamData->multipartParser().done())
         {
            // Move parser's multipartBody to streamData
            streamData->multipartBody = streamData->multipartParser().multipartBody();
         }
      }
      else
         streamData->requestBody.append(reinterpret_cast<const char*>(data), len);
   }

   return 0;
}

#ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA

namespace {
   constexpr auto PADDING = std::array<uint8_t, 256>{};
}

int sendDataCallback(nghttp2_session *session, nghttp2_frame *frame,
                     const uint8_t *framehd, size_t length,
                     nghttp2_data_source *source, void *userData) 
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   auto* httpContext = static_cast<http::Context*>(source->ptr);
   if (httpContext == nullptr || httpContext->closed()) {
      throw http::Exception("invalid http context pointer", frame->hd.stream_id);
   }

   auto response = httpContext->response();

   size_t padlen = frame->data.padlen;
   std::ostream sendBufferStream(&httpSession->sendBuffer());
   sendBufferStream.write(reinterpret_cast<const char*>(framehd), 9);

   if (padlen > 0) 
   {
      if (httpSession->option()->logVerbose)
         Logger::logT("[{}] [conn:{}] [http2] sendDataCallback, add padding, length: {}", LOGTYPE, httpSession->connId(), padlen);

      sendBufferStream << static_cast<uint8_t>(padlen - 1);
   }

   int64_t readCount=0;
   // If data was prepared by dataSourceReadCallback, use it
   if (httpSession->hasPendingData()) {
      readCount = httpSession->usePendingData();
   }
   else if (length)
   {
      http::ResponseSerializer serializer(response);
      readCount = serializer.serializeHttp2(&httpSession->sendBuffer(), length);
      if (readCount < 0 )
         return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
   }

   if (padlen > 0) 
   {
      if (httpSession->option()->logVerbose)
         Logger::logT("[{}] [conn:{}] [http2]    sendDataCallback, add padding, length: {}", LOGTYPE, httpSession->connId(), padlen);

      std::vector<char> padding(padlen-1);
      sendBufferStream.write(padding.data(), padlen-1);
   }

   if (httpSession->option()->logVerbose)
   {
      httpSession->_sendDataCounterBytes += readCount; 
      ++httpSession->_sendDataCounter;
      if (readCount == 0)
      {
         Logger::logT("[{}] [conn:{}] [http2]    sendDataCallback, called: {}, total bytes: {}", LOGTYPE, httpSession->connId(), httpSession->_sendDataCounter, httpSession->_sendDataCounterBytes);
         httpSession->_sendDataCounter = 0;
         httpSession->_sendDataCounterBytes = 0;
      }
   }

   return 0;
}

nghttp2_ssize dataSourceReadCallback(nghttp2_session* session, int32_t streamId, 
   uint8_t* buf, size_t length, uint32_t* dataFlags, nghttp2_data_source* source, void* userData)
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if (! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   auto* httpContext = static_cast<http::Context*>(source->ptr);
   if (httpContext == nullptr || httpContext->closed()) {
      throw http::Exception("invalid http context pointer", streamId);
   }

   auto response  = httpContext->response();

   // Prepare payload into session->_dataPendingVec by calling serializer into a temporary streambuf
   http::ResponseSerializer serializer(response);
   asio::streambuf tmpBuf;

   // Request a larger buffer from the serializer so gzip deflation produces
   // more output per call (reduces many small produce/write cycles).
   const size_t NO_COPY_MIN_WANT = 8092; // 8KB
   size_t want = std::max(length, NO_COPY_MIN_WANT);
   int64_t written = serializer.serializeHttp2(&tmpBuf, want);
   if (written < 0) {
      return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
   }

   *dataFlags |= NGHTTP2_DATA_FLAG_NO_COPY;

   if (written == 0)
   {
      // determine EOF: no bytes produced and source fully consumed (including gzip finished)
      bool eof = false;
      try {
         auto ds = response->dataSource();
         if (ds->readLeft == 0) 
         {
            if (!response->compressionEnabled())
               eof = true;
            else {
               if (response->getGzipState() == nullptr) eof = true;
            }
         }
      } catch (...) {
         eof = false;
      }

      if (eof) 
      {
         *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
         if (nghttp2_session_get_stream_remote_close(session, streamId) == 0)
            nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, streamId, NGHTTP2_NO_ERROR);
      }
      return 0;
   }

   size_t totalWritten = static_cast<size_t>(written);
   // copy tmpBuf contents into session pending vector
   httpSession->fillPendingData(&tmpBuf, totalWritten);

   (void)httpSession; // silence unused variable in non-verbose builds

   // Clear serializer-owned pending in response's gzip state to avoid duplication
   response->clearGzipPendings();

      // If serializer consumed all source data and gzip state is finished (or not used), signal EOF
   bool eof = false;
   try {
      auto ds = response->dataSource();
      if (ds->readLeft == 0) 
      {
         if (!response->compressionEnabled())
            eof = true;
         else {
            if (response->getGzipState() == nullptr) eof = true;
         }
      }
   } catch (...) {
      eof = false;
   }

   if (eof) {
      *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
   }

   return static_cast<nghttp2_ssize>(totalWritten);
}

#else

nghttp2_ssize dataSourceReadCallback(nghttp2_session* session, int32_t streamId, 
   uint8_t* buf, size_t length, uint32_t* dataFlags, nghttp2_data_source* source, void* userData)
{
   auto httpSession = static_cast<Http2Session*>(userData);
   if ( ! httpSession)
      return NGHTTP2_ERR_CALLBACK_FAILURE;

   auto* httpContext = static_cast<http::Context*>(source->ptr);
   if (httpContext == nullptr || httpContext->closed()) {
      throw http::Exception("invalid http context pointer", streamId);
   }

   auto response = httpContext->response();
   http::ResponseSerializer serializer(response);
   
   auto readCount = serializer.serializeHttp2(buf, length, dataFlags);
   if (readCount < 0 ) {
      return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
   }

   if (httpSession->option()->logVerbose)
   {
      httpSession->_dataSourceReadCbCounterBytes += readCount;
      ++httpSession->_dataSourceReadCbCounter;
      if (serializer.readLeft() == 0)
      {
         Logger::logT("[{}] [conn:{}] [http2] dataSourceReadCallback, called: {}, total bytes: {}", LOGTYPE, httpSession->connId(), httpSession->_dataSourceReadCbCounter, httpSession->_dataSourceReadCbCounterBytes);
         httpSession->_dataSourceReadCbCounter = 0;
         httpSession->_dataSourceReadCbCounterBytes = 0;
      }
   }

   return (nghttp2_ssize) readCount;
}

#endif // TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA

} // namespace cb (callbacks)

Result::~Result() {}

Result::Result(int code, const std::string& message, int32_t streamId)
   : _code {code}
   , _streamId {streamId}
{
   if (_code < 0 ) {
      _success = false;
   }

   if (message.empty())
      _message = std::string(nghttp2_strerror( _code) );
   else 
      _message = message;
}

Result Result::fail(const std::string& message, int32_t streamId)
{
   return Result(-1, message, streamId);
}

Http2StreamData::Http2StreamData(int32_t id, http::ConnectionId connId)
   : _id {id}
   , _connId {connId} 
   , _multipartParser {nullptr} 
{
}

Http2StreamData::~Http2StreamData()
{
   if (_multipartParser != nullptr)
      _multipartParser = nullptr;

}

void Http2StreamData::initMultipartParser(const std::string& temporaryDir)
{
   _multipartParser = std::make_unique<http::parser::MultipartParser>(temporaryDir);
   hasMultipartBody = true;
}

http::parser::MultipartParser& Http2StreamData::multipartParser()
{
   return *_multipartParser.get();
}

bool Http2StreamData::isWebSocketConnect()
{
   return 
      requestHeader.protocol == "websocket" && 
      requestHeader.method == "CONNECT" ;
}

Result Http2Session::handleRequest(int32_t streamId)
{
   auto streamData = findStream(streamId);
   if (!streamData) {
      return Result::fail("Stream not found", streamId);
   }

   if (this->_onRequestReadyHandler)
      return this->_onRequestReadyHandler(streamId);
   
   return {};
}

Result Http2Session::submitResponse(http::HttpContext httpContext, int streamId)
{
   auto response = httpContext->response();

   if (! response->compressionEnabled()) 
   {
      if ( response->contentSize() > 0 )
         response->setHeaderContentLength(response->contentSize());
      else
         response->setHeaderContentLength(0);
   }

   auto nva = std::vector<nghttp2_nv>();
   nva.reserve(1 + response->headers().size());
   auto statusCode = response->httpStatus().codeStr();
   nva.push_back(http2::makeNvLs(":status", statusCode));

   // get http date, no second fraction, use UTC. eg: Wed, 08 Jan 2025 18:25:08 GMT
   DateTime dt;
   auto httpDate = dt.format("{:%a, %d %b %Y %H:%M:%S GMT}",false,true);
   nva.push_back(http2::makeNvLs("date", httpDate));
   
   for ( size_t i = 0; i < response->headers().size(); ++i )
   {
      auto f = response->headers().field(i);
      if ( f != nullptr )
        nva.push_back(http2::makeNv(f->nameRef(), f->valueRef(), false/*sensitive*/));
   }

   nghttp2_data_provider2 dataProvider;
   dataProvider.source.ptr = httpContext.get(); // Pass the raw pointer as user data
   dataProvider.read_callback = cb::dataSourceReadCallback;

   int rv = nghttp2_submit_response2(this->rawSession(), 
               streamId, nva.data(), nva.size(), &dataProvider);

   if (rv < 0)
      return Result(rv, "", streamId);

   this->writeData();
   
   return {};
}

CallbackGuard::CallbackGuard(Http2Session &h) 
   : http2Session(h) 
{
   http2Session.enterCallback();
}

CallbackGuard::~CallbackGuard() 
{ 
   http2Session.leaveCallback(); 
}

Http2Session::Http2Session(http::ConnectionId connId
      , asio::streambuf* sendBuffer , std::shared_ptr<Http2Option>& option)
   : _connId {connId}
   , _sendBuffer {sendBuffer}
   , _option {option}
{
}

Http2Session::~Http2Session()
{
   if (! _closed)
      this->close();
   
   Logger::logT("[{}] [conn:{}] [http2] Http2Session Destroyed", LOGTYPE, this->connId());
}

Result Http2Session::start()
{
   Logger::logT("[{}] [conn:{}] [http2] Initializing nghttp2 session", LOGTYPE, this->connId());

   int rv = nghttp2_session_callbacks_new(&_callbacks);
   if (rv < 0)
      return Result(rv);

   nghttp2_session_callbacks_set_before_frame_send_callback(      _callbacks, cb::beforeFrameSendCallback);
   nghttp2_session_callbacks_set_on_frame_send_callback(          _callbacks, cb::onFrameSendCallback);
   nghttp2_session_callbacks_set_on_frame_recv_callback(          _callbacks, cb::onFrameRecvCallback);
   nghttp2_session_callbacks_set_on_stream_close_callback(        _callbacks, cb::onStreamCloseCallback);
   nghttp2_session_callbacks_set_on_header_callback2(             _callbacks, cb::onHeaderCallback2);
   nghttp2_session_callbacks_set_on_begin_headers_callback(       _callbacks, cb::onBeginHeadersCallback);
   // nghttp2_session_callbacks_set_on_invalid_frame_recv_callback( _callbacks, cb::onInvalidFrameRecvCallback);
   // nghttp2_session_callbacks_set_on_frame_not_send_callback(    _callbacks, cb::onFrameNotSendCallback);

   nghttp2_session_callbacks_set_on_data_chunk_recv_callback(     _callbacks, cb::onDataChunkRecvCallback);
   // nghttp2_session_callbacks_set_data_source_read_length_callback2(_callbacks, cb::dataSourceReadLengthCallback2);

#ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
   nghttp2_session_callbacks_set_send_data_callback(              _callbacks, cb::sendDataCallback);
#endif

   rv = nghttp2_option_new(&_nghttp2Option);
   rv = nghttp2_session_server_new2(&_session, _callbacks, this, _nghttp2Option);
   if (rv < 0)
   {
      std::string errMsg(nghttp2_strerror( (int)rv) );
      return Result(rv, "Could not create server session: " + errMsg);
   }

   auto result =  this->sendServerConnectionHeader();
   if (! result.success())
      return Result(result.code(), "Failed send server connection header: " + result.message());

   return {};
}

bool Http2Session::close()
{
   nghttp2_option_del(_nghttp2Option);
   nghttp2_session_callbacks_del(_callbacks);

   free(_session);
   _closed = true;

   Logger::logT("[{}] [conn:{}] [http2] Http2Session closed", LOGTYPE, this->connId());
   return true;
}

Http2StreamData* Http2Session::createStreamData(int32_t streamId)
{
   auto p = _streams.emplace(streamId, std::make_unique<Http2StreamData>(streamId, this->connId()));
   return (*p.first).second.get();
}

void Http2Session::closeStream(int32_t streamId)
{
   _streams.erase(streamId);
   _httpContexts.erase(streamId);

   if (_onStreamDataClose)
      _onStreamDataClose(streamId);
}

Http2StreamData* Http2Session::findStream(int32_t streamId) 
{
   auto it = _streams.find(streamId);
   if (it == std::end(_streams)) 
   {
      return nullptr;
   }

   return (*it).second.get();
}

void Http2Session::addHttpContext(http::HttpContext context, int32_t streamId)
{
   _httpContexts[context->streamId()] = context;
}

Result Http2Session::sendServerConnectionHeader()
{
   Logger::logT("[{}] [conn:{}] [http2] sendServerConnectionHeader", LOGTYPE, this->connId());
   
   std::array<nghttp2_settings_entry, 5> entry;
   size_t niv = 1;
   
   entry[0].settings_id = NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS;
   entry[0].value = _option->maxConcurrentStreams;

   entry[1].settings_id = NGHTTP2_SETTINGS_HEADER_TABLE_SIZE;
   entry[1].value = _option->headerTableSize;

   entry[2].settings_id = NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE;
   entry[3].value = (1 << _option->windowBits) - 1;

   entry[3].settings_id = NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES;
   entry[3].value = 1;

   entry[4].settings_id = NGHTTP2_SETTINGS_MAX_FRAME_SIZE;
   entry[4].value = _option->maximumFrameSize; // Maximum allowed frame size

   int rv = nghttp2_submit_settings(_session, NGHTTP2_FLAG_NONE, entry.data(), niv);
   if (rv<0)
      return Result(rv);

   // rv = nghttp2_option_set_max_frame_size(_options, 128 * 1024); // Set to 128KB
   rv = nghttp2_session_set_local_window_size( _session, NGHTTP2_FLAG_NONE, 0,
      (1 << _option->connectionWindowBits) - 1);

   return Result(rv);
}

Result Http2Session::readIncomingData(asio::const_buffer buffer, size_t bytesTransferred)
{
   CallbackGuard cg(*this);
   auto readlen = nghttp2_session_mem_recv2(_session, reinterpret_cast<const uint8_t*>(buffer.data()), bytesTransferred);
   return Result(static_cast<int>(readlen));
}

bool Http2Session::shouldStop() const 
{
  return !nghttp2_session_want_read(_session) && !nghttp2_session_want_write(_session);
}


Result Http2Session::fillSendBuffer(size_t& byteToTransfer)
{
   CallbackGuard cg(*this);
   try
   {
      std::ostream sendStream(_sendBuffer);

      byteToTransfer = 0;
      if (_dataPending) 
      {
         sendStream.write(reinterpret_cast<const char*>(_dataPending), _dataPendinglen);

         byteToTransfer += _dataPendinglen;
         _dataPending = nullptr;
         _dataPendinglen = 0;
      }

      for (;;) 
      {
         const uint8_t *data;
         if (_option->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] fillSendBuffer,    START call nghttp2_session_mem_send2", LOGTYPE, connId());
         
         auto readlen = nghttp2_session_mem_send2(_session, &data);
         
         if (_option->logVerbose)
            Logger::logT("[{}] [conn:{}] [http2] fillSendBuffer,    END   call nghttp2_session_mem_send2", LOGTYPE, connId());

         if (readlen < 0) 
            return Result((int)readlen);

         if (readlen == 0) {
            break;
         }

         if (byteToTransfer + readlen > _option->sendBufferSize)
         {
            _dataPending = data;
            _dataPendinglen = readlen;
            break;
         }

         sendStream.write(reinterpret_cast<const char*>(data), readlen);
         byteToTransfer += readlen;
      }
   }
   catch (const http::Exception& ex)
   {
      return Result(-9872, ex.what(), ex.streamId());
   }

   if (_option->logVerbose)
      Logger::logT("[{}] [conn:{}] [http2] fillSendBuffer, END sendBuffer size: {}", LOGTYPE, this->connId(), _sendBuffer->size());
   
   return Result();
}

void Http2Session::writeData()
{
   if (!_insideCallback && !_writeSignaled)
   {
      _writeSignaled = true;

      auto self = this->shared_from_this();
      this->_onWriteHandler(
         [self](){
            self->initiateWrite();
         }
      );
   }
}

void Http2Session::enterCallback() { _insideCallback = true;}

void Http2Session::leaveCallback() { _insideCallback = false; }

void Http2Session::initiateWrite() { _writeSignaled = false; }

void Http2Session::terminateSession(uint32_t errorCode)
{
   nghttp2_session_terminate_session(_session, errorCode);
}

bool Http2Session::hasPendingData()
{
   return (_dataPendinglen > 0 && _dataPending != nullptr);
}

int64_t Http2Session::usePendingData()
{
   std::ostream outStream( &sendBuffer() );
   outStream.write(reinterpret_cast<const char*>(_dataPending), static_cast<std::streamsize>(_dataPendinglen));
   auto readCount = static_cast<int64_t>(_dataPendinglen);

   // clear pending data
   _dataPendingVec.clear();
   _dataPending = nullptr;
   _dataPendinglen = 0;

   return readCount;
}

void Http2Session::fillPendingData(asio::streambuf* srcBuff, size_t length)
{
   _dataPendingVec.resize(length);
   std::istream is(srcBuff);
   is.read(reinterpret_cast<char*>(_dataPendingVec.data()), static_cast<std::streamsize>(length));
   _dataPending = _dataPendingVec.data();
   _dataPendinglen = length;
}

} // namespace http2
} // namespace tbs

#endif //TOBASA_HTTP_USE_HTTP2