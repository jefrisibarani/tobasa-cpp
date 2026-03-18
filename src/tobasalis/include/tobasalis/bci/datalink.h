#pragma once

#include "tobasalis/lis1a/datalink_base.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * BCI DataLink
 */
class DataLinkBci
   : public lis1a::DataLinkBase
{
public:

   /**
    * DataLinkBci constructor.
    */
   DataLinkBci(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);

   DataLinkBci() = default;
   virtual ~DataLinkBci() = default;

private:

   /**
    * Calculate message checksum.
    */
   std::string calculateChecksum(const std::string& data);

   /**
    * Validate message checksum.
    */
   bool checkChecksum(const std::string& line);

   /**
    * Connection receive data event handler.
    */
   void connectionOnReceiveData(const std::string& data);

private:
   const char RS = '\x1E';    // hex: 1E    dec: 30
   const char GS = '\x1D';    // hex: 1D    dec: 29
};

} // namespace bci
} // namespace tbs