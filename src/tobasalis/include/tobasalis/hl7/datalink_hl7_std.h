#pragma once

#include "tobasalis/lis/datalink.h"

namespace tbs {
namespace hl7 {

/** \ingroup LIS
 * DataLinkHl7Std.
 *
 * HL7 message
 * <SB> datadatadata <EB><CR>
 *
 * <SB> Beginning of the message, ASCII code is <VT>, which is 0x0B;
 * <EB> The End of the message,   ASCII code is <FS>, which is 0x1C；
 * <CR> The end confirmation of message, also the separation sign between two different message
 *      The corresponding ASCII code is 0x0D；
 * datadatadata means the message Content, which is including several segments, at end of each segment
 * it ends with <CR>, that is 0x0D
 *
 * example:
 *   <VT>
 *   MSH|^~\&|DevTest_LIS1A|TobasaSystem|Host|Host provider|20130708074756||NACK^NACK|80|P|2.5.1||||||UNICODE UTF-8||<CR>
 *   MSA|AR|<CR>
 *   ERR|||207|E|invalidMessage||Message rejected<CR>
 *   <FS><CR>
 */
class DataLinkHl7Std
   : public lis::DataLink
{
public:

   /**
    * DataLinkHl7Std constructor.
    */
   DataLinkHl7Std(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);

   DataLinkHl7Std() = default;
   virtual ~DataLinkHl7Std() = default;

   /**
    * Send HL7 message to endpoint
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    *
    * endOfLine will be used to split messageStr as HL7 Message segment
    * messageStr will be tokenized by endOfLine, then reconstructed with CR as record separator
    */
   virtual void sendMessage(const std::string& message, const std::string& endOfLine="CRLF");

protected:

   /**
    * Link state.
    */
   enum class State
   {
      Idle,
      GotSB,   // Start Block       : VT
      GotEB    // End Block         : FS CR
   };

   State _state;

   /**
    * Connection receive data event handler.
    */
   virtual void connectionOnReceiveData(const std::string& data);

   /**
    * Send test message.
    */
   virtual void sendTestMessage(const std::string& rawdata="");


   virtual void sendString(const std::string& data);


   const char VT = '\x0B';    // hex: 0B    dec: 11   -> Start Block
   const char FS = '\x1C';    // hex: 1C    dec: 28   -> End Block

   char _previousCharacter = NUL;
};

} // namespace hl7
} // namespace tbs