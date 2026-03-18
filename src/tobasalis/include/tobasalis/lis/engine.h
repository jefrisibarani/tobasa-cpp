#pragma once

#include <memory>
#include <functional>
#include <signal.h>
#include <deque>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/thread.hpp>
#include <asio/signal_set.hpp>
#include <tobasa/non_copyable.h>
#include "tobasalis/lis/settings.h"

namespace tbs {
namespace lis {

class Message;
class Record;
class Connection;
class DataLink;
class Parser;

/** \ingroup LIS
 * LisEngine
 */
enum class CommunicationState
{
   Idle,
   OrderRequestMessageReceived,     // Order Request Message received from Analyzer
   OrderMessageSent,                // Order Message sent to Analyzer (in response to Order Request message from Analyzer)
   OrderMessageAckReceived,         // ACK message from analyzer received
   OrderMessageReceived,            // Received Order Message from LIS Manager
   OrderMessageReceivedAckSent      // ACK message sent (in response to received Order Message from LIS Manager)
};

struct OutgoingMessage
{
   std::shared_ptr<Message> message;
   std::string              name;
   std::function<void()>    sentCallback;
};

typedef std::deque<OutgoingMessage> MessageQueue;

class LisEngine : private NonCopyable
{
private:

   asio::io_context& _ioContext;
   /// The signal_set is used to register for process termination notifications.
   asio::signal_set  _signals;
   bool              _started;
   bool              _useSerial;
   bool              _tcpServerMode;
   bool              _connected;       // client mode server connectivity status

   std::string       _rs232ComName;
   int               _rs232BaudRate;
   int               _rs232DataBits;
   std::string       _ipAddress;
   std::string       _ipPort;

   std::shared_ptr<Connection>  _pLisConn;
   std::unique_ptr<DataLink>    _pLisLink;
   std::unique_ptr<Parser>      _pParser;
   conf::Engine                 _engineOption;

   MessageQueue                 _outgoingMessages;
   CommunicationState           _communicationState;
   std::shared_ptr<std::thread> _workerThread;
   bool                         _runOnDedicatedThread;

public:
   explicit LisEngine(asio::io_context& io_ctx, const conf::Engine& option);
   virtual ~LisEngine();

   bool start();
   bool stop();

   bool runOnDedicatedThread() { return  _runOnDedicatedThread; }

   bool        isStarted() const          { return _started; }
   bool        isConnected() const        { return _connected; }
   bool        isTCPServerMode() const    { return  !_useSerial && _tcpServerMode; }
   std::string getSerialComName() const   { return _rs232ComName; }
   int         getSerialBaudRate() const  { return _rs232BaudRate; }
   int         getSerialDataBits() const  { return _rs232DataBits; }
   std::string getIpAddress() const       { return _ipAddress; }
   std::string getIpPort() const          { return _ipPort; }
   void        setUseSerialPort(bool useSerial = true) { _useSerial = useSerial; }

   std::string lisLinkState();

   MessageQueue& outgoingMessages() { return _outgoingMessages;}
   void outgoingMessage(std::shared_ptr<Message> message, const std::string& name = {}, std::function<void()> callback=nullptr);
   CommunicationState communicationState() { return _communicationState; }
   void communicationState(CommunicationState state) {_communicationState=state;}

   /**
    * Send pending message
    */
   void sendPendingMessage();

   /**
    * Send LIS2-A2 message stored in record vector.
    * Convert each record object into raw lis record, then send.
    * @param records Records vector.
    */
   void sendMessage(const std::vector<std::shared_ptr<Record>>& records);

   /**
    * Send raw LIS2-A2 messsage. 
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    * 
    * endOfLine will be used to split messageStr as lis records.
    * 
    * Example LIS2-A2 raw message text:
    * Here <CR><LF> is end of line character
    * 
    *  H|\^&|||1^LIS host^1.0|||||||P<CR>
    *  P|1|PatientID_03|||Patient Name_3|||U|||||||||||||||||Doctor Name|<CR>
    *  O|1|SampleID_03||^^^ISE|S||20101102100000||||||Test information field||3||||||||||O<CR>
    *  L|1|F<CR>
    * 
    * messageStr will be tokenized by endOfLine, then reconstructed with CR as record separator
    */
   void sendMessage(const std::string& messageStr, const std::string& endOfLine="CR");
   void sendMessageWithPost(const std::string& messageStr, const std::string& endOfLine="CR", std::function<void()> callback=nullptr);

   /**
    * Send LIS2-A2 message.
    * Convert message object into raw lis message, then send.
    * @param message Lis2-a2 Message object
    */ 
   void sendMessage(std::shared_ptr<Message> message);
   void sendMessageWithPost(std::shared_ptr<Message> message, std::function<void()> callback=nullptr);

   /** 
    * Parse then send raw LIS2-A2 message or multiple LIS2-A2 message.
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    *
    * endOfLine will be used to split messageStr as lis records.
    * each lis records then converted back to lis string
    *
    * Message
    *   #1--+--  H|\^&|||1^LIS host^1.0|||||||P<CR><LF>
    *       |    P|1|PatientID_03|||Patient Name_3|||U|||||||||||||||||Doctor Name|<CR><LF>
    *       |    O|1|SampleID_03||^^^ISE|S||20101102100000||||||Test information field||3||||||||||O<CR><LF>
    *       \--  L|1|F<CR><LF>
    *
    * Message
    *   #2--+--  H|\^&|||1^LIS host^1.0|||||||P<CR><LF>
    *       |    P|1|PatientID_04|||Patient Name_4|||U|||||||||||||||||Doctor Name|<CR><LF>
    *       |    O|1|SampleID_04||^^^ISE__test|S||20101102100000||||||Test information field||3||||||||||O<CR><LF>
    *       \--  L|1|F<CR><LF>
    */
   void parseAndSendMessages(const std::string& messageStr, const std::string& endOfLine="CRLF");


   /// Event to forward
   std::function<void(const std::string&)>        onEngineStarted;
   std::function<void(const std::string&)>        onEngineStopped;
   std::function<void(const std::string&)>        onConnectionCreated;
   std::function<void(const std::string&)>        onConnected;
   std::function<void(const std::string&)>        onConnectFailed;
   std::function<void(double)>                    onSendProgress;
   // should run async func
   std::function<void(const std::string&)>        onCommunicationIdle;

   /// LisParser event handlers forwarders
   std::function<void(std::shared_ptr<Record>&)>  onRecordReady;
   std::function<void(std::shared_ptr<Message>&)> onMessageReady;
   std::function<void(std::shared_ptr<Message>&)> onMessageToStorage;
   std::function<void(const std::string&)>        onParserError;

   /**
    * \brief Returns the IO context used by the Webapp.
    * \return Reference to the IO context.
    */
   asio::io_context& ioContext() { return _ioContext; }

private:

   void parseAndSendMessages(const std::vector<std::string>& messages);

   /// Wait for a request to stop the server.
   void doAwaitStop();

   /// Event Handlers
   void connectionOnConnected(     const std::string& data);
   void connectionOnConnectFailed( const std::string& data);
   void linkOnReceiveData(         const std::string& data);
   void linkOnReceiveTimeOut(      const std::string& data);
   void linkOnLinkClosed(          const std::string& data);

   void parserOnRecordReady(       std::shared_ptr<Record>& record);
   void parserOnMessageReady(      std::shared_ptr<Message>& message);
   void parserOnParserError(       const std::string& data);
   // should run async func
   void linkOnIdle(std::shared_ptr<Connection> lisConn);
};

} // namespace lis
} // namespace tbs
