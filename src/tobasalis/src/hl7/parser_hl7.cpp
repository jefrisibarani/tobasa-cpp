#include <map>
#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/hl7/parser_hl7.h"
#include "tobasalis/hl7/message.h"

namespace tbs {
namespace hl7 {

ParserHl7::ParserHl7(bool enableLog)
   : lis::Parser() 
   , _enableLog(enableLog)
{
   if (_enableLog) 
      Logger::logD("[lis_parser] HL7 parser initialized");   
} 

void ParserHl7::parse(const std::string& data)
{
   try
   {
      if (_enableLog)
         Logger::logD("[parser] parse data: {}", data);

      // Full message come, all Control characters(eg: VT,FS) already removed
      // we dont use onRecordReady, only onMessageReady() 
      auto message = std::make_shared<hl7::Message>(data);

      // Set instrument type we got from Lis Engine
      message->instrumentType(this->instrumentType());  
      message->encoding()->instrumentType(this->instrumentType());
      
      //if (this->instrumentType() == lis::NONSTD_HL7)
      //{
      //   // Use FS as segement delimiter
      //   message->encoding()->segmentDelimiter(std::string(1,'\x1C'));
      //}

      bool parsed  = message->parseMessage();

      if (parsed && onMessageReady)
      {
         auto msg = std::static_pointer_cast<lis::Message>(message);
         msg->vendorProtocolId(lis::MSG_HL7);
         onMessageReady(msg);
      }  
   }
   catch(const std::exception& ex)
   {
      if (onParserError)
         onParserError(ex.what());
   }
}

} // namespace hl7
} // namespace tbs
