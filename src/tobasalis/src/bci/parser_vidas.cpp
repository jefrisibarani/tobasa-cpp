#include <string>
#include <tobasa/logger.h>
#include "tobasalis/bci/record_vidas.h"
#include "tobasalis/bci/message.h"
#include "tobasalis/bci/parser_vidas.h"

namespace tbs {
namespace bci {

VidasParser::VidasParser(bool enableLog)
   : lis::Parser()
   , _enableLog(enableLog) 
{
   if (_enableLog) 
      Logger::logD("[lis_parser] Vidas parser initialized");   
}

void VidasParser::parse(const std::string& rawData)
{
   try
   {
      if (_enableLog) 
         Logger::logD("[lis_parser] parse: {}", rawData);

      /* -------------------------------------------------------

      --- raw
      \x2\x1emtrsl|piDKENT CLARK|pnDKENT CLARK|pb|ps|so|si|ciDKENT CLARK|rtHBS|rn
      \x1eHBs Ag Ultra|tt14:20|td26/05/2017|qlNegative|qn0.00|qd1|ncvalid|idVIDASPC01|sn|m
      \x1e4LabAdmin|\x1d2e\r\n

      --- removed <STX> , <GS> and <CR><LF>
      \x1emtrsl|piDKENT CLARK|pnDKENT CLARK|pb|ps|so|si|ciDKENT CLARK|rtHBS|rn
      \x1eHBs Ag Ultra|tt14:20|td26/05/2017|qlNegative|qn0.00|qd1|ncvalid|idVIDASPC01|sn|m
      \x1e4LabAdmin|"

      tempReceiveBuffer = "\x2\x1emtrsl|piDKENT CLARK|pnDKENT CLARK|pb|ps|so|si|ciDKENT CLARK|rtHBS|rn\x1eHBs Ag Ultra|tt14:20|td26/05/2017|qlNegative|qn0.00|qd1|ncvalid|idVIDASPC01|sn|m\x1e4LabAdmin|\x1d2e\r\n"
      cleanReceiveBuffer   = "\x1emtrsl|piDKENT CLARK|pnDKENT CLARK|pb|ps|so|si|ciDKENT CLARK|rtHBS|rn\x1eHBs Ag Ultra|tt14:20|td26/05/2017|qlNegative|qn0.00|qd1|ncvalid|idVIDASPC01|sn|m\x1e4LabAdmin|\x1d"
      
      ------------------------------------------------------- */

      // data is cleanReceiveBuffer
      // remove RS char : \x1e  = Decimal 30
      std::string data = "";
      for (size_t i = 0; i < rawData.length(); i++)
      {
         char c = rawData[i];
         if (c != 30)
            data += rawData[i];
      }

      auto recPtr = std::make_shared<bci::VidasRecord>(data);
      if (recPtr->fromString())
      {
         if (onRecordReady)
         {
            auto pR = std::static_pointer_cast<lis::Record>(recPtr);
            onRecordReady(pR);
         }

         // Vidas does not have "Header record", so create a dummy header
         auto vidasMessage = std::make_shared<VidasMessage>();
         vidasMessage->setHeader(std::make_shared<VidasRecord>(""));
         
         // Add the only record
         vidasMessage->getHeader()->addChild(recPtr);


         // -------------------------------------------------------
         // message contructed, emit event
         if (onMessageReady)
         {
            auto msg = std::static_pointer_cast<lis::Message>(vidasMessage);
            onMessageReady(msg);
         }
      }
      else 
         throw std::runtime_error("Failed parsing Vidas data");
   }
   catch(const std::exception& ex)
   {
      if (onParserError)
         onParserError(ex.what());
   }
}

} // namespace bci
} // namespace tbs