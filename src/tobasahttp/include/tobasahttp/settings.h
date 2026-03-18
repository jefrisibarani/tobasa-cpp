#pragma once

#include <asio/ip/tcp.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <tobasa/util_string.h>

namespace tbs {
namespace http {

const int32_t HTTP_TIMEOUT_NOLIMIT              = 60*60*24;         // 24 hours
const int32_t HTTP_TIMEOUT_MIN                  = 10;               // 10 s
const int32_t HTTP_TIMEOUT_MAX                  = 60*60;            // 1 hour
const int32_t HTTP_TIMEOUT_DEFAULT              = 60;               // 60 s
const int32_t HTTP_PROCESSING_TIMEOUT_DEFAULT   = 120;              // 120 s

const size_t  HTTP_BUFFER_SIZE_MIN              = 512;              // 4 KB

const size_t  HTTP_READ_BUFFER_SIZE_MAX         = 1024*1024 * 8;    // 8 MB
const size_t  HTTP_READ_BUFFER_SIZE_DEFAULT     = 1024*64;          // 64 KB

const size_t  HTTP_SEND_BUFFER_SIZE_MAX         = 1024*1024 * 8;    // 8 MB
const size_t  HTTP_SEND_BUFFER_SIZE_DEFAULT     = 1024*64;          // 64 KB

const size_t  HTTP_HEADER_MAX_SIZE_MAX          = 1024*1024;        // 1 MB
const size_t  HTTP_HEADER_MAX_SIZE_DEFAULT      = 1024*64;          // 64 KB

const size_t  HTTP_WS_MSG_MAX_SIZE_MIN          = 1024*4;           // 4 KB
const size_t  HTTP_WS_MSG_MAX_SIZE_MAX          = 1024*1024;        // 1 MB
const size_t  HTTP_WS_MSG_MAX_SIZE_DEFAULT      = 1024*8;           // 8 KB

const size_t  HTTP_CLIENT_BODY_MAX_SIZE_MAX     = 1024*1024 * 50;  // 50 MB
const size_t  HTTP_CLIENT_BODY_MAX_SIZE_DEFAULT = 0;               // unlimited

/** \addtogroup HTTP
 * @{
 */

/** 
 * HTTP Server settings.
 * \tparam DerivedType
 */
template <typename DerivedType>
class SettingsBase
{
   using BaseType = SettingsBase<DerivedType>;

protected:
   using Derived  = DerivedType;
   // helpers for fluent API
   Derived& self() & { return static_cast<Derived&>(*this); }
   Derived&& self() && { return static_cast<Derived&&>(*this); }

   bool           _tlsMode           { false };

private:
   std::string    _address;
   uint16_t       _port;
   asio::ip::tcp  _protocol;

   int32_t        _timeoutWrite      { HTTP_TIMEOUT_DEFAULT };              // in seconds
   int32_t        _timeoutRead       { HTTP_TIMEOUT_DEFAULT };              // in seconds
   int32_t        _timeoutProcessing { HTTP_PROCESSING_TIMEOUT_DEFAULT };   // in seconds
   
   std::size_t    _readBufferSize    { HTTP_READ_BUFFER_SIZE_DEFAULT };     // in bytes
   std::size_t    _sendBufferSize    { HTTP_SEND_BUFFER_SIZE_DEFAULT};      // in bytes
   std::size_t    _maxHeaderSize     { HTTP_HEADER_MAX_SIZE_DEFAULT };      // in bytes

   uint16_t       _maxRequestsPerConnection  { 100 };

   bool           _enableMultipartParsing    { true };
   std::string    _temporaryDir              {"./tmp"};

   bool           _logVerbose          { false };

   #ifdef TOBASA_HTTP_USE_HTTP2
   bool           _logVerboseHttp2     { false };
   bool           _http2Enabled        { true };
   #endif

public:
   SettingsBase(
      std::string address = "localhost",
      uint16_t port = 8084,
      asio::ip::tcp protocol = asio::ip::tcp::v4() )
      : _address  { address }
      , _port     { port }
      , _protocol { protocol }
   {}

   // ------------------------------------------------
   Derived& port(uint16_t p) &
   {
      _port = (p == 0) ? 8084 : p;
      return self();
   }
   Derived&& port(uint16_t p) &&
   {
      return std::move(self().port(p));
   }
   [[nodiscard]]
   uint16_t port() const { return _port; }

   // ------------------------------------------------
   Derived& protocol(asio::ip::tcp p) &
   {
      _protocol = p;
      return self();
   }
   Derived&& protocol(asio::ip::tcp p) &&
   {
      return std::move(self().protocol(std::move(p)));
   }
   [[nodiscard]] asio::ip::tcp protocol() const { return _protocol; }

   // ------------------------------------------------
   Derived& address(std::string val) &
   {
      _address = std::move(val);
      return self();
   }
   Derived&& address(std::string val) &&
   {
      return std::move(self().address(std::move(val)));
   }
   [[nodiscard]]
   const std::string& address() const { return _address; }

   // ------------------------------------------------
   // HTTP write timeout in seconds
   // default 60s, min 10s, max 1 hour, set value to 0 disable limit 
   Derived& timeoutWrite(int32_t val) &
   {
      _timeoutWrite = clampTimeOut(val, HTTP_TIMEOUT_DEFAULT);
      return self();
   }
   Derived&& timeoutWrite(int32_t val) &&
   {
      return std::move(self().timeoutWrite(val));
   }
   // HTTP write timeout in seconds, default 60s, min 10s, max 1 hour
   [[nodiscard]] int32_t timeoutWrite() const { return _timeoutWrite; }

   // ------------------------------------------------
   // HTTP read timeout in seconds
   // default 60s, min 10s, max 1 hour, set value to 0 disable limit 
   Derived& timeoutRead(int32_t val) &
   {
      _timeoutRead = clampTimeOut(val, HTTP_TIMEOUT_DEFAULT);
      return self();
   }
   Derived&& timeoutRead(int32_t val) &&
   {
      return std::move(self().timeoutRead(val));
   }
   [[nodiscard]] int32_t timeoutRead() const { return _timeoutRead; }

   // ------------------------------------------------
   // HTTP processing timeout in seconds
   // default 120s, min 10s, max 1 hour, set value to 0 disable limit
   Derived& timeoutProcessing(int32_t val) &
   {
      _timeoutProcessing = clampTimeOut(val, HTTP_PROCESSING_TIMEOUT_DEFAULT);
      return self();
   }
   Derived&& timeoutProcessing(int32_t val) &&
   {
      return std::move(self().timeoutProcessing(val));
   }
   [[nodiscard]] int32_t timeoutProcessing() const { return _timeoutProcessing; }

   // ------------------------------------------------
   // HTTP socket receive/read buffer size
   // default 64KB, min 16 KB, max 8 MB
   Derived& readBufferSize(std::size_t val) &
   {
      _readBufferSize = clampValue(val, HTTP_BUFFER_SIZE_MIN, HTTP_READ_BUFFER_SIZE_MAX, HTTP_READ_BUFFER_SIZE_DEFAULT);
      return self();
   }
   Derived&& readBufferSize(std::size_t val) &&
   {
      return std::move(self().readBufferSize(val));
   }
   [[nodiscard]] std::size_t readBufferSize() const { return _readBufferSize; }

   // ------------------------------------------------
   // HTTP socket send/write buffer size
   // default 64KB, min 16 KB, max 8 MB
   Derived& sendBufferSize(std::size_t val) &
   {
      _sendBufferSize = clampValue(val, HTTP_BUFFER_SIZE_MIN, HTTP_SEND_BUFFER_SIZE_MAX, HTTP_SEND_BUFFER_SIZE_DEFAULT);
      return self();
   }
   Derived&& sendBufferSize(std::size_t val) &&
   {
      return std::move(self().sendBufferSize(val));
   }
   [[nodiscard]] std::size_t sendBufferSize() const { return _sendBufferSize; }

   // ------------------------------------------------
   // HTTP header maximum size
   // default 64KB, min 16 KB, max 1 MB
   Derived& maxHeaderSize(std::size_t val) &
   {
      _maxHeaderSize = clampValue(val, HTTP_BUFFER_SIZE_MIN, HTTP_HEADER_MAX_SIZE_MAX, HTTP_HEADER_MAX_SIZE_DEFAULT);
      return self();
   }
   Derived&& maxHeaderSize(std::size_t val) &&
   {
      return std::move(self().maxHeaderSize(val));
   }
   [[nodiscard]] std::size_t maxHeaderSize() const { return _maxHeaderSize; }
   
   // ------------------------------------------------
   // Maximum HTTP requests per connection
   // default 100, max 100000, 0 to disable
   Derived & maxRequestsPerConnection(uint16_t val) &
   {
      _maxRequestsPerConnection = self().clampValue(val, (uint16_t)0, (uint16_t)100000, (uint16_t)100);
      return self();
   }
   Derived&& maxRequestsPerConnection(uint16_t val) &&
   {
      return std::move(self().maxRequestsPerConnection(val));
   }
   [[nodiscard]] uint16_t maxRequestsPerConnection() const { return _maxRequestsPerConnection; }

   // ------------------------------------------------
   Derived& tlsMode(bool val) &
   {
      _tlsMode = val;
      return self();
   }
   Derived&& tlsMode(bool val) &&
   {
      return std::move(self().tlsMode(val));
   }
   [[nodiscard]] bool tlsMode() const { return _tlsMode; }

   // ------------------------------------------------
   Derived& logVerbose(bool val) &
   {
      _logVerbose = val;
      return self();
   }
   Derived&& logVerbose(bool val) &&
   {
      return std::move(self().logVerbose(val));
   }
   [[nodiscard]] bool logVerbose() const { return _logVerbose; }

   // ------------------------------------------------
#ifdef TOBASA_HTTP_USE_HTTP2
   Derived& logVerboseHttp2(bool val) &
   {
      _logVerboseHttp2 = val;
      return self();
   }
   Derived&& logVerboseHttp2(bool val) &&
   {
      return std::move(self().logVerboseHttp2(val));
   }
   [[nodiscard]] bool logVerboseHttp2() const { return _logVerboseHttp2; }


   Derived& http2Enabled(bool val) &
   {
      _http2Enabled = val;
      return self();
   }
   Derived&& http2Enabled( bool val ) &&
   {
      return std::move( self().http2Enabled( val ) );
   }
   [[nodiscard]] bool http2Enabled() const { return _http2Enabled; }
#endif


   // ------------------------------------------------
   Derived& temporaryDir(std::string val) &
   {
      _temporaryDir = std::move(val);
      return self();
   }
   Derived&& temporaryDir(std::string val) &&
   {
      return std::move(self().temporaryDir(val));
   }
   [[nodiscard]] std::string temporaryDir() const { return _temporaryDir; }

   // ------------------------------------------------
   Derived& enableMultipartParsing(bool val) &
   {
      _enableMultipartParsing = val;
      return self();
   }
   Derived&& enableMultipartParsing(bool val) &&
   {
      return std::move(self().enableMultipartParsing(val));
   }
   [[nodiscard]] bool enableMultipartParsing() const { return _enableMultipartParsing; }

protected:
   int32_t clampTimeOut(int32_t val, int32_t def)
   {
      if (val == 0)
         return HTTP_TIMEOUT_NOLIMIT;

      if (val >= HTTP_TIMEOUT_MIN && val <= HTTP_TIMEOUT_MAX)
         return val;

      return def;
   }

   template<typename T>
   T clampValue(T value, T min, T max, T def)
   {
      if (value >= min && value <= max)
         return value;

      return def;
   }
};

/** @}*/

} // namespace http
} // namespace tbs