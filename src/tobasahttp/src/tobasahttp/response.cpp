#ifdef TOBASA_HTTP_USE_HTTP2
   #include <nghttp2/nghttp2.h>
#endif

#include <tobasa/datetime.h>
#include <tobasa/util.h>
#include <tobasa/util_string.h>
#include <tobasa/string_reader.h>
#include <tobasa/file_reader.h>
#include <tobasa/bytes_reader.h>
#include <tobasahttp/util.h>
#include "tobasahttp/response.h"
#include "tobasahttp/exception.h"
#include <zlib.h>
#include <memory>
#include <algorithm>
#include <cstring>
#include <tobasa/logger.h>

// Note: HTTP/2 write variant selection is controlled by build flags

namespace {
   const size_t GZBUFFSIZE = 8092;
   struct GzipState 
   {
      z_stream strm;
      std::vector<uint8_t> outBuf;
      std::vector<uint8_t> pending; // holds compressed bytes not yet sent
      size_t pendingPos;
      bool finished;

      GzipState()
         : outBuf(GZBUFFSIZE), finished(false) 
      { 
         pendingPos = 0;
         std::memset(&strm, 0, sizeof(strm)); 
      }
   };
}

namespace tbs {
namespace http {

Response::Response(HttpVersion httpVersion)
   : Message()
   , _httpVersion {httpVersion}
   , _dataSource  {std::make_shared<tbs::http::ResponseDataSource>(ResponseDataType::string)}
{
}

Response::~Response()
{
   destroyGzipState();
}

uint16_t Response::statusCode() const
{
   return static_cast<uint16_t>(_httpStatus.code());
}

std::string Response::statusMessage() const
{
   return _httpStatus.reason();
}

void Response::httpStatus(HttpStatus status)
{
   _httpStatus = status;
}

void Response::httpStatus(StatusCode code)
{
   _httpStatus = HttpStatus(code);
}

HttpStatus Response::httpStatus()
{
   return _httpStatus;
}

void Response::redirect(const std::string& location)
{
   _redirected = true;
   setHeader("Location", location);
   addHeader("Content-Length", "0" );

   // we have to call content(), to init datasource's datareader
   this->content("");
}

size_t Response::contentSize() const
{
   return _dataSource->dataSize;
}

void Response::content(const std::string& content)
{
   _content = std::move(content);
   
   _dataSource->dataType   = http::ResponseDataType::string;
   _dataSource->dataReader = std::make_unique<tbs::StringReader>(_content);
   
   auto dataSize           = _dataSource->dataReader->dataSize();
   _dataSource->dataSize   = dataSize;
   _dataSource->readLeft   = dataSize;
}

void Response::fileContent(const std::string& fullPath)
{
   _content = "";

   _dataSource->dataType   = http::ResponseDataType::file;
   _dataSource->dataReader = std::make_unique<tbs::FileReader>(fullPath);
   _dataSource->filePath   = fullPath;

   auto dataSize           = _dataSource->dataReader->dataSize();
   _dataSource->dataSize   = dataSize;
   _dataSource->readLeft   = dataSize;
}

void Response::rawBytesContent(const nonstd::span<const unsigned char>& rawBytes)
{
   _content = "";

   _dataSource->dataType   = http::ResponseDataType::rawBytes;
   _dataSource->dataReader = std::make_unique<tbs::BytesReader>(rawBytes);

   auto dataSize           = _dataSource->dataReader->dataSize();
   _dataSource->dataSize   = dataSize;
   _dataSource->readLeft   = dataSize;
}

void Response::addCookieHeader(std::shared_ptr<ResponseCookie> cookie) 
{
   std::string val = cookie->toString();
   addHeader("Set-Cookie", val);
}

void Response::addCookieHeader(const ResponseCookie& cookie)
{
   std::string val = cookie.toString();
   addHeader("Set-Cookie", val);
}

void Response::initGzipState(size_t dataSize)
{
   // zlib's default compression level is 6
   int level = (dataSize > 100 * 1024) ? 6 : 5;
   //int level = Z_DEFAULT_COMPRESSION;

   auto gs = new GzipState();
   if (deflateInit2(&gs->strm, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) 
   {
      _gzipStreamingActive = false;
      delete gs;
      throw http::Exception("gzip deflateInit2 failed");
   }
   _gzipState = gs;
   _gzipStreamingActive = true;
}

void* Response::getGzipState()
{
   if (_gzipState == nullptr)
      return nullptr;
   
   return static_cast<void*>(_gzipState);
}

void Response::clearGzipPendings()
{
   // Remove only the bytes already consumed by the serializer (prefix)
   // to avoid dropping any remaining compressed bytes that were not
   // yet written into the session-owned pending buffer.
   if (_gzipState)
   {
      auto gs = static_cast<GzipState*>(_gzipState);
      if (!gs) {
         throw http::Exception("Invalid compression state");
      }

      // If serializer advanced pendingPos, erase that consumed prefix.
      if (gs->pendingPos > 0)
      {
         if (gs->pendingPos >= gs->pending.size())
         {
            gs->pending.clear();
         }
         else
         {
            gs->pending.erase(gs->pending.begin(), gs->pending.begin() + gs->pendingPos);
         }
         gs->pendingPos = 0;
      }
      // if pendingPos == 0, nothing was consumed by serializer; leave pending as-is
   }
}

void Response::destroyGzipState()
{
   if (_gzipState)
   {
      delete _gzipState;
      _gzipState = nullptr;
      _gzipStreamingActive = false;
   }
}

bool Response::compressionActive() const
{
   return _gzipStreamingActive;
}

void Response::prepareForCompression(CompressionRule rule)
{
   if (! rule.useCompression )
   {
      _compressionEnabled = false;
      _preparedForCompression = true;
      return;
   }


   if (_dataSource->dataSize >= rule.minimalBodySize && 
      http::isCompressible(contentType(), rule.mimetypes) && 
      util::contains(rule.acceptEncoding, "gzip") &&
      util::contains(rule.encoding, "gzip") )
   {
      _compressionEnabled = true;
      setHeader("Content-Encoding", "gzip");
   }
   else {
      _compressionEnabled = false;
   }

   _preparedForCompression = true;
}

// -------------------------------------------------------

int64_t computeExpectedTotal(
   bool headRequest, int64_t headerSize, int64_t bodySize, size_t chunkSize=0 ) 
{
   if (headRequest)
   {
      // HEAD request, no body
      return headerSize;
   }

   if (chunkSize == 0) 
   {
      // fixed-length response
      return headerSize + bodySize;
   }

   // chunked response
   if (bodySize == 0) 
   {
      // no body, just headers + final zero chunk
      return headerSize + 5; // 0\r\n\r\n
   }

   int64_t total = headerSize;

   // calculate number of full chunks
   int64_t fullChunks = bodySize / chunkSize;
   int64_t remainder  = bodySize % chunkSize;

   auto chunkHeaderSize = [](int64_t len) -> int 
   {
      // hex length + \r\n
      std::ostringstream oss;
      oss << std::hex << len << "\r\n";
      return static_cast<int>(oss.str().size());
   };

   // full chunks
   total += fullChunks * (chunkSize + 2 + chunkHeaderSize(chunkSize)); // data + \r\n + hex header
   // remainder chunk
   if (remainder > 0) {
      total += remainder + 2 + chunkHeaderSize(remainder);
   }

   // final zero-length chunk
   total += 5; // 0\r\n\r\n

   return total;
}


ResponseDataSource::ResponseDataSource(ResponseDataType datType)
   : dataType {datType}
{
}

ResponseDataSource::~ResponseDataSource() 
{
   if (_writerCb)
      _writerCb = nullptr;

#ifdef TOBASA_HTTP_USE_HTTP2
   if (_writerCb2)
      _writerCb2 = nullptr;
#endif
}

void ResponseDataSource::writerCallback(ResponseWriterCb cb) 
{
   _writerCb = std::move(cb);
}

#ifdef TOBASA_HTTP_USE_HTTP2
void ResponseDataSource::writerCallback2(ResponseWriterCb2 cb) 
{
   _writerCb2 = std::move(cb);
}
#endif

void ResponseDataSource::connect(std::shared_ptr<Response> response)
{
   response->_dataSource = nullptr;
   response->_dataSource = this->shared_from_this();

   auto dataSize  = dataReader->dataSize();
   this->dataSize = dataSize;
   this->readLeft = dataSize;
}

// -------------------------------------------------------

ResponseSerializer::ResponseSerializer(std::shared_ptr<Response> response)
   : _response {response}
{}

size_t ResponseSerializer::readLeft()
{
   return _response->_dataSource->readLeft;
}

// -------------------------------------------------------
int64_t ResponseSerializer::serializeHttp1(asio::streambuf* sendBuffer, size_t bufferSize) 
{
   auto& writeCallback = _response->_dataSource->_writerCb;
   if ( writeCallback )
      return writeCallback(sendBuffer);
   else
      return doSerializeHttp1(sendBuffer, bufferSize);

   return 0;
}

// -------------------------------------------------------

#ifdef TOBASA_HTTP_USE_HTTP2
   #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
   int64_t ResponseSerializer::serializeHttp2(asio::streambuf* sendBuffer, size_t length) 
   #else
   int64_t ResponseSerializer::serializeHttp2(uint8_t* buffer, size_t length, uint32_t* dataFlags) 
   #endif
   {
      auto& writeCallback2 = _response->_dataSource->_writerCb2;

      if ( writeCallback2) {
   #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
         return writeCallback2(sendBuffer, length);
   #else
         return writeCallback2(buffer, length, dataFlags);
   #endif
      }
      else
      {
   #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
         return doSerializeHttp2(sendBuffer, length);
   #else
         return doSerializeHttp2(buffer, length, dataFlags);
   #endif
      }
      return 0;
   }
#endif // TOBASA_HTTP_USE_HTTP2


int64_t ResponseSerializer::doSerializeHttp1(asio::streambuf* sendBuffer, size_t bufferSize) 
{
   // For HTTP/1 or HTTP/1.1
   // https://datatracker.ietf.org/doc/html/rfc2616#section-14.13

   auto dataSource = _response->_dataSource;
   auto dataSize   = dataSource->dataSize;
   // effective body size may change if compression is enabled
   size_t effectiveBodySize = dataSize;
   std::ostream outStream(sendBuffer);
   size_t headerSize = 0;

   if (!_response->_headersDone)
   {
      if (! (_response->_redirected || _response->_httpStatus.code() == StatusCode::SWITCHING_PROTOCOLS) )
      {
         // If gzip requested, prepare streaming gzip state and force chunked
         if (_response->compressionEnabled()) 
         {
            // initialize gzip state if not present
            if (_response->getGzipState() == nullptr)
               _response->initGzipState(dataSize);

            //if (_response->getGzipState())
            //   _response->setHeader("Content-Encoding", "gzip");

            // force chunked encoding when compressing on-the-fly
            if (! _response->useChunkedEncoding())
               _response->useChunkedEncoding(true);
         }

         if (_response->useChunkedEncoding()) 
            _response->setHeader("Transfer-Encoding", "chunked");
         else 
         {
            // If gzip streaming is enabled we do not set Content-Length (use chunked).
            if (! _response->compressionEnabled()) 
            {
               if ( dataSize > 0 )
                  _response->setHeaderContentLength(dataSize);
               else
                  _response->setHeaderContentLength(0);
            }
         }
      }

      // if gzip enabled, set Content-Encoding header
      if (_response->compressionEnabled()) 
      {
         // when streaming gzip we don't know compressed length ahead of time
         effectiveBodySize = 0;
      }

      // get http date, no second fraction, use UTC. eg: Wed, 08 Jan 2025 18:25:08 GMT
      DateTime dt;
      auto httpDate = dt.format("{:%a, %d %b %Y %H:%M:%S GMT}",false,true);
      _response->setHeader("Date", httpDate);

      std::ostringstream hdrStream;
      // first, build a status line ( HTTP/1.0 200 OK )
      hdrStream << "HTTP/" << _response->_majorVersion << "." << _response->_minorVersion
                << " "
                << static_cast<uint16_t>(_response->_httpStatus.code()) << " "
                << _response->_httpStatus.reason()
                << "\r\n";

      // add headers
      for ( size_t i = 0; i < _response->_headers.size(); ++i )
      {
         auto f = _response->_headers.field(i);
         if ( f != nullptr )
            hdrStream <<  f->name() << ": " << f->value() << "\r\n";
      }

      hdrStream << "\r\n";
      std::string headerStr = std::move(hdrStream).str();  // single allocation
      headerSize = headerStr.size();
      outStream.write( headerStr.data(), headerSize );

      _response->_headersDone = true;

      if (_response->useChunkedEncoding() && ! _response->compressionEnabled()) 
         _response->_expectedTotalTransferred = computeExpectedTotal(_response->isHeadRequest(), headerSize, static_cast<int64_t>(effectiveBodySize), bufferSize);
      else
         _response->_expectedTotalTransferred = computeExpectedTotal(_response->isHeadRequest(), headerSize, static_cast<int64_t>(effectiveBodySize), 0);
   }

   // HEAD request, no body
   if (_response->isHeadRequest()) 
   {
      dataSource->readLeft = 0;
      return headerSize;
   }

   if (dataSize <= 0)
      return 0;

   if ( dataSource->dataReader == nullptr )
      throw http::Exception("invalid response data source reader pointer");

   if ( dataSource->dataType == ResponseDataType::file && !dataSource->dataReader->isOpen() ) 
      throw http::Exception(std::string("error opening data source " +  dataSource->filePath ));

   // =========================================================================
   // Now process the Body
   // =========================================================================

   // Chunked Encoding with Compression
   if (_response->compressionEnabled())
   {
      auto gs = static_cast<GzipState*>(_response->getGzipState());
      if (!gs) {
         throw http::Exception("Invalid compression state");
      }

      // read raw bytes from source
      auto readCount = static_cast<size_t>(std::min(bufferSize, dataSource->readLeft));
      std::vector<uint8_t> inBuf(readCount);
      std::streamsize bytesRead = 0;
      if (readCount > 0) 
      {
         auto startAt = dataSource->dataSize - dataSource->readLeft;
         bytesRead = dataSource->dataReader->readAt(startAt, inBuf.data(), static_cast<std::streamsize>(readCount));
         if (bytesRead < 0) 
            throw http::Exception("Invalid Response data source state");
         
         dataSource->readLeft -= static_cast<size_t>(bytesRead);
      }

      // feed input to deflate
      // use actual bytesRead (not requested readCount) to decide whether next_in
      // should point to real input or Z_NULL when avail_in == 0
      gs->strm.next_in = (bytesRead > 0) ? inBuf.data() : Z_NULL;
      gs->strm.avail_in = static_cast<uInt>(bytesRead);

      std::vector<uint8_t> comp;
      if (!gs->finished)
      {
         for (;;) 
         {
            gs->outBuf.resize(GZBUFFSIZE);
            gs->strm.next_out = gs->outBuf.data();
            gs->strm.avail_out = static_cast<uInt>(gs->outBuf.size());
            int ret = deflate(&gs->strm, (dataSource->readLeft == 0 && bytesRead == 0) ? Z_FINISH : Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR)
               throw http::Exception("gzip deflate failed");

            size_t have = gs->outBuf.size() - gs->strm.avail_out;
            if (have > 0) 
               comp.insert(comp.end(), gs->outBuf.data(), gs->outBuf.data() + have);

            if (ret == Z_STREAM_END) 
            {
               gs->finished = true;
               break;
            }

            if (gs->strm.avail_out != 0) 
               break; // no more output produced this iteration
         }
      }

      if (!comp.empty()) 
      {
         std::ostringstream chunkHdr;
         chunkHdr << std::hex << comp.size() << "\r\n";
         outStream.write(chunkHdr.str().data(), chunkHdr.str().size());
         outStream.write(reinterpret_cast<const char*>(comp.data()), static_cast<std::streamsize>(comp.size()));
         outStream.write("\r\n", 2);
         return headerSize + static_cast<int64_t>(chunkHdr.str().size() + comp.size() + 2);
      }

      if (gs->finished) 
      {
         const char* lastChunk = "0\r\n\r\n";
         outStream.write(lastChunk, 5);
         deflateEnd(&gs->strm);
         _response->destroyGzipState();
         return headerSize + 5;
      }

      return 0;
   }

   // Chunked Endcoding without compression
   else if (_response->useChunkedEncoding())
   {
      auto readCount = std::min(bufferSize, dataSource->readLeft);
      std::vector<uint8_t> tmpBuffer(readCount);
      std::streamsize bytesRead = 0;

      if (readCount > 0) 
      {
         // First copy the block into tmpBuffer
         auto startAt = dataSource->dataSize - dataSource->readLeft;
         bytesRead = dataSource->dataReader->readAt(startAt, tmpBuffer.data(), static_cast<std::streamsize>(readCount) );

         if (bytesRead < 0)
            throw http::Exception("Invalid Response data source state");

         dataSource->readLeft -= bytesRead;
      }

      if (bytesRead > 0) 
      {
         // write chunk: "<hex>\r\n<data>\r\n"
         std::ostringstream chunkHdr;
         chunkHdr << std::hex << bytesRead << "\r\n";

         outStream.write(chunkHdr.str().data(), chunkHdr.str().size());
         outStream.write(reinterpret_cast<const char*>(tmpBuffer.data()), bytesRead);
         outStream.write("\r\n", 2);

         auto bytesWritten = chunkHdr.str().size() + bytesRead + 2;
         return headerSize + bytesWritten;
      } 
      else 
      {
         // final zero-length chunk
         const char* lastChunk = "0\r\n\r\n";
         outStream.write(lastChunk, 5);

         return headerSize + 5;
      }
   }
   
   // Default behavior
   else
   {
      if (dataSource->dataType == ResponseDataType::string)
      {
         outStream << _response->_content;
         dataSource->readLeft = 0;
         return headerSize + static_cast<int64_t>(dataSize);
      }
      else if (dataSource->dataType == ResponseDataType::file || dataSource->dataType == ResponseDataType::rawBytes)
      {
         if ( dataSource->dataReader == nullptr )
            throw http::Exception("invalid response data source reader pointer");

         if ( dataSource->dataType == ResponseDataType::file && !dataSource->dataReader->isOpen() ) 
            throw http::Exception(std::string("error opening data source " +  dataSource->filePath ));

         auto readCount = std::min(bufferSize, dataSource->readLeft);
         auto startAt   = dataSource->dataSize - dataSource->readLeft;
         // we write/append data from dataSource directly into outStream
         auto bytesRead = dataSource->dataReader->readAt(startAt, outStream, static_cast<std::streamsize>(readCount));
         if (bytesRead < 0)
            throw http::Exception("Invalid Response data source state");

         dataSource->readLeft -= readCount;
         return headerSize + static_cast<int64_t>(readCount);
      }
   }

   return 0;
}

#ifdef TOBASA_HTTP_USE_HTTP2
  #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
   int64_t ResponseSerializer::doSerializeHttp2(asio::streambuf* sendBuffer, size_t bufferSize)
  #else
   int64_t ResponseSerializer::doSerializeHttp2(uint8_t* buffer, size_t bufferSize, uint32_t* dataFlags)
  #endif
   {
      auto dataSource = _response->_dataSource;
      
      if (dataSource->dataReader == nullptr)
         throw http::Exception("invalid response data source reader pointer");

      if (! dataSource->dataReader->isOpen()) 
      {
         std::string msg("error opening data source " +  dataSource->filePath );
         throw http::Exception(msg);
      }

      auto readCount = std::min(bufferSize, dataSource->readLeft);
      auto startAt   = dataSource->dataSize - dataSource->readLeft;

    #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
      // NO-COPY variant: write compressed/plain data into provided streambuf
      if (_response->compressionEnabled())
      {
         // ensure gzip state exists
         if (_response->getGzipState() == nullptr)
            _response->initGzipState(dataSource->dataSize);

         auto gs = static_cast<GzipState*>(_response->getGzipState());
         if (!gs) 
            throw http::Exception("Invalid compression state");

         // read raw bytes
         std::vector<uint8_t> inBuf(readCount);
         std::streamsize bytesRead = 0;
         if (readCount > 0) 
         {
            bytesRead = dataSource->dataReader->readAt(startAt, inBuf.data(), static_cast<std::streamsize>(readCount));
            if (bytesRead < 0) 
               throw http::Exception("Invalid Response data source state");

            dataSource->readLeft -= static_cast<size_t>(bytesRead);
         }

         gs->strm.next_in = (bytesRead>0) ? inBuf.data() : Z_NULL;
         gs->strm.avail_in = static_cast<uInt>(bytesRead);

         std::vector<uint8_t> comp;
         if (!gs->finished)
         {
            for (;;) 
            {
               gs->outBuf.resize(GZBUFFSIZE);
               gs->strm.next_out = gs->outBuf.data();
               gs->strm.avail_out = static_cast<uInt>(gs->outBuf.size());
               int ret = deflate(&gs->strm, (dataSource->readLeft == 0 && bytesRead == 0) ? Z_FINISH : Z_NO_FLUSH);
               if (ret == Z_STREAM_ERROR) 
                  throw http::Exception("gzip deflate failed");

               size_t have = gs->outBuf.size() - gs->strm.avail_out;
               if (have > 0) 
                  comp.insert(comp.end(), gs->outBuf.data(), gs->outBuf.data() + have);

               if (ret == Z_STREAM_END) 
               { 
                  gs->finished = true; 
                  break; 
               }

               if (gs->strm.avail_out != 0) 
                  break; // no more output this iteration
            }
         }

         if (!comp.empty()) 
         {
            // append newly produced compressed bytes to pending buffer
            gs->pending.insert(gs->pending.end(), comp.begin(), comp.end());
         }

         // write up to bufferSize bytes from pending into sendBuffer
         size_t avail = gs->pending.size() - gs->pendingPos;
         if (avail > 0)
         {
            size_t toWrite = std::min(avail, bufferSize);
            std::ostream outStream(sendBuffer);
            outStream.write(reinterpret_cast<const char*>(gs->pending.data() + gs->pendingPos), static_cast<std::streamsize>(toWrite));
            gs->pendingPos += toWrite;

            if (gs->pendingPos == gs->pending.size()) 
            {
               gs->pending.clear();
               gs->pendingPos = 0;
            }

            // if finished and no more pending and no more input, cleanup
            if (gs->finished && gs->pending.empty() && dataSource->readLeft == 0) 
            {
               deflateEnd(&gs->strm);
               _response->destroyGzipState();
            }

            return static_cast<int64_t>(toWrite);
         }

         // nothing to send now
         if (gs->finished && gs->pending.empty()) 
         {
            // ensure EOF is transmitted as empty data block handled by caller
            deflateEnd(&gs->strm);
            _response->destroyGzipState();
            return 0;
         }

         return 0;
      }
      else
      {
         // plain data: read directly into sendBuffer
         std::vector<uint8_t> tmpBuffer(readCount);
         auto bytesRead = dataSource->dataReader->readAt(startAt, tmpBuffer.data(), static_cast<std::streamsize>(readCount));
         if (bytesRead < 0) 
            throw http::Exception("Invalid Response data source state");

         dataSource->readLeft -= static_cast<size_t>(bytesRead);

         std::ostream outStream(sendBuffer);
         if (bytesRead > 0) 
            outStream.write(reinterpret_cast<const char*>(tmpBuffer.data()), static_cast<std::streamsize>(bytesRead));

         return static_cast<int64_t>(bytesRead);
      }
    #else
      // COPY variant: write into provided buffer pointer
      if (_response->compressionEnabled())
      {
         if (_response->getGzipState() == nullptr)
            _response->initGzipState(dataSource->dataSize);

         auto gs = static_cast<GzipState*>(_response->getGzipState());
         if (!gs) 
            throw http::Exception("Invalid compression state");

         std::vector<uint8_t> inBuf(readCount);
         std::streamsize bytesRead = 0;
         if (readCount > 0) 
         {
            bytesRead = dataSource->dataReader->readAt(startAt, inBuf.data(), static_cast<std::streamsize>(readCount));
            if (bytesRead < 0) 
               throw http::Exception("Invalid Response data source state");

            dataSource->readLeft -= static_cast<size_t>(bytesRead);
         }

         gs->strm.next_in = (bytesRead>0) ? inBuf.data() : Z_NULL;
         gs->strm.avail_in = static_cast<uInt>(bytesRead);

         std::vector<uint8_t> comp;
         if (!gs->finished)
         {
            for (;;) 
            {
               gs->outBuf.resize(GZBUFFSIZE);
               gs->strm.next_out = gs->outBuf.data();
               gs->strm.avail_out = static_cast<uInt>(gs->outBuf.size());
               int ret = deflate(&gs->strm, (dataSource->readLeft == 0 && bytesRead == 0) ? Z_FINISH : Z_NO_FLUSH);
               if (ret == Z_STREAM_ERROR) 
                  throw http::Exception("gzip deflate failed");

               size_t have = gs->outBuf.size() - gs->strm.avail_out;
               if (have > 0) 
                  comp.insert(comp.end(), gs->outBuf.data(), gs->outBuf.data() + have);

               if (ret == Z_STREAM_END) 
               { 
                  gs->finished = true; 
                  break; 
               }

               if (gs->strm.avail_out != 0) 
                  break;
            }
         }

         if (!comp.empty()) 
         {
            // append newly produced compressed bytes to pending buffer
            gs->pending.insert(gs->pending.end(), comp.begin(), comp.end());
         }

         // if there is pending data, copy up to bufferSize
         size_t avail = gs->pending.size() - gs->pendingPos;
         if (avail > 0)
         {
            size_t toCopy = std::min(avail, bufferSize);
            std::memcpy(buffer, gs->pending.data() + gs->pendingPos, toCopy);
            gs->pendingPos += toCopy;

            // if we've consumed all pending, reset to reclaim memory
            if (gs->pendingPos == gs->pending.size()) 
            {
               gs->pending.clear();
               gs->pendingPos = 0;
            }

            // if finished and no more pending and no more input, mark EOF and cleanup
            if (gs->finished && gs->pending.empty() && dataSource->readLeft == 0) 
            {
               *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
               deflateEnd(&gs->strm);
               _response->destroyGzipState();
            }

            return static_cast<int64_t>(toCopy);
         }

         // nothing to send now
         // if finished and no pending and no more input, signal EOF
         if (gs->finished && gs->pending.empty())
         {
            *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
            deflateEnd(&gs->strm);
            _response->destroyGzipState();
            return 0;
         }

         return 0;
      }
      else
      {
         std::vector<uint8_t> tmpBuffer(readCount);
         auto bytesRead = dataSource->dataReader->readAt(startAt, tmpBuffer.data(), static_cast<std::streamsize>(readCount));
         if (bytesRead < 0) 
            throw http::Exception("Invalid Response data source state");

         dataSource->readLeft -= static_cast<size_t>(bytesRead);

         if (bytesRead > 0) 
            std::memcpy(buffer, tmpBuffer.data(), static_cast<size_t>(bytesRead));

         if (dataSource->readLeft == 0) 
            *dataFlags |= NGHTTP2_DATA_FLAG_EOF;
         
         return static_cast<int64_t>(bytesRead);
      }
    #endif
   }
#endif // TOBASA_HTTP_USE_HTTP2

} // namespace http
} // namespace tbs