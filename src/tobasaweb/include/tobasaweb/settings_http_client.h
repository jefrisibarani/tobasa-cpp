#pragma once

#include <tobasa/json.h>

namespace tbs {
namespace http {

/// Http server configuration options
namespace conf {

/** \addtogroup WEB
 * @{
 */


/// Http Server configuration options
struct Client
{
   /// IP Address this client connect to
   std::string address            {"127.0.0.1"};
   int  port                      {80};

   bool tlsMode                   {false};
   bool verifyPeer                {true};
   std::string caVerificationFile {};

   // mTLS client

   std::string privateKeyFile     {};
   std::string privateKeyPassword {};
   std::string certificateFile    {};
   

   int  timeoutRead               {60};      // seconds
   int  timeoutWrite              {60};      // seconds
   int  timeoutProcessing         {120};     // seconds
   int  readBufferSize            {1024*64}; // bytes
   int  sendBufferSize            {1024*64}; // bytes
   long maxHeaderSize             {1024*64}; // bytes
   int  connectionPoolSize        {1};
   int  maxRequestsPerConnection  {100};

   bool logVerbose                {false};
   std::string temporaryDir       {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
   Client, address, port, 
   tlsMode, verifyPeer, caVerificationFile, 
   privateKeyFile, privateKeyPassword, certificateFile,
   timeoutRead, timeoutWrite, timeoutProcessing, readBufferSize,
   sendBufferSize, maxHeaderSize, connectionPoolSize, maxRequestsPerConnection,
   logVerbose, temporaryDir)

/** @}*/

} // namespace conf
} // namespace http
} // namespace tbs
