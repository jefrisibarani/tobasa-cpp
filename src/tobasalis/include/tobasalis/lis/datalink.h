#pragma once

#include <condition_variable>
#include <sstream>
#include <mutex>
#include <functional>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <tobasa/non_copyable.h>
#include "tobasalis/lis/message.h"

namespace tbs {
namespace lis {

class Connection;


/** \ingroup LIS
 * LinkState.
 */
enum class LinkState
{
   Idle,
   Sending,
   Receiving,
   Establishing
};


/** 
 * LinkLimit.
 */
class LinkLimit
{
public:
   LinkLimit();

   int receiveTimeOut; // 30 secs
   int enqTimeOut;     // 15 secs
   int ackTimeOut;     // 15 secs
   int sendRetry;      // 5
   bool sendEachRecordInOneFrame;
   bool sendRecordAsIntermediateFrame;
   bool incomingDataAsIntermediateFrame;
};


/** 
 * DataLink.
 * LIS Connection Low Level Protocol
 *   +------------------------------------------------+
 *   | LIS Data Link                                  |
 *   |                 +---------------------------+  |
 *   |                 | TCPIP or RS232 Connection |  |
 *   |                 +---------------------------+  |
 *   +------------------------------------------------+
 */
class DataLink : private NonCopyable
{
public:
   /**
    * Event handlers 
    */
   std::function<void(const std::string&)> onReceiveData;
   std::function<void(const std::string&)> onLinkClosed;
   std::function<void(const std::string&)> onReceiveTimeOut;
   // should call or run async func
   std::function<void(std::shared_ptr<Connection>)> onIdle;

   /**
    * DataLink constructor.
    */
   DataLink(asio::io_context& io_ctx, std::shared_ptr<Connection> connection, const LinkLimit& limit);
   

   /**
    * DataLink destructor.
    */
   virtual ~DataLink() = default;
   
   /**
    * Connects the low level connection, e.g. opens the COM-port or opens the TCP/IP Socket
    */
   void connect();
   
   /**
    * Disconnects the low level connection, e.g. closes the COM-port or closes the TCP/IP Socket
    */
   void disConnect();

   /**
    * Link state
    */
   LinkState state();
   std::string stateStr();

   /**
    * Connection status
    */
   bool connected();

   /**
    * Parse given raw messsage and send. 
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    * 
    * endOfLine will be used to split messageStr as lis records.
    * messageStr will be tokenized by endOfLine, then reconstructed with CR as record separator
    */
   virtual void sendMessage(const std::string& message, const std::string& endOfLine="CRLF") {}
   
   virtual void asyncSendMessage(const std::string& message, const std::string& endOfLine="CRLF") {}

   /**
    * Send Lis message stored in record vector.
    * Convert each record object into raw lis record, then send.
    * @param records Records vector.
    */
   virtual void sendMessage(const std::vector<std::shared_ptr<Record>>& messageVec) {}
   

   /**
    * Send Lis message.
    * Convert message object into raw lis message, then send.
    * @param message Message object
    */   
   //virtual void sendMessage(const Message& message) {}   

protected:

   /**
    * Connection receive data event handler.
    */     
   virtual void connectionOnReceiveData(const std::string& data) {}
   
   /**
    * Starts the Receive Timeout counter.
    */
   virtual void startReceiveTimeoutTimer();

   /**
    * Stops the Receive Timeout counter.
    */
   virtual void stopReceiveTimeoutTimer();
   
   virtual void establishSendMode() {}

   virtual void stopSendMode() {}

   virtual void doSendMessage(const std::string& message) {}

   void setConnection(std::shared_ptr<lis::Connection> connection);

   void setStatus(LinkState state);

   void onTimerExpire(const asio::error_code& ec);
   
   std::string strInRaw(const std::string& data);

   std::mutex _mx_ENQ;
   std::mutex _mx_ACK;
   std::mutex _mx_EOT;

   std::condition_variable _cvWait_ENQ;
   std::condition_variable _cvWait_ACK;
   std::condition_variable _cvWait_EOT;

   const char STX = '\x02';       // hex: 02   dec: 2
   const char ETX = '\x03';       // hex: 03   dec: 3
   const char ETB = '\x17';       // hex: 17   dec: 23
   const char ENQ = '\x05';       // hex: 05   dec: 5
   const char ACK = '\x06';       // hex: 06   dec: 6
   const char NAK = '\x15';       // hex: 15   dec: 21
   const char EOT = '\x04';       // hex: 04   dec: 4
   const char CR  = '\r';         // hex: 0D   dec: 13
   const char LF  = '\n';         // hex: 0A   dec: 10
   const char NUL = '\x00';       // hex: 02   dec: 0

   const char _EOF      = '\x1A'; // hex: 1A
   const char _A        = '\x41'; // hex: 41
   const char _B        = '\x42'; // hex: 42
   const char _C        = '\x43'; // hex: 43
   const char _HASHTAG  = '\x30'; // hex: 30-39
   const char _STAR     = '\x2A'; // hex: 2A

   std::shared_ptr<lis::Connection> _pConnection;
   asio::steady_timer _receiveTimeOutTimer;
   std::stringstream  _tempReceiveBuffer;
   bool               _ackReceived;
   bool               _eotReceviced;
   LinkState          _state;
   LinkLimit          _limit;
};

} // namespace lis
} // namespace tbs 