#include <utility>
#include <tobasa/logger.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/bci/message.h"

namespace tbs {
namespace bci {

BciMessage::BciMessage()
   : lis::Message()
{
   _vendorProtocolId = lis::MSG_BCI;
}

void BciMessage::addTest(const lis::RecordPtr& test)
{
   _pHeader->addChild(test);
}

std::shared_ptr<Record> BciMessage::getTest()
{
   return std::dynamic_pointer_cast<Record>(_pHeader->getLastChild());
}

std::string BciMessage::toString()
{
   if (!_pHeader)
   {
      Logger::logE("[lis_message] invalid LIS Message");
      return "";
   }

   std::string resultStr;
   resultStr.append(_pHeader->toString());

   auto record = _pHeader->getChildren();

   while (record)
   {
      auto record_ = std::static_pointer_cast<Record>(record);
      resultStr.append(record->toString());

      if (record_->recordType() == RecordType::Test)
      {
         auto child = record->getChildren();
         while (child)
         {
            auto child_ = std::static_pointer_cast<Record>(child);
            resultStr.append(child->toString());

            // Get Af records
            auto ap = child->getChildren();
            while (ap)
            {
               auto ap_ = std::static_pointer_cast<Record>(ap);
               resultStr.append(ap->toString());
               ap = ap->getNext();// AF's children (ap )
            }

            child = child->getNext(); // Tests's children (AF and Suspect Result )
         }
      }

      record = record->getNext();
   }

   return resultStr;
}

// VidaMessage

VidasMessage::VidasMessage()
   : lis::Message() 
{
   _vendorProtocolId = lis::MSG_BCI;
}

VidasMessage::~VidasMessage() {}

std::string VidasMessage::toString()
{
   if (!_pHeader)
   {
      Logger::logE("[lis_message] invalid LIS Message");
      return "";
   }

   std::string resultStr;

   auto record = _pHeader->getChildren();

   while (record)
   {
      auto record_ = std::static_pointer_cast<Record>(record);
      resultStr.append(record->toString());
      record = record->getNext();
   }

   return resultStr;
}

} // namespace bci
} // namespace tbs