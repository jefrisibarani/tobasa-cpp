#include <string>
#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/lis2a/header.h"
#include "tobasalis/lis2a/subrecord.h"
#include "tobasalis/lis2a/patient.h"
#include "tobasalis/lis2a/order.h"
#include "tobasalis/lis2a/requestinfo.h"
#include "tobasalis/lis2a/result.h"
#include "tobasalis/lis2a/terminator.h"
#include "tobasalis/lis2a/comment.h"
#include "tobasalis/lis2a/message.h"
#include "tobasalis/lis2a/parser.h"

namespace tbs {
namespace lis2a {

Parser::Parser(ParserOption option, bool enableLog)
   : lis::Parser()
   , _option(option)
   , _enableLog(enableLog)
{
   if (_enableLog) 
      Logger::logD("[lis_parser] lis2a parser initialized");
}

void Parser::parse(const std::string& data)
{
   try
   {
      if (_enableLog) 
         Logger::logD("[lis_parser] parse data: {}", data);

      // Full message (Header-Terminator) might come inside one frame : <STX>.....<ETX>,
      // or one frame per Record
      // split incoming frame data into records by using CR as separator
      std::vector<std::string> array = tbs::util::split(data, "\r");

      size_t i = 0;
      if (!array.empty())
      {
         while (i < array.size())
         {
            std::string r = array[i];
            if (!r.empty())
            {
               if (_enableLog) 
                  Logger::logT("[lis_parser] processLine {}: {}", i, r);

               processLine(r);
            }
            
            i++;
         }
      }
   }
   catch(const std::exception& ex)
   {
      if (onParserError)
         onParserError(ex.what());
   }
}

void Parser::processLine(const std::string& data)
{
   char recordType = data[0];
   lis::Delimiter delimiter = _option.delimiter();

   if (recordType == 'H')
   {
      if (_option.autoDetectDelimiter) {
         delimiter = {}; // create empty delimiter
      }

      /*  TODO_JEFRI: Check received string format
         if (data[5] != '|')
            // Invalid Header format, Cancel/ Drop processing
      */
      // HeaderRecord constructor will parse delimiter from incoming data, if delimiter not initialized
      RecordPtr recPtr = std::make_shared<HeaderRecord>(data, delimiter);
      
      // now use header record delimiter
      _option.delimiter( recPtr->delimiter() );

      processRecord(recPtr);
   }
   else if (recordType == 'P')
   {
      RecordPtr recPtr(new PatientRecord(data)); // if using new, must be in one line
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
   else if (recordType == 'O')
   {
      RecordPtr recPtr = std::make_shared<OrderRecord>(data);
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
   else if (recordType == 'Q')
   {
      RecordPtr recPtr = std::make_shared<RequestInfoRecord>(data);
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
   else if (recordType == 'R')
   {
      RecordPtr recPtr = std::make_shared<ResultRecord>(data, _option.resultRecordTotalField );
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
   else if (recordType == 'C')
   {
      RecordPtr recPtr = std::make_shared<CommentRecord>(data);
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
   else if (recordType == 'L')
   {
      RecordPtr recPtr = std::make_shared<TerminatorRecord>(data);
      recPtr->delimiter(delimiter);
      processRecord(recPtr);
   }
}

void Parser::processRecord(RecordPtr& record)
{
   if (_enableLog) 
      Logger::logT("[lis_parser] Contructing {} object from line", record->recordTypeStr());

   // parse stored record text
   record->fromString();
   RecordType recType = record->recordType();

   // raw record converted to Record object, emit event
   if (onRecordReady)
   {
      auto rec = std::static_pointer_cast<lis::Record>(record);
      onRecordReady(rec);
   }

   // Construct Message object from every incoming record
   // if incoming record is HeaderRecord, contruct new Message object
   if (recType == RecordType::Header)
   {
      // we got Header record
      // build a new LIS message
      //_pLisMessage = std::make_shared<Message>();
      // reset the _pLisMessage, hand it a fresh instance of Message
      // (the old instance will be destroyed after this call)
      _pLisMessage.reset(new Message);
      _pLisMessage->vendorProtocolId(lis::MSG_LIS2A);
      _pLisMessage->instrumentType(this->instrumentType());

      if (_enableLog) 
      {
         Logger::logT("[lis_parser] ---------------------------------------");
         Logger::logI("[lis_parser] Constructing LIS Message");
         Logger::logT("[lis_parser] Add Header record");
      }

      _pLisMessage->setHeader(record);
   }

   if (recType == RecordType::Patient)
   {
      auto pat = std::static_pointer_cast<PatientRecord>(record);
      
      if (_enableLog) 
         Logger::logT("[lis_parser] Add Patient record: {}", pat->getPatientName().firstName() );

      _pLisMessage->addPatient(record);
      _pLisMessage->setLastTouchedRecord(record);
   }

   if (recType == RecordType::Query)
   {
      auto pat = std::static_pointer_cast<RequestInfoRecord>(record);
      
      if (_enableLog) 
         Logger::logT("[lis_parser] Add Request Info record");

      _pLisMessage->addRequestInfo(record);
      _pLisMessage->setLastTouchedRecord(record);
      _pLisMessage->hasRequest(true);
   }   

   if (recType == RecordType::Order)
   {
      auto patient = _pLisMessage->getLastPatient();
      if (patient)
      {
         auto pat = std::static_pointer_cast<PatientRecord>(patient);
         auto odr = std::static_pointer_cast<OrderRecord>(record);
         
         if (_enableLog) 
            Logger::logT("[lis_parser] Add Order record: {} to Patient: {}", odr->sequenceNumber(), pat->getPatientName().firstName());

         patient->addChild(record);
         _pLisMessage->setLastTouchedRecord(record);
      }
   }

   if (recType == RecordType::Result)
   {
      auto order = _pLisMessage->getLastPatient()->getLastChild();
      if (order)
      {
         auto odr = std::static_pointer_cast<OrderRecord>(order);
         auto res = std::static_pointer_cast<ResultRecord>(record);
         
         if (_enableLog) 
            Logger::logT("[lis_parser] Add Result record: {} to Order: {}", res->sequenceNumber(), odr->sequenceNumber());

         order->addChild(record);
         _pLisMessage->setLastTouchedRecord(record);
      }
   }

   if (recType == RecordType::Comment)
   {
      auto lastTouch = _pLisMessage->getLastTouchedRecord();
      if (lastTouch)
      {
         auto cmt = std::static_pointer_cast<CommentRecord>(record);
         
         if (_enableLog) 
            Logger::logT("[lis_parser] Add Comment: {} to {}", cmt->sequenceNumber(), lastTouch->recordTypeStr());

         lastTouch->addChild(record);
      }
   }

   if (recType == RecordType::Terminator)
   {
      _pLisMessage->getHeader()->setNext(record);

      if (_enableLog) 
         Logger::logI("[lis_parser] LIS Message construction done");

      // ok, we got a Terminator Record, end of message
      // one Message constructed, emit event
      if (onMessageReady)
      {
         auto msg = std::static_pointer_cast<lis::Message>(_pLisMessage);
         msg->vendorProtocolId(lis::MSG_LIS2A);
         onMessageReady(msg);
      }
   }
}

} // namespace lis2a
} // namespace tbs