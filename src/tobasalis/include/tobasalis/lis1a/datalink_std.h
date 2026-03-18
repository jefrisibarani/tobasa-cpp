#pragma once

#include "tobasalis/lis1a/datalink_base.h"

namespace tbs {
namespace lis1a {

/** \ingroup LIS
 * DataLinkStd
 */
class DataLinkStd
   : public DataLinkBase
{
public:

   /**
    * DataLinkStd constructor.
    */  
   DataLinkStd(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);
   
   DataLinkStd() = default;
   virtual ~DataLinkStd() = default;

   /**
    * Send raw LIS2-A2 messsage. 
    * @param messageStr raw message text.
    * @param endOfLine  EOL character used in messageStr.
    * 
    * endOfLine will be used to split messageStr as lis records.
    * 
    * Example LIS2-A2 raw message text:
    * Here <CR><LF> is end of line character
    * 
    *  H|\^&|||1^LIS host^1.0|||||||P<CR>
    *  P|1|PatientID_03|||Patient Name_3|||U|||||||||||||||||Doctor Name|<CR>
    *  O|1|SampleID_03||^^^ISE|S||20101102100000||||||Test information field||3||||||||||O<CR>
    *  L|1|F<CR>
    * 
    * messageStr will be tokenized by endOfLine, then reconstructed with CR as record separator
    */
   virtual void sendMessage(const std::string& message, const std::string& endOfLine="CR");

   /**
    * Send LIS2-A2 message stored in record vector.
    * Convert each record object into raw lis record, then send.
    * @param records Records vector.
    */
   virtual void sendMessage(const std::vector<std::shared_ptr<lis::Record>>& records);

protected:

   /**
    * Calculate message cheksum
    */  
   virtual std::string calculateChecksum(const std::string& data);


   /**
    * Validate message checksum
    */  
   virtual bool checkChecksum(const std::string& line);


   /**
    * Connection receive data event handler.
    */  
   virtual void connectionOnReceiveData(const std::string& data);
};

} // namespace lis1a
} // namespace tbs