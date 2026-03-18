#include <tobasa/logger.h>
#include "api_lis_controller.h"
#include <tobasalis/lis2a/message.h>
#include <tobasalis/lis2a/header.h>
#include <tobasalis/lis2a/patient.h>
#include <tobasalis/lis2a/order.h>
#include <tobasalis/lis2a/subuniversaltest.h>
#include <tobasalis/lis2a/terminator.h>
#include "lis_db_service_factory.h"

namespace tbs {
namespace lis {

void ApiLisController::hndlrLisMessageReady_DevTest_LIS1A(std::shared_ptr<lis::Message>& message, EngineState& engineState)
{
   auto msg = std::static_pointer_cast<lis2a::Message>(message);
   if (!msg) 
   {
      Logger::logE("[lis_ctrl] Invalid lis2a::Message shared pointer");
      return;
   }

   if (_option.role == "LIS Manager")
   {
      // We act as LIS Manager

      if (lisEngine()->communicationState() == CommunicationState::Idle)
      {
         auto request = msg->getRequestInfo();
         if (request)
         {
            engineState.totalRequestMessageReceived++;

            // we received Order Request Message from analyser, onMessageReady handler should then send Order Message to Analyzer
            lisEngine()->communicationState(CommunicationState::OrderRequestMessageReceived);
            Logger::logI("[lis_ctrl] Received Order Request Message from Analyzer");

            // Prepare Order Message to send when Engine Idle after receving Request from analyzer

            // in case the message we sent, rejected by Analyzer, analyzer will not send Order Message Received ACK
            // thus our state still in CommunicationState::OrderMessageSent,
            // after +- 2 minutes, Analyzer will resend RequestInfo message
            auto orderMessage = prepareOrderMessageToAnalyzer(message);
            lisEngine()->outgoingMessage(orderMessage, "Order Message",
               [this]() {
                  lisEngine()->communicationState(CommunicationState::OrderMessageSent);
               });
         }
      }
      else if (lisEngine()->communicationState() == CommunicationState::OrderMessageSent)
      {
         // Order Message has been sent to analyzer, we received Order Message Received ACK from Analayzer
         if (msg->isAckMessage())
         {
            engineState.totalOrderMessageReceivedACK++;

            lisEngine()->communicationState(CommunicationState::OrderMessageAckReceived);
            lisEngine()->communicationState(CommunicationState::Idle);
            Logger::logI("[lis_ctrl] Received ACK message from Analyzer; Order Message received successfully by Analyzer");

            // We got Acknowledge message form Analyzer telling it has correctly received the orders
            auto header = msg->getHeaderRecord();
            auto id = header->messageControlID();
            // TODO_JEFRI: Update date in Database, set relevant tag
         }
         else
         {
            auto request = msg->getRequestInfo();
            if (request)
            {
               engineState.totalRequestMessageReceived++;

               // oops, we received RequestInfo message from Analyzer, meaning, 
               // Order Message we sent previously has been rejected by analyzer

               lisEngine()->communicationState(CommunicationState::Idle);
               Logger::logW("[lis_ctrl] Received Request Info Message from Analyzer, while waiting for  Order Message Received ACK");
               Logger::logW("[lis_ctrl] Setting back state to CommunicationState::Idle");

               // We send order now.
               // we received Order Request Message from analyser, onMessageReady handler should then send Order Message to Analyzer
               lisEngine()->communicationState(CommunicationState::OrderRequestMessageReceived);
               Logger::logI("[lis_ctrl] Received Order Request Message from Analyzer");

               // Prepare Order Message to send when Engine Idle after receving Request from analyzer
               auto orderMessage = prepareOrderMessageToAnalyzer(message);
               lisEngine()->outgoingMessage(orderMessage, "Order Message",
                  [this]() {
                     lisEngine()->communicationState(CommunicationState::OrderMessageSent);
                  });
            }
         }
      }
      else {
         // should already Idle
      }
   }
   else if (_option.role == "Analyzer")
   {
      // We act as Analyzer

      if (lisEngine()->communicationState() == CommunicationState::Idle)
      {
         if (msg->isOrderRequest())
         {
            lisEngine()->communicationState(CommunicationState::OrderMessageReceived);

            Logger::logI("[lis_ctrl] Received Order Message from LIS Manager");

            // Prepare ACK Message, to be sent on Lis connection Idle Event
            auto ackMessage = prepareAckMessageToLisManager(message);
            lisEngine()->outgoingMessage(ackMessage, "ACK Message",
               [this](){
                  // set state back to Idle, bacause ACK Message sent to LIS Manager does not need a reply
                  lisEngine()->communicationState(CommunicationState::Idle);
               });
         }
      }
   }
   else
      Logger::logE("[lis_engine] Invalid LIS engine role");
}

void ApiLisController::hndlrLisCommunicationIdle_DevTest_LIS1A()
{
   if (_option.role == "LIS Manager")
   {
      // Act as LIS Manager

      if ( ( lisEngine()->communicationState() == CommunicationState::OrderRequestMessageReceived) &&
         ! lisEngine()->outgoingMessages().empty() )
      {
         Logger::logI("[lis_engine] Sending Order Message to Analyzer");
         // TODO_JEFRI: Fix this!! how about Serial connection?
         lisEngine()->sendPendingMessage();
      }
      else if ( lisEngine()->communicationState() == CommunicationState::OrderMessageAckReceived)
         lisEngine()->communicationState(CommunicationState::Idle);
      else {} // should already Idle
   }
   else if (_option.role == "Analyzer")
   {
      // Act as Analyzer

      if ( ( lisEngine()->communicationState() == CommunicationState::OrderMessageReceived) &&
         ! lisEngine()->outgoingMessages().empty() )
      {
         Logger::logI("[lis_engine] Sending ACK message to LIS Manager");
         // TODO_JEFRI: Fix this!! how about Serial connection?
         lisEngine()->sendPendingMessage();
      }
      else if ( lisEngine()->communicationState() == CommunicationState::OrderMessageReceivedAckSent)
         lisEngine()->communicationState(CommunicationState::Idle);
      else {} // should already Idle
   }
   else
      Logger::logE("[lis_engine] Invalid LIS engine role");
}

std::shared_ptr<lis::Message> ApiLisController::prepareAckMessageToLisManager(std::shared_ptr<lis::Message>& message)
{
   auto msg = std::static_pointer_cast<lis2a::Message>(message);
   auto reqHeader = msg->getHeaderRecord();
   auto headerRecord = std::make_shared<lis2a::HeaderRecord>(std::string(), reqHeader->delimiter());

   headerRecord->setMessageControlID(reqHeader->messageControlID());
   headerRecord->setSenderID(_option.hostId);
   headerRecord->setReceiverID(reqHeader->senderID());

   // The value of this field will be always ‘P’ for Production.
   headerRecord->setProcessingID("P");

   // The version number sent and supported is ‘LIS2A’
   headerRecord->setVersion(reqHeader->version());

   std::string msgDateTime = DateTime::now().format("{:%Y%m%d%H%M%S}");
   headerRecord->setMessageDateTime(msgDateTime);

   headerRecord->setComment("ACK");

   auto newMessage = std::make_shared<lis2a::Message>();
   newMessage->setHeader(headerRecord);

   // L|1|F
   auto terminator = std::make_shared<lis2a::TerminatorRecord>();
   terminator->delimiter(reqHeader->delimiter());
   terminator->setCode(lis2a::TerminationCode::Normal);

   newMessage->getHeader()->setNext(terminator);

   auto rawMsg = newMessage->toString();
   Logger::logT("[lis_ctrl] -- Prepared ACK Message To Lis Manager ------------");
   Logger::logT("[lis_ctrl] {}", rawMsg);
   Logger::logT("[lis_ctrl] ----------------------------------------------------");
   return newMessage;
}

std::shared_ptr<lis::Message> ApiLisController::prepareOrderMessageToAnalyzer(std::shared_ptr<lis::Message>& message)
{
   auto msg       = std::static_pointer_cast<lis2a::Message>(message);
   auto reqHeader = msg->getHeaderRecord();
   auto request   = msg->getRequestInfo();

   /*
      Answer Request Info coming from analyzer (Analyzer requesting orders from LIS)

      Recv: <ENQ>
      Send: <ACK>
      Recv: <STX>H|\^&|69F2746D24014F21AD7139756F64CAD8||TobasaLab|||||DevTest_LIS1A||
      P|LIS2A|20130129102030<CR>
      P|1||PID01||Campeny^Ricard||19850819|M<CR>
      O|1|0010079||^^^ASO|R|20130129101530|20130129092030||||A||||SER|||||^56|||||O<CR>
      L|1|N<CR><ETX>OB<CR><LF>
      Send: <ACK>
      Recv: <EOT>
   */
   auto repo = _dbServiceFactory->lis2aService(_option.getDbDataWithNewConnection);

   // DevTest use field 3 as order code range, separated with repeat delimiter
   // the values will reside in startRange's patientID
   auto orderCodes = request->startRange().patientID();
   auto headerRecord = std::make_shared<lis2a::HeaderRecord>(std::string(), reqHeader->delimiter());

   headerRecord->setMessageControlID(reqHeader->messageControlID());
   headerRecord->setSenderID(_option.hostId);
   headerRecord->setReceiverID(reqHeader->senderID());

   // The value of this field will be always ‘P’ for Production.
   headerRecord->setProcessingID("P");

   // The version number sent and supported is ‘LIS2A’
   headerRecord->setVersion(reqHeader->version());

   std::string msgDateTime = DateTime::now().format("{:%Y%m%d%H%M%S}");
   headerRecord->setMessageDateTime(msgDateTime);

   auto newMessage = std::make_shared<lis2a::Message>();
   newMessage->setHeader(headerRecord);

   // there might be exists multiple order(with same order code) for one patient, thus retrieve all order for the patient
   // we only need the last order

   std::vector<svc::JobOrder> jobOrderArr;
   if (orderCodes=="ALL")
   {
      jobOrderArr = repo->getAllJobOrderCode();
   }
   else
   {
      // orderCodes value may also in repeatable form
      // eg: 2400002048\\2400002049\\4800002049\\2400002050\\2400002051
      std::vector<std::string> codes = util::split( orderCodes, request->repeatDelimiter() );
      for (auto c: codes)
      {
         auto orderArr = repo->getJobOrderCode(c);
         for (auto x: orderArr)
         {
            if (! x.ignore)
               jobOrderArr.push_back(x);
         }
      }
   }

   int patIdx = 1;
   for (auto jobOrder: jobOrderArr)
   {
      if (jobOrder.ignore) {
         continue;
      }

      auto patientRecord = std::make_shared<lis2a::PatientRecord>();
      patientRecord->delimiter(reqHeader->delimiter());
      patientRecord->setSequenceNumber(patIdx);

      auto patienData = repo->getJobOrderPatient(jobOrder);
      if (patienData.empty())
      {
         /*
            Answer with No info message from patient that no exist in LIS

            Produce, patient & orders similar to this records
            P|4
            O|1|2400002050|||R||||||||||||||||||||Y\Q
         */
         newMessage->addPatient(patientRecord);
         newMessage->setLastTouchedRecord(patientRecord);

         auto orderRecord = std::make_shared<lis2a::OrderRecord>();
         orderRecord->delimiter(reqHeader->delimiter());
         orderRecord->setSequenceNumber(1);
         orderRecord->setSpecimenID(jobOrder.code);

         orderRecord->setPriority("R");

         auto repeatDelim = reqHeader->delimiter().repeatDelimiter;
         std::string reportType = "Y";             // Y = no order on record for this test
         reportType.append(1, repeatDelim);
         reportType.append("Q");                   // Q = response to query
         orderRecord->setReportType(reportType);   //  ==>  Y/Q

         patientRecord->addChild(orderRecord);
         newMessage->setLastTouchedRecord(orderRecord);
      }
      else
      {
         /*
            Data found in database,
            Build patient & orders similar to this records

            P|3||JM0014||DOE^JHON|||F
            O|1|4800002049||^^^CREATININE|R||||||A||||URI||||||||||O
            O|2|4800002049||^^^URIC_ACID|R||||||A||||URI||||||||||O
            O|3|4800002049||^^^PHOSPHORUS|R||||||A||||URI||||||||||O
            O|4|4800002049||^^^UREA-BUN-UV|R||||||A||||URI||||||||||O
         */

         std::string labPatid = patienData["lab_patid"].get<std::string>();
         if (labPatid.length() > 30)
         {
            labPatid = labPatid.substr(0,30);
            Logger::logE("[lis_ctrl] Invalid Lab patient Id length. Maximum length is 30 characters");
         }
         patientRecord->setLaboratoryAssignedPatientID( labPatid );

         lis2a::PatientName patientName;

         std::string firstname = patienData["firstname"].get<std::string>();
         if (firstname.length() > 30)
         {
            firstname = firstname.substr(0,30);
            Logger::logE("[lis_ctrl] Invalid First name length. Maximum length is 30 characters");
         }
         patientName.setFirstName(firstname);


         std::string lastname = patienData["lastname"].get<std::string>();
         if (lastname.length() > 30)
         {
            lastname = lastname.substr(0,30);
            Logger::logE("[lis_ctrl] Invalid Last name length. Maximum length is 30 characters");
         }
         patientName.setLastName(lastname);

         patientRecord->setPatientName(patientName);

         std::string birthdate = patienData["birthdate"].get<std::string>();
         patientRecord->setBirthdate(birthdate);

         patientRecord->setPatientSex(patienData["gender"].get<std::string>());

         std::string location = patienData["location_id"].get<std::string>();
         if (location.length() > 20)
         {
            location = location.substr(0,20);
            Logger::logE("[lis_ctrl] Invalid Location Id length. Maximum length is 20 characters");
         }
         patientRecord->setLocation(location);

         newMessage->addPatient(patientRecord);
         newMessage->setLastTouchedRecord(patientRecord);

         auto orderData = repo->getJobOrderDetail(jobOrder);
         if (!orderData.empty())
         {
            int orderIdx = 1;
            for (auto order: orderData)
            {
               auto orderRecord = std::make_shared<lis2a::OrderRecord>();
               orderRecord->delimiter(reqHeader->delimiter());
               orderRecord->setSequenceNumber(orderIdx);

               std::string specimenId = order["spec_id"].get<std::string>();
               if (jobOrder.code !=  specimenId)
                  Logger::logW("[lis_ctrl] Invalid order code and specimen id, order code={}, specimen id={}", jobOrder.code, specimenId);

               if (specimenId.length() > 30)
                  Logger::logE("[lis_ctrl] Invalid Specimen Id length. Maximum length is 30 characters");

               orderRecord->setSpecimenID(specimenId);

               lis2a::UniversalTestID testId;
               std::string testCode  = order["test_code"].get<std::string>();
               std::string testGroup = order["test_group"].get<std::string>();
               if (testCode.length() > 50)
                  Logger::logE("[lis_ctrl] Invalid Test code length. Maximum length is 50 characters");
               if (testGroup.length() > 50)
                  Logger::logE("[lis_ctrl] Invalid Test group length. Maximum length is 50 characters");

               testId.setManufacturerCode(testCode);
               testId.setOptionalField1(testGroup);
               orderRecord->setTestID(testId);

               /*
               DevTest_LIS1A Analyzer Order record's field no.6: Priority
               ‘S’, for STAT.
               ‘R’, for routine samples.
               */
               std::string priorityId = order["priority_id"].get<std::string>();
               if ( ! (priorityId == "S" || priorityId == "R") ) {
                  Logger::logE("Invalid Priority Id. Allowed Id are S and R");
               }
               orderRecord->setPriority(priorityId);

               orderRecord->setOrderedDateTime(order["ordered_datetime"].get<std::string>());
               orderRecord->setSpecimenCollectionDateTime(order["spec_coll_datetime"].get<std::string>());


               orderRecord->setActionCode(order["action_code"].get<std::string>());

               orderRecord->setSpecimenDescriptor(order["spec_type"].get<std::string>());
               orderRecord->setOrderingPhysician(order["physician_name"].get<std::string>());

               //orderRecord->setUserField1(order["xxx"].get<std::string>());

               auto labField1_1 = order["ordering_facility"].get<std::string>();
               auto labField1_2 = order["spec_dilution_factor"].get<std::string>();
               auto labField1_3 = order["auto_rerun_allowed"].get<std::string>();
               auto labField1_4 = order["auto_reflex_allowed"].get<std::string>();
               auto  c_delim = reqHeader->componentDelimiter();
               std::string labField = labField1_1 + c_delim + labField1_2 + c_delim + labField1_3 + c_delim + labField1_4;
               util::removeTraillingChar(labField, c_delim);
               orderRecord->setLaboratoryField1(labField);

               // Order record's field no.22: Laboratory field #2
               orderRecord->setLaboratoryField2(order["parent_spec_id"].get<std::string>());

               // only send by machine
               //orderRecord->setLastModified(order["result_reported"].get<std::string>();

               orderRecord->setReportType(order["report_type"].get<std::string>());

               orderIdx++;

               auto patient = newMessage->getLastPatient();
               if (patient)
               {
                  patient->addChild(orderRecord);
                  newMessage->setLastTouchedRecord(orderRecord);
               }
            }
         } // order not empty
      } // patient not empty

      patIdx ++;
   }

   // L|1|F
   auto terminator = std::make_shared<lis2a::TerminatorRecord>();
   terminator->delimiter(reqHeader->delimiter());
   terminator->setCode(lis2a::TerminationCode::LastRequestForInformationProcessed);

   newMessage->getHeader()->setNext(terminator);

   auto rawMsg = newMessage->toString();
   Logger::logT("[lis_ctrl] -- Prepared Order Message To Lis Analyzer ----------");
   Logger::logT("[lis_ctrl] {}", rawMsg);
   Logger::logT("[lis_ctrl] ----------------------------------------------------");
   return newMessage;
}

} // namespace lis
} // namespace tbs