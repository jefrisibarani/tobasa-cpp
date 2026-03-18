#include <iostream>
#include <functional>
#include <asio.hpp>

#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include <tobasa/util_string.h>
#include <tobasa/config.h>
#include <tobasa/util.h>

#include "tobasalis/lis/common.h"
#include "tobasalis/lis1a/datalink_std.h"
#include "tobasalis/bci/datalink.h"
#include "tobasalis/lis1a/datalink_gem3500.h"
#include "tobasalis/dirui/datalink_diruih500.h"
#include "tobasalis/dirui/datalink_diruihbcc3600.h"
#include "tobasalis/hl7/datalink_hl7_std.h"
#include "tobasalis/lis/connection_rs232.h"
#include "tobasalis/lis/connection_tcp.h"
#include "tobasalis/lis/record.h"

#include "tobasalis/hl7/parser_hl7.h"
#include "tobasalis/dirui/parser_diruih500.h"
#include "tobasalis/dirui/parser_diruihbcc3600.h"
#include "tobasalis/bci/parser_vidas.h"
#include "tobasalis/bci/parser_vitek2compact.h"
#include "tobasalis/lis2a/parser.h"
#include "tobasalis/lis2a/message.h"
#include "tobasalis/lis2a/header.h"
#include "tobasalis/lis2a/patient.h"
#include "tobasalis/lis2a/order.h"
#include "tobasalis/lis2a/requestinfo.h"
#include "tobasalis/lis2a/result.h"
#include "tobasalis/lis2a/terminator.h"
#include "tobasalis/lis2a/comment.h"
#include "tobasalis/lis/engine.h"

namespace tbs {
namespace lis {

/*
   Supported / tested devices

   Indiko         : LIS2A
   DxH500         : LIS2A
   GEM3500        : LIS2A
   Selectra       : LIS2A
   Vidas          : bioMerieux
   Vitek2Compact  : bioMerieux
   DiruiH-500     : 
   DiruiBcc3600   : 
   DevTest_LIS1A  : LIS2A
   DevTest_HL7    : HL7 v2.x 
*/


LisEngine::LisEngine(asio::io_context& io_ctx, const conf::Engine& option)
   : _ioContext(io_ctx)
   , _engineOption(option)
   , _signals(_ioContext)
   , _started(false)
   , _useSerial(false)
   , _tcpServerMode(false)
   , _connected(false)
   , _rs232BaudRate(9600)
   , _rs232DataBits(8)
   , _communicationState(CommunicationState::Idle)
   , _runOnDedicatedThread(option.useDedicatedRunningThread)
{
   // Register to handle the signals that indicate when the server should exit.
   // It is safe to register for the same signal multiple times in a program,
   // provided all registration for the specified signal is made through Asio.
   _signals.add(SIGINT);
   _signals.add(SIGTERM);

#if defined(SIGQUIT)
   _signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
}

LisEngine::~LisEngine()
{
   stop();

   Logger::logT("[lis_engine] LisEngine destroyed");
}

std::string LisEngine::lisLinkState()
{
   if (_pLisLink)
      return _pLisLink->stateStr();
   else  
      return "Unknown";
}

void LisEngine::doAwaitStop()
{
   _signals.async_wait(
      [this](std::error_code /*ec*/, int /*signo*/)
      {
         Logger::logD("[lis_engine] Receive stop signal");

         // The server is stopped by cancelling all outstanding asynchronous
         // operations. Once all operations have finished the io_context::run()
         // call will exit.
         if (_started)
            stop();
      });
}

bool LisEngine::start()
{
   auto threadId0 = util::threadId(std::this_thread::get_id());
   Logger::logD("[lis_engine] Starting LIS Engine on thread id {}", threadId0);
   
   doAwaitStop();

   try
   {
      // Setup low level connection, serial port or tcp ip
      _useSerial = _engineOption.connection.activeConnection == "serial" ? true : false;

      LinkLimit conLimit;
      conLimit.receiveTimeOut = _engineOption.connection.receiveTimeOutSeconds;
      conLimit.enqTimeOut     = _engineOption.connection.enqTimeOutSeconds;
      conLimit.ackTimeOut     = _engineOption.connection.ackTimeOutSeconds;
      conLimit.sendRetry      = _engineOption.connection.sendRetry;
      
      conLimit.sendEachRecordInOneFrame        = _engineOption.connection.sendEachRecordInOneFrame;
      conLimit.sendRecordAsIntermediateFrame   = _engineOption.connection.sendRecordAsIntermediateFrame;
      conLimit.incomingDataAsIntermediateFrame = _engineOption.connection.incomingDataAsIntermediateFrame;
      
      if (_useSerial)
      {
         auto serialOption = _engineOption.connectionTypes.serialPort;

         // set up serial connection
         _rs232ComName  = serialOption.portName;
         _rs232BaudRate = serialOption.baudRate;
         _rs232DataBits = serialOption.dataBits;

         auto serial = std::make_shared<RS232Connection>(_ioContext);
         serial->setPortName(_rs232ComName);
         serial->setBaudRate(_rs232BaudRate);

         _pLisConn = serial;

         Logger::logI("[lis_engine] Serial port {} connection created", _rs232ComName);

         if (onConnectionCreated)
            onConnectionCreated("serial");
      }
      else
      {
         // set up tcp/ip connection, server or client mode
         auto tcpIpOption = _engineOption.connectionTypes.tcpIp;

         _tcpServerMode = tcpIpOption.activeMode == "server";
         std::string ip, port;
         if (_tcpServerMode)
         {
            _ipAddress = tcpIpOption.server.listenAddress;
            _ipPort = std::to_string(tcpIpOption.server.listenPort);
         }
         else
         {
            _ipAddress = tcpIpOption.client.serverAddress;
            _ipPort = std::to_string(tcpIpOption.client.serverPort);
         }

         auto pTcpConn = std::make_shared<TCPConnection>(_ioContext, _ipAddress, _ipPort, _tcpServerMode);
         _pLisConn = pTcpConn;

         if (_tcpServerMode)
            Logger::logI("[lis_engine] Tcp connection created in server mode");
         else
            Logger::logI("[lis_engine] Tcp connection created in cient mode");

         if (onConnectionCreated)
            onConnectionCreated("tcpip");
      }

      // now, setup DataLink object, which run on top Connection(serial/tcpip)

      conf::Instrument instrumentOption;
      for (auto instrument: _engineOption.instruments) 
      {
         if ( instrument.type == _engineOption.activeInstrument) 
            instrumentOption = instrument;
      }

      // LIS1A Devices
      if (instrumentOption.type == lis::DEV_DEFAULT_LIS1A && instrumentOption.dataLink == "LIS1A_STD")
         _pLisLink = std::make_unique<lis1a::DataLinkStd>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_TEST_LIS1A && instrumentOption.dataLink == "LIS1A_STD")
         _pLisLink = std::make_unique<lis1a::DataLinkStd>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_DXH_500 && instrumentOption.dataLink == "LIS1A_STD")
         _pLisLink = std::make_unique<lis1a::DataLinkStd>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_INDIKO && instrumentOption.dataLink == "LIS1A_STD")
         _pLisLink = std::make_unique<lis1a::DataLinkStd>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_SELECTRA && instrumentOption.dataLink == "LIS1A_STD")
         _pLisLink = std::make_unique<lis1a::DataLinkStd>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_GEM_3500 && instrumentOption.dataLink == "LIS1A_GEM3500")
         _pLisLink = std::make_unique<lis1a::DataLinkGem3500>(_ioContext, _pLisConn, conLimit);

      // HL7 Devices
      else if (instrumentOption.type == lis::DEV_DEFAULT_HL7 && instrumentOption.dataLink == "HL7_STD")
         _pLisLink = std::make_unique<hl7::DataLinkHl7Std>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_TEST_HL7 && instrumentOption.dataLink == "HL7_STD")
         _pLisLink = std::make_unique<hl7::DataLinkHl7Std>(_ioContext, _pLisConn, conLimit);

      // BCI Devices
      else if (instrumentOption.type == lis::DEV_VIDAS && instrumentOption.dataLink == "BCI")
         _pLisLink = std::make_unique<bci::DataLinkBci>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_VITEK2_COMPACT && instrumentOption.dataLink == "BCI")
         _pLisLink = std::make_unique<bci::DataLinkBci>(_ioContext, _pLisConn, conLimit);

      // DIRUI Devices
      else if (instrumentOption.type == lis::DEV_DIRUI_H_500 && instrumentOption.dataLink == "DIRUI_H500")
         _pLisLink = std::make_unique<dirui::DataLinkDirUih500>(_ioContext, _pLisConn, conLimit);
      else if (instrumentOption.type == lis::DEV_DIRUI_BCC_3600 && instrumentOption.dataLink == "DIRUI_BCC3600")
         _pLisLink = std::make_unique<dirui::DataLinkDirUiBcc3600>(_ioContext, _pLisConn, conLimit);

      else {
         _pLisLink = nullptr;
      }

      if (_pLisLink)
      {   
         using namespace std::placeholders;
         // connect Connection and DataLink event handler
         _pLisConn->onConnected      = std::bind(&LisEngine::connectionOnConnected, this, _1);
         _pLisConn->onConnectFailed  = std::bind(&LisEngine::connectionOnConnectFailed, this, _1);
         _pLisLink->onReceiveData    = std::bind(&LisEngine::linkOnReceiveData, this, _1);
         _pLisLink->onReceiveTimeOut = std::bind(&LisEngine::linkOnReceiveTimeOut, this, _1);
         _pLisLink->onLinkClosed     = std::bind(&LisEngine::linkOnLinkClosed, this, _1);
         _pLisLink->onIdle           = std::bind(&LisEngine::linkOnIdle, this, _1);

         // Note: we dont do this: Message parser initialized in LisEngine::linkOnReceiveData
         
         // setup incoming data parser
         lis2a::ParserOption option;
         option.autoDetectDelimiter = _engineOption.lisMessage.autoDetectDelimiters;
         option.fieldDelimiter      = _engineOption.lisMessage.delimiters.field.at(0);
         option.repeatDelimiter     = _engineOption.lisMessage.delimiters.repeat.at(0);
         option.componentDelimiter  = _engineOption.lisMessage.delimiters.component.at(0);
         option.escapeCharacter     = _engineOption.lisMessage.delimiters.escape.at(0);

         if (_engineOption.activeInstrument == lis::DEV_DXH_500) {
            option.resultRecordTotalField = 15;
         }

         _pParser = std::make_unique<lis2a::Parser>(option, _engineOption.lisMessage.logOnParserRead);
         _pParser->onRecordReady    = std::bind(&LisEngine::parserOnRecordReady, this, _1);
         _pParser->onMessageReady   = std::bind(&LisEngine::parserOnMessageReady, this, _1);
         _pParser->onParserError    = std::bind(&LisEngine::parserOnParserError, this, _1);
         
         _communicationState = CommunicationState::Idle;

         // all sets, connect and receive data now
         _pLisLink->connect();
         _started = true;

         // Call our handler
         if (onEngineStarted)
         {
            auto currentThreadId = util::threadId(std::this_thread::get_id());
            onEngineStarted(currentThreadId);
         }

         return true;
         //_ioContext.run();  // moved outside, called from caller scope
      }
      else
      {
         Logger::logE("[lis_engine] Incorrect active instrument configuration");
         return false;
      }
   }
   catch (tbs::AppException &e) {
      Logger::logE("[lis_engine] start() exception: {}, engine.cpp:{}", e.what(), __LINE__);
   }
   catch (std::exception & e) {
      Logger::logE("[lis_engine] start() exception: {}, engine.cpp:{}", e.what(), __LINE__);
   }

   return false;
}

bool LisEngine::stop()
{
   if (_started && _pLisLink)
   {
      Logger::logI("[lis_engine] Starting shutdown sequence");

      try
      {
         _pLisLink->disConnect();

         if (_workerThread && _workerThread->joinable())
            _workerThread->join();
      }
      catch (tbs::AppException & e) {
         Logger::logE("[lis_engine] stop() exception: {}, engine.cpp:{}", e.what(), __LINE__);
      }
      catch (const std::exception & e) 
      {
         Logger::logE("[lis_engine] stop() exception: {}, engine.cpp:{}", e.what(), __LINE__);
         return false;
      }

      _started = false;

      if (onEngineStopped)
         onEngineStopped("");

      Logger::logI("[lis_engine] Engine stopped");
   }

   return true;
}

void LisEngine::sendPendingMessage()
{
   if (!_outgoingMessages.empty())
   {
      if (runOnDedicatedThread())
      {
         if (_workerThread) 
         {
            if ( _workerThread->joinable() )
            _workerThread->join();
         }

         _workerThread = 
            std::make_shared<std::thread>( 
            [this] {
               auto msg = _outgoingMessages.front();
               sendMessage(msg.message->toString());
               _outgoingMessages.pop_front();

               if (msg.sentCallback)
                  msg.sentCallback();

               if (!_outgoingMessages.empty())
                  sendPendingMessage();
            });
      }
      else
      {
         auto msg = _outgoingMessages.front();
         sendMessageWithPost(msg.message->toString(), "CR",
            [this,msg] {
               _outgoingMessages.pop_front();

               if (msg.sentCallback)
                  msg.sentCallback();

               if (!_outgoingMessages.empty())
                  sendPendingMessage();
            });
      }
   }
}

void LisEngine::sendMessage(const std::vector<std::shared_ptr<Record>>& messageVec)
{
   _pLisLink->sendMessage(messageVec);
}

void LisEngine::sendMessage(const std::string& messageStr, const std::string& endOfLine)
{
   _pLisLink->sendMessage(messageStr, endOfLine);
}

void LisEngine::sendMessageWithPost(const std::string& messageStr, const std::string& endOfLine, std::function<void()> callback)
{
   auto cb = std::move(callback);
   std::exception_ptr exceptionCaught;
   asio::post(
      _ioContext.get_executor(),
      [this, messageStr, endOfLine, cb, &exceptionCaught] {
         try
         {
            _pLisLink->sendMessage(messageStr, endOfLine);
            if (cb)
               cb();
         }
         catch (...) {
            exceptionCaught = std::current_exception();
         }
      });
   
   // If an error was detected it should be propagated.
   if (exceptionCaught)
      std::rethrow_exception( exceptionCaught );
}

void LisEngine::sendMessage(std::shared_ptr<Message> message)
{
   _pLisLink->sendMessage(message->toString(), "CR");
}

void LisEngine::sendMessageWithPost(std::shared_ptr<Message> message, std::function<void()> callback)
{
   auto cb = std::move(callback);
   std::exception_ptr exceptionCaught;
   asio::post(
      _ioContext.get_executor(),
      [this, message, cb, &exceptionCaught] {
         try
         {
            _pLisLink->sendMessage(message->toString(), "CR");
            if (cb)
               cb();
         }
         catch (...) {
            exceptionCaught = std::current_exception();
         }
      });
   
   // If an error was detected it should be propagated.
   if (exceptionCaught)
      std::rethrow_exception( exceptionCaught );
}

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
void LisEngine::parseAndSendMessages(const std::string& messageStr, const std::string& endOfLine)
{
   Logger::logT("[lis_engine] parseAndSendMessages() : raw lis messages");

   if (messageStr.length() == 0)
      return;

   using arrayString = std::vector<std::string>;
   using arrayMessages = std::vector<arrayString>;
   arrayMessages messages;
   arrayString lisLines;

   // set new line character for splitting message
   std::string newLineCharacter;
   if ( endOfLine == "CR")
      newLineCharacter = "\r";
   else if ( endOfLine == "LF")
      newLineCharacter = "\n";
   else
      newLineCharacter = "\r\n";

   arrayString tokenized = tbs::util::split(messageStr, newLineCharacter);
   if (tokenized.size() == 0)
      return;

   // lines start with Header->Terminator is one Message
   // put every message to vector
   for (size_t i = 0; i < tokenized.size(); i++)
   {
      std::string line = tokenized[i];
      if ( !line.empty() || !std::isblank(line.at(0)) )
      {
         if (  tbs::util::startsWith(line, "H|")
            || tbs::util::startsWith(line, "P|")
            || tbs::util::startsWith(line, "O|")
            || tbs::util::startsWith(line, "Q|")
            || tbs::util::startsWith(line, "R|")
            || tbs::util::startsWith(line, "C|") )
         {
            lisLines.push_back(line);
         }

         if (tbs::util::startsWith(line, "L"))
         {
            lisLines.push_back(line);
            // we got one message, put it to messages
            messages.push_back(lisLines);
            lisLines = {};
         }
      }
   }

   // Now, send each message
   arrayMessages::iterator it;
   for (it = messages.begin(); it != messages.end(); ++it)
   {
      arrayString arr = *it;
      parseAndSendMessages(arr);
   }
}

void LisEngine::parseAndSendMessages(const std::vector<std::string>& messages)
{
   Logger::logT("[lis_engine] parseAndSendMessages() : raw lis messages array");

   if (messages.size() == 0)
      return;

   std::vector<std::shared_ptr<lis::Record>> lisRecordList;

   bool fromDxH500 = false;
   int lineNumber = 0;

   std::string headerline = messages[0];
   if (! tbs::util::startsWith( messages[0], "H|") )
      return;

   fromDxH500 = headerline.find("DxH 500") != std::string::npos;

   std::shared_ptr<lis2a::HeaderRecord> hdr = std::make_shared<lis2a::HeaderRecord>(headerline);
   hdr->fromString();
   lisRecordList.push_back(hdr);

   lineNumber += 1;
   while ( ! tbs::util::startsWith(messages[lineNumber], "L") )
   {
      auto line = messages[lineNumber];
      if (tbs::util::startsWith(line, "P"))
      {
         auto  prec = std::make_shared<lis2a::PatientRecord>(line);
         prec->fromString();
         lisRecordList.push_back(prec);
      }

      if (tbs::util::startsWith(line, "O"))
      {
         auto orec = std::make_shared<lis2a::OrderRecord>(line);
         orec->fromString();
         lisRecordList.push_back(orec);
      }

      if (tbs::util::startsWith(line, "Q"))
      {
         auto qrec = std::make_shared<lis2a::RequestInfoRecord>(line);
         qrec->fromString();
         lisRecordList.push_back(qrec);
      }

      if (tbs::util::startsWith(line, "R"))
      {
         int fiedCount = 14;
         if (fromDxH500) {
            fiedCount = 15;
         }

         auto  rrec = std::make_shared<lis2a::ResultRecord>(line, fiedCount);
         rrec->fromString();
         lisRecordList.push_back(rrec);
      }

      if (tbs::util::startsWith(line, "C"))
      {
         auto crec = std::make_shared<lis2a::CommentRecord>(line);
         crec->fromString();
         lisRecordList.push_back(crec);
      }

      lineNumber += 1;
   }

   auto trec = std::make_shared<lis2a::TerminatorRecord>(messages[lineNumber]);
   trec->fromString();
   lisRecordList.push_back(trec);

   sendMessage(lisRecordList);
}

void LisEngine::outgoingMessage(std::shared_ptr<Message> message, const std::string& name, std::function<void()> callback) 
{ 
   _outgoingMessages.push_back({message, name, std::move(callback)});
}

void LisEngine::connectionOnConnected(const std::string& data)
{
   Logger::logT("[lis_engine] connectionOnConnected: {}", data);
   _connected = true;

   if (onConnected)
      onConnected(data);
}

void LisEngine::connectionOnConnectFailed(const std::string& data)
{
   Logger::logT("[lis_engine] connectionOnConnectFailed: {}", data);

   if (onConnectFailed)
      onConnectFailed(data);

   stop();
}

void LisEngine::linkOnReceiveData(const std::string& data)
{
   if (_engineOption.lisMessage.logOnLinkReceive)
      Logger::logI("[lis_engine] linkOnReceiveData: {}", data);

   using namespace std::placeholders;

   if (    _engineOption.activeInstrument == lis::DEV_DEFAULT_LIS1A
        || _engineOption.activeInstrument == lis::DEV_TEST_LIS1A
        || _engineOption.activeInstrument == lis::DEV_DXH_500
        || _engineOption.activeInstrument == lis::DEV_INDIKO
        || _engineOption.activeInstrument == lis::DEV_GEM_3500
        || _engineOption.activeInstrument == lis::DEV_SELECTRA)
   {
      _pParser->instrumentType(_engineOption.activeInstrument);
      _pParser->parse(data);
   }
   else if (_engineOption.activeInstrument == lis::DEV_VIDAS)
   {
      bci::VidasParser parser(_engineOption.lisMessage.logOnParserRead);;
      parser.instrumentType(_engineOption.activeInstrument);
      parser.onRecordReady  = std::bind(&LisEngine::parserOnRecordReady, this, _1);
      parser.onMessageReady = std::bind(&LisEngine::parserOnMessageReady, this, _1);
      parser.onParserError  = std::bind(&LisEngine::parserOnParserError, this, _1);
      parser.parse(data);
   }
   else if (_engineOption.activeInstrument == lis::DEV_VITEK2_COMPACT)
   {
      bci::Vitek2CompactParser parser(_engineOption.lisMessage.logOnParserRead);
      parser.instrumentType(_engineOption.activeInstrument);
      parser.onRecordReady  = std::bind(&LisEngine::parserOnRecordReady, this, _1);
      parser.onMessageReady = std::bind(&LisEngine::parserOnMessageReady, this, _1);
      parser.onParserError  = std::bind(&LisEngine::parserOnParserError, this, _1);
      parser.parse(data);
   }
   else if (_engineOption.activeInstrument == lis::DEV_DIRUI_H_500)
   {
      dirui::ParserDirUih500 parser(_engineOption.lisMessage.logOnParserRead);;
      parser.instrumentType(_engineOption.activeInstrument);
      parser.onRecordReady  = std::bind(&LisEngine::parserOnRecordReady, this, _1);
      parser.onMessageReady = std::bind(&LisEngine::parserOnMessageReady, this, _1);
      parser.onParserError  = std::bind(&LisEngine::parserOnParserError, this, _1);
      parser.parse(data);
   }
   else if (_engineOption.activeInstrument == lis::DEV_DIRUI_BCC_3600)
   {
      dirui::ParserDirUiBcc3600 parser(_engineOption.lisMessage.logOnParserRead);;
      parser.instrumentType(_engineOption.activeInstrument);
      parser.onRecordReady  = std::bind(&LisEngine::parserOnRecordReady, this, _1);
      parser.onMessageReady = std::bind(&LisEngine::parserOnMessageReady, this, _1);
      parser.onParserError  = std::bind(&LisEngine::parserOnParserError, this, _1);
      parser.parse(data);
   }
   else if (_engineOption.activeInstrument == lis::DEV_DEFAULT_HL7
         || _engineOption.activeInstrument == lis::DEV_TEST_HL7 )
   {
      // HL7 parser does not use onRecordReady
      hl7::ParserHl7 parser(_engineOption.lisMessage.logOnParserRead);
      parser.instrumentType(_engineOption.activeInstrument);
      parser.onMessageReady = std::bind(&LisEngine::parserOnMessageReady, this, _1);
      parser.onParserError  = std::bind(&LisEngine::parserOnParserError, this, _1);
      parser.parse(data);
   }
   else
      Logger::logE("[lis_engine] linkOnReceiveData: No matched instrument found");
}

void LisEngine::linkOnReceiveTimeOut(const std::string& data)
{
   Logger::logT("[lis_engine] linkOnReceiveTimeOut: {}", data);
   _communicationState = CommunicationState::Idle;
}

void LisEngine::linkOnLinkClosed(const std::string& data)
{
   _connected = false;
   Logger::logT("[lis_engine] linkOnLinkClosed: {}", data);
}

void LisEngine::parserOnRecordReady(std::shared_ptr<Record>& record)
{
   if (_engineOption.lisMessage.logOnMessageReady)
      Logger::logT("[lis_engine] parserOnRecordReady: {}", record->toString());

   if (onRecordReady)
      onRecordReady(record);
}

void LisEngine::parserOnMessageReady(std::shared_ptr<Message>& message)
{
   if (_engineOption.lisMessage.logOnMessageReady)
      Logger::logT("[lis_engine] parserOnMessageReady: {}", message->toString() );

   // Forward to next handler which will save message to database
   // This handler should return immediately, because we don't want delay in 
   // saving to database causing block on current thread
   if (onMessageToStorage)
      onMessageToStorage(message);
   
   // Forward to next handler
   if (onMessageReady)
      onMessageReady(message);
}

void LisEngine::parserOnParserError(const std::string& data)
{
   Logger::logE("[lis_engine] Parser error: {}", data);

   if (onParserError)
      onParserError(data);
}

void LisEngine::linkOnIdle(std::shared_ptr<Connection> lisConn)
{
   Logger::logI("[lis_engine] Communication entering idle state");

   if (onCommunicationIdle) 
      onCommunicationIdle("");
}


} // namespace lis
} // namespace tbs
