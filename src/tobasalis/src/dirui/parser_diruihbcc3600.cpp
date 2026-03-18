#include <tobasa/logger.h>
#include "tobasalis/dirui/record_diruihbcc3600.h"
#include "tobasalis/dirui/parser_diruihbcc3600.h"
#include "tobasalis/dirui/message_diruih.h"

namespace tbs {
namespace dirui {

ParserDirUiBcc3600::ParserDirUiBcc3600(bool enableLog)
   : lis::Parser()
   , _enableLog(enableLog)
{
   if (_enableLog) 
      Logger::logD("[lis_parser] DirUiBcc3600 parser initialized");   
}        

void ParserDirUiBcc3600::parse(const std::string& data)
{
   try
   {
      if (_enableLog) 
         Logger::logD("[lis_parser] parse: {}", data);

      auto record = std::make_shared<RecordDirUiBcc3600>(data);

      if (record->fromString())
      {
         if (onRecordReady)
         {
            auto pRrec = std::dynamic_pointer_cast<lis::Record>(record);
            onRecordReady(pRrec);
         }

         if (_enableLog) 
            Logger::logT("[lis_parser] Constructing LIS Message");

         auto message = std::make_shared<Message>();
         message->instrumentType(this->instrumentType());
         // DirUI does not have "Header record", so create a dummy header
         message->setHeader(std::make_shared<RecordDirUiBcc3600>(""));
         // Add the only record
         message->getHeader()->addChild(record);

         // one Message constructed, emit event
         if (onMessageReady)
         {
            auto msg = std::static_pointer_cast<lis::Message>(message);
            onMessageReady(msg);
         }
      }
      else 
         throw std::runtime_error("Failed parsing DirUI data");
   }
   catch(const std::exception& ex)
   {
      if (onParserError)
         onParserError(ex.what());
   }
}

} // namespace dirui
} // namespace tbs
