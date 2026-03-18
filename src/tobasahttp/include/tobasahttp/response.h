#pragma once

#include <sstream>
#include <vector>
#include <functional>
#include <asio/streambuf.hpp>
#include <tobasa/data_reader.h>
#include <tobasa/nonstd_span.hpp>
#include "tobasahttp/type_common.h"
#include "tobasahttp/message.h"
#include "tobasahttp/status_codes.h"
#include "tobasahttp/cookie_response.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

using ResponseWriterCb = std::function<int64_t(asio::streambuf* sendBuffer)>;

#ifdef TOBASA_HTTP_USE_HTTP2
   #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
      using ResponseWriterCb2 = std::function<int64_t(asio::streambuf* sendBuffer, size_t length)>;
   #else
      using ResponseWriterCb2 = std::function<int64_t(uint8_t* buffer, size_t length, uint32_t* dataFlags)>;
   #endif
#endif

enum class ResponseDataType : uint8_t
{
   file,
   string,
   rawBytes
};


struct CompressionRule
{
   bool        useCompression    {true};  // IS Compression enabled in HTTP server
   int         minimalBodySize   {1024};  // Minimal body size allowed for compression
   std::string encoding          {};      // Valid compression type: gzip, br (only gzip for now)
   std::string acceptEncoding    {};      // HTTP Request Header Accept-Encoding
   std::vector<std::string> mimetypes;    // Compressible mime types
};

class Response;

/**
 * @brief Data source for an HTTP response.
 * 
 * The ResponseDataSource class encapsulates the data source for an HTTP response,
 * including the data type, data size, and the data reader. It provides methods
 * to connect the data source to a response object and to set the writer callback
 * for the response serialization.
 */
class ResponseDataSource 
   : public std::enable_shared_from_this<ResponseDataSource> 
{
private:
   friend class ResponseSerializer;
   ResponseWriterCb  _writerCb   {nullptr};
#ifdef TOBASA_HTTP_USE_HTTP2   
   ResponseWriterCb2 _writerCb2  {nullptr};
#endif   

public:
   ResponseDataSource(ResponseDataType datType);
   ~ResponseDataSource() ;
   
   std::unique_ptr<DataReader> dataReader {nullptr};
   ResponseDataType  dataType;
   size_t            dataSize {0};
   size_t            readLeft {0};
   size_t            totalTransferred {0};
   std::string       filePath {}; // only for error message

   void connect(std::shared_ptr<Response> response);

   void writerCallback(ResponseWriterCb cb);
#ifdef TOBASA_HTTP_USE_HTTP2
   void writerCallback2(ResponseWriterCb2 cb);
#endif
};



/**
 * @brief HTTP response message.
 * 
 * The Response class encapsulates the details of an HTTP response, including
 * the status code, HTTP version, response content, and any redirection information.
 * It provides methods to set and retrieve these details, as well as to manage
 * the response content from either a string or a file.
 * 
 * This class inherits from the Message class and is used in conjunction with
 * ResponseSerializer and ResponseDataSource to handle the serialization and
 * data sourcing of the response.
 */
class Response : public Message
{
private:
   friend class ResponseSerializer;
   friend class ResponseDataSource;

   std::shared_ptr<ResponseDataSource> _dataSource {nullptr};
   HttpStatus _httpStatus              {StatusCode::OK};
   HttpVersion _httpVersion            {HttpVersion::one};

   bool     _redirected                { false };
   size_t   _totalTransferred          { 0 };
   bool     _headersDone               { false };
   size_t   _expectedTotalTransferred  { 0 };
   bool     _useChunkedEncoding        { false };
   bool     _isHeadRequest             { false };
   bool     _compressionEnabled        { false };

   // gzip compression buffer/state used when Response requests gzip encoding
   void*    _gzipState                 {nullptr};
   bool     _gzipStreamingActive       {false};

   bool     _preparedForCompression    {false};
public:
   Response(HttpVersion httpVersion);
   ~Response();
   
   uint16_t statusCode() const;
   std::string statusMessage() const;
   void httpStatus(HttpStatus status);
   void httpStatus(StatusCode code);
   HttpStatus httpStatus();
   void redirect(const std::string& location);
   size_t contentSize() const;

   using Message::content;
   
   /// Set response content from string
   /// We must set the content before sending the response
   /// Use this method, fileContent() or rawBytesContent() to set the content
   /// \param content The content to set
   void content(const std::string& content) override;

   /// Set response content from a file
   /// We must set the content before sending the response
   /// Use this method, content() or rawBytesContent() to set the content
   /// \param fullPath The full path to the file
   void fileContent(const std::string& fullPath);

   /// Set response content from raw bytes data
   /// We must set the content before sending the response
   /// Use this method, content() or fileContent() to set the content
   /// \param fullPath The full path to the file
   void rawBytesContent(const nonstd::span<const unsigned char>& rawBytes);

   void addCookieHeader(std::shared_ptr<ResponseCookie> cookie);
   void addCookieHeader(const ResponseCookie& cookie);

   std::shared_ptr<ResponseDataSource> dataSource() { return _dataSource;}
   
   HttpVersion httpVersion() const          { return _httpVersion; }
   size_t totalTransferred() const          { return _totalTransferred; }
   size_t expectedTotalTransferred() const  { return _expectedTotalTransferred; }
   void updateTotalTransferred(size_t val)  { _totalTransferred += val; } 
   bool useChunkedEncoding() const          { return _useChunkedEncoding; }
   void useChunkedEncoding(bool val)        { _useChunkedEncoding = val; }

   void isHeadRequest(bool value)           { _isHeadRequest = value; }
   bool isHeadRequest() const               { return _isHeadRequest; } 

   bool compressionEnabled() const          { return _compressionEnabled; }
   bool compressionActive() const;
   bool preparedForCompression() const      { return _preparedForCompression; }
   void prepareForCompression(CompressionRule rule);

   void* getGzipState();
   void clearGzipPendings();

private:

   void initGzipState(size_t dataSize);
   void destroyGzipState();
};


/**
 * @class ResponseSerializer
 * @brief A class responsible for serializing HTTP responses.
 *
 * \sa https://datatracker.ietf.org/doc/html/rfc2616#section-6
 * \sa https://datatracker.ietf.org/doc/html/rfc2616#section-4.3
 * 
 * The ResponseSerializer class provides methods to serialize HTTP responses
 * for different versions of the HTTP protocol. It supports both HTTP/1.x and
 * HTTP/2 protocols, with conditional compilation options for HTTP/2.
 *
 * The class holds a shared pointer to a Response object and provides methods
 * to serialize the response into a buffer for transmission over the network.
 *
 * @note The class supports conditional compilation for HTTP/2 serialization
 *       with or without data copying, based on the defined macros.
 */
class ResponseSerializer
{
private:
   std::shared_ptr<Response> _response {nullptr};

public:
   ResponseSerializer(std::shared_ptr<Response> response);
   size_t readLeft();
   int64_t serializeHttp1(asio::streambuf* sendBuffer, size_t bufferSize=0);
   int64_t serializeHttp1Body(asio::streambuf* sendBuffer, size_t bufferSize=0);

#ifdef TOBASA_HTTP_USE_HTTP2
   #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
      int64_t serializeHttp2(asio::streambuf* sendBuffer, size_t bufferSize);
private:
      int64_t doSerializeHttp2(asio::streambuf* sendBuffer, size_t bufferSize);
   #else
public:
      int64_t serializeHttp2(uint8_t* buffer, size_t bufferSize, uint32_t* dataFlags);
private:
      int64_t doSerializeHttp2(uint8_t* buffer, size_t bufferSize, uint32_t* dataFlags);
   #endif
#endif

private:
   int64_t doSerializeHttp1(asio::streambuf* sendBuffer, size_t bufferSize=0);

};

/** @}*/

} // namespace http
} // namespace tbs
