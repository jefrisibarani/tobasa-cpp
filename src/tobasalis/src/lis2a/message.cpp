#include <utility>
#include <tobasa/logger.h>
#include <tobasa/exception.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/lis2a/message.h"
#include "tobasalis/lis2a/patient.h"
#include "tobasalis/lis2a/header.h"
#include "tobasalis/lis2a/requestinfo.h"
#include "tobasalis/lis2a/terminator.h"
#include "tobasalis/lis2a/order.h"

namespace tbs {
namespace lis2a {

Message::Message()
   : lis::Message() 
{
   _vendorProtocolId = lis::MSG_LIS2A;
}

void Message::addPatient(const lis::RecordPtr& patient)
{
   if (!_pHeader)
      throw AppException("Invalid header object in lis message");

   _pHeader->addChild(patient);
}

void Message::addRequestInfo(const lis::RecordPtr& query)
{
   if (!_pHeader)
      throw AppException("Invalid header object in lis message");

   _pHeader->addChild(query);
}

std::shared_ptr<PatientRecord> Message::getLastPatient()
{
   if (_hasRequest)
      return nullptr;
      
   auto child = _pHeader->getLastChild();
   return std::static_pointer_cast<PatientRecord>(child);
}

std::shared_ptr<HeaderRecord> Message::getHeaderRecord()
{
   if (_pHeader) {
      return std::static_pointer_cast<HeaderRecord>(_pHeader);
   }

   return nullptr;
}

std::shared_ptr<RequestInfoRecord> Message::getRequestInfo()
{
   if (_hasRequest)
   {
      auto childBasePtr = _pHeader->getLastChild();
      return std::static_pointer_cast<RequestInfoRecord>(childBasePtr);
   }
   else
      return nullptr;
}

bool Message::isAckMessage()
{
   if (!_pHeader)
      return false;
   else 
   {
      if (_pHeader->getChildren())
         return false;

      auto record = std::static_pointer_cast<TerminatorRecord>(_pHeader->getNext());
      if (record && (record->recordType() == RecordType::Terminator))
      {
         if (record->codeEnum() == TerminationCode::Normal)
            return true;
      }
   }

   return false;
}

bool Message::isOrderRequest()
{
   if (!_pHeader)
      return false;
   else 
   {
      auto patient = _pHeader->getChildren();
      if (patient)
      {
         // TODO_JEFRI: Fix this. Every Patient should have an order

         if (patient->getChildren())
         {
            // Found patient with order, return true
            auto order = std::static_pointer_cast<OrderRecord>(patient->getChildren());
            if (order)
               return true;
         }
         else
         {
            while (patient->getNext())
            {
               // First time we found patient with order, return true
               auto order = std::static_pointer_cast<OrderRecord>(patient->getChildren());
               if (order)
                  return true;

               patient = patient->getNext();
            }
         }
      }
   }

   return false;
}


std::string Message::toString()
{
   if (!_pHeader)
   {
      Logger::logE("[message] invalid LIS Message");
      return "";
   }

   std::string resultStr;
   resultStr.append(_pHeader->toString());

   // get patient or request
   auto patient = _pHeader->getChildren();
   while (patient)
   {
      resultStr.append(patient->toString());

      auto order = patient->getChildren();
      while (order)
      {
         resultStr.append(order->toString());

         auto result = order->getChildren();
         while (result)
         {
            resultStr.append(result->toString());
            result = result->getNext();
         }
         order = order->getNext();
      }
      patient = patient->getNext();
   }

   auto term = _pHeader->getNext();
   if (term)
   {
      resultStr.append(_pHeader->getNext()->toString());
   }

   return resultStr;
}

} // namespace lis2a
} // namespace tbs
