#include <tobasa/logger.h>
#include <tobasalis/hl7/message.h>
#include "api_lis_controller.h"

namespace tbs {
namespace lis {

bool ApiLisController::isLIS2A2ProtocolMode()
{
   if (    _option.activeInstrument == lis::DEV_DEFAULT_LIS1A
        || _option.activeInstrument == lis::DEV_TEST_LIS1A
        || _option.activeInstrument == lis::DEV_INDIKO
        || _option.activeInstrument == lis::DEV_DXH_500
        || _option.activeInstrument == lis::DEV_GEM_3500
        || _option.activeInstrument == lis::DEV_SELECTRA )
   {
      return true;
   }

   return false;
}

bool ApiLisController::isHL7ProtocolMode()
{
   if (    _option.activeInstrument == lis::DEV_DEFAULT_HL7
        || _option.activeInstrument == lis::DEV_TEST_HL7)
   {
      return true;
   }

   return false;
}

void ApiLisController::hndlrLisMessageReady_HL7(std::shared_ptr<lis::Message>& message)
{
   // HL7 Message handled here
   auto msg = std::static_pointer_cast<hl7::Message>(message);
   if (!msg) 
   {
      Logger::logE("[lis_ctrl] Invalid hl7::Message shared pointer");
      return;
   }

   // Prepare ACK Message
   bool bypassValidation = false;
   auto tmpAckMessage    = msg->getACK(bypassValidation);
   auto tmpAckMessageStr = tmpAckMessage.serializeMessage(false);
   auto ackMessage       = std::make_shared<hl7::Message>( tmpAckMessageStr );
   
   ackMessage->encoding(msg->encoding());
   ackMessage->parseMessage(bypassValidation);

   // Tell LIS Engine to put ACK message in its outgoing queue, ready to be sent 
   // when it receives LIS Link OnIdle event, which will fire up ApiLisController::hndlrLisCommunicationIdle()
   lisEngine()->outgoingMessage(ackMessage, "HL7 ACK Message",
      [this]() {
         // set state back to Idle
         lisEngine()->communicationState(CommunicationState::Idle);
      });
}

} // namespace lis
} // namespace tbs