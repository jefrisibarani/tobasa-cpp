#include <tobasa/logger.h>
#include "tobasalis/dirui/record_diruih500.h"
#include "tobasalis/dirui/parser_diruih500.h"
#include "tobasalis/dirui/message_diruih.h"

namespace tbs {
namespace dirui {

ParserDirUih500::ParserDirUih500(bool enableLog)
   : lis::Parser()
   , _enableLog(enableLog)
{
   if (_enableLog)
      Logger::logD("[lis_parser] DirUi500 parser initialized");
}

void ParserDirUih500::parse(const std::string& data)
{
   try
   {
      if (_enableLog)
         Logger::logD("[lis_parser] parse: {}", data);

      auto record = std::make_shared<DirUih500Record>(data);
      if (record->fromString())
      {
         if (onRecordReady)
         {
            auto pRrec = std::dynamic_pointer_cast<lis::Record>(record);
            onRecordReady(pRrec);
         }

         if (_enableLog)
            Logger::logD("[lis_parser] Constructing LIS Message");

         auto message = std::make_shared<Message>();
         message->instrumentType(this->instrumentType());
         // DirUI does not have "Header record", so create a dummy header
         message->setHeader(std::make_shared<DirUih500Record>(""));
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
