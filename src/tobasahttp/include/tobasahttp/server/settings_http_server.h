#pragma once

#include <tobasa/json.h>
#include <tobasahttp/host_certificate.h>

namespace tbs {
namespace http {

/// Http server configuration options
namespace conf {

/** \addtogroup WEB
 * @{
 */

struct HotsTlsCert
{
   std::string hostname             {};
   std::string certificateChainFile {};
   std::string privateKeyFile       {};
   std::string password             {};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HotsTlsCert, hostname, certificateChainFile, privateKeyFile, password)

struct Tls
{
   std::string certificateChainFile {"./localhost.crt"};
   std::string privateKeyFile       {"./localhost.key"};
   std::string password             {""};
   std::string tmpDhFile            {"./dh2048.pem"};
   std::vector<HotsTlsCert>hostCertificates;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Tls, certificateChainFile, privateKeyFile, password, tmpDhFile, hostCertificates)


inline static std::vector<http::HostCertificate> 
   toHostCertificates(const std::vector<HotsTlsCert>& source)
{
   std::vector<http::HostCertificate> result;
   for (auto& entry: source)
   {  
      http::HostCertificate data;
      
      data.hostname             = entry.hostname;
      data.certificateChainFile = entry.certificateChainFile;
      data.privateKeyFile       = entry.privateKeyFile;
      data.password             = entry.password;
      
      result.emplace_back(std::move(data));
   }
   
   return result;
}

struct Compression
{
   bool        enable         {true};
   int         minimalLength  {1024};  // bytes
   std::string encoding       {"gzip"};
   std::string mimetypes      {"text/plain text/css application/json application/javascript"};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Compression, enable, minimalLength, encoding, mimetypes)

/// Http Server configuration options
struct Server
{
   /// Run https server only if true
   bool runHttpsOnly      {false};
   /// IP Address this http server runs on
   std::string address    {"127.0.0.1"};
   int  port              {8084};
   int  portHttps         {8085};

   int  timeoutRead       {60};      // seconds
   int  timeoutWrite      {60};      // seconds
   int  timeoutProcessing {120};     // seconds
   int  readBufferSize    {1024*64}; // bytes
   int  sendBufferSize    {1024*64}; // bytes
   long maxHeaderSize     {1024*64}; // bytes

   /// Web Server document root for serving HTML pages
   std::string docRoot           {"./wwwroot"};

   std::string temporaryDir; // Temporary folder for processing multipart body

   Compression compression;

   Tls  tls;
   int  ioPoolSize                {4};
   int  workerPoolSize            {4};
   bool logVerbose                {false};
   bool useRateLimiter            {false};
   int  rateLimiterMaxRequests    {10};
   int  rateLimiterWindowDuration {1000};  // in milliseconds
   int  rateLimiterBlockDuration  {30000}; // in milliseconds 
   int  rateLimiterMaxViolations  {3};
   int  maxRequestsPerConnection  {100};

#ifdef TOBASA_HTTP_USE_HTTP2
   bool logVerboseHttp2           {false};
   bool http2Enabled              {false};
#endif

   bool enableMultipartParsing    {true};
};

#ifdef TOBASA_HTTP_USE_HTTP2
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Server, runHttpsOnly, address, port, portHttps
      , timeoutRead, timeoutWrite, timeoutProcessing, readBufferSize, sendBufferSize, maxHeaderSize
      , docRoot, temporaryDir, compression, tls, ioPoolSize, workerPoolSize, logVerbose, useRateLimiter , rateLimiterMaxRequests
      , rateLimiterWindowDuration, rateLimiterBlockDuration, rateLimiterMaxViolations, maxRequestsPerConnection
      , enableMultipartParsing , http2Enabled, logVerboseHttp2)
#else
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Server, runHttpsOnly, address, port, portHttps
      , timeoutRead, timeoutWrite, timeoutProcessing, readBufferSize, sendBufferSize, maxHeaderSize
      , docRoot, temporaryDir, compression, tls, ioPoolSize, workerPoolSize, logVerbose, useRateLimiter , rateLimiterMaxRequests
      , rateLimiterWindowDuration, rateLimiterBlockDuration, rateLimiterMaxViolations, maxRequestsPerConnection
      , enableMultipartParsing)
#endif

/** @}*/

} // namespace conf
} // namespace http
} // namespace tbs