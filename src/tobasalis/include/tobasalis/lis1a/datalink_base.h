#pragma once

#include <asio/strand.hpp>
#include "tobasalis/lis/datalink.h"

namespace tbs {
namespace lis1a {

/** DataLinkBase
 * CLSI LIS01-A2 (formerly ASTM1381-02)  Low Level Protocol
 *   +------------------------------------------------+
 *   | LIS Data Link                                  |
 *   |                 +---------------------------+  |
 *   |                 | TCPIP or RS232 Connection |  |
 *   |                 +---------------------------+  |
 *   +------------------------------------------------+
 */
class DataLinkBase
   : public lis::DataLink
{
public:

   using AsioExecutor = asio::any_io_executor;
   using Executor     = asio::strand<AsioExecutor>;

   /**
    * DataLinkBase constructor.
    */  
   DataLinkBase(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);
   
   DataLinkBase() = default;
   virtual ~DataLinkBase() = default;


   /**
    * Parse given raw LIS2-A2 messsage and send. 
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    * 
    * endOfLine will be used to split messageStr as lis records.
    * 
    * Example LIS2-A2 raw message text:
    * Here <CR><LF> is end of line character
    * 
    *  H|\\^&|||1^LIS host^1.0|||||||P<CR><LF>
    *  P|1|PatientID_03|||Patient Name_3|||U|||||||||||||||||Doctor Name|<CR><LF>
    *  O|1|SampleID_03||^^^ISE|S||20101102100000||||||Test information field||3||||||||||O<CR><LF>
    *  L|1|F<CR><LF>
    * 
    * messageStr will be tokenized by endOfLine, then reconstructed with CR as record separator
    */
   virtual void sendMessage(const std::string& message, const std::string& endOfLine="CRLF") {}
   
   
   /**
    * Send LIS2-A2 message stored in record vector.
    * Convert each record object into raw lis record, then send.
    * @param records Records vector.
    */   
   virtual void sendMessage(const std::vector<std::shared_ptr<lis::Record>>& messageVec) {}

protected:

   virtual std::string calculateChecksum(const std::string& data) { return {}; }

   /**
    * Connection receive data event handler.
    */  
   virtual void connectionOnReceiveData(const std::string& data)  {}


   /**
    * Wait for ACK from remote lis
    */  
   virtual bool waitForACK();


   /**
    * Initiates the Establishment Phase, see CLSI LIS01-A2 Chapter 6.2
    * @returns True when the receiving system answered with an ACK within 15 seconds</returns>
    */
   virtual void establishSendMode();

   virtual void sendString(const std::string& data);

   /**
    * Transmits a message to the receiver.
    * @param message The message to be transmitted, messages are sent in frames;
    * each frame contains a maximum of 64000 characters (including frame overhead). 
    * Messages longer than 64000 characters are divided between two or more frames.
    */
   virtual void doSendMessage(const std::string& message);

   /**
    * Transmits the EOT transmission control character and then regards the data link to be in a neutral state.
    */
   virtual void stopSendMode();

   Executor          _executor;
   const int MAXFRAMESIZE = 63993;
   std::string        _tempIntermediateFrameBuffer;
   int                _frameNumber;
   bool               _lastFrameWasIntermediate;
};

} // namespace lis1a
} // namespace tbs